#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaMethod>
#include <QMetaObject>
#include <QMetaType>
#include <QSet>
#include <QString>
#include <QTextStream>

#include "hostapi/HostApiClassList.h"
#include "hostapi/HostApiClasses.h"
#include "hostapi/HostApiEventTypes.h"
#include "hostapi/HostApiVersion.h"

namespace {

struct ParamInfo {
  QString name;
  QString qtType;
  QString tsType;
};

struct MethodInfo {
  QString name;
  QString qtReturn;
  QString tsReturn;
  bool returnsVoid = false;
  QList<ParamInfo> params;
};

struct SignalInfo {
  QString name;
  QList<ParamInfo> params;
};

struct ClassInfo {
  QString name;
  QString cppName;
  QList<MethodInfo> methods;
  QList<SignalInfo> signalInfos;
};

QString normalizeType(const QString &typeName) {
  QString type = typeName;
  type.replace("const ", "");
  type.replace("&", "");
  type.replace("*", "");
  return type.trimmed();
}

bool splitTemplateArgs(const QString &typeName, QStringList *outArgs) {
  const int start = typeName.indexOf('<');
  const int end = typeName.lastIndexOf('>');
  if (start <= 0 || end <= start) {
    return false;
  }
  const QString inner = typeName.mid(start + 1, end - start - 1);
  int depth = 0;
  QString current;
  for (QChar ch : inner) {
    if (ch == '<') {
      depth++;
      current.append(ch);
      continue;
    }
    if (ch == '>') {
      depth--;
      current.append(ch);
      continue;
    }
    if (ch == ',' && depth == 0) {
      outArgs->append(current.trimmed());
      current.clear();
      continue;
    }
    current.append(ch);
  }
  if (!current.trimmed().isEmpty()) {
    outArgs->append(current.trimmed());
  }
  return !outArgs->isEmpty();
}

bool mapScalarToTs(const QString &typeName, QString *outTs) {
  static const QSet<QString> kNumeric = {"short", "ushort", "int", "uint", "long", "ulong",
                                         "qint64", "quint64", "float", "double"};
  if (typeName == "bool") {
    *outTs = "boolean";
    return true;
  }
  if (kNumeric.contains(typeName)) {
    *outTs = "number";
    return true;
  }
  if (typeName == "QString") {
    *outTs = "string";
    return true;
  }
  if (typeName == "QStringList") {
    *outTs = "string[]";
    return true;
  }
  if (typeName == "QJsonValue" || typeName == "QVariant") {
    *outTs = "any";
    return true;
  }
  if (typeName == "QJsonObject" || typeName == "QVariantMap" || typeName == "QVariantHash") {
    *outTs = "Record<string, any>";
    return true;
  }
  if (typeName == "QJsonArray" || typeName == "QVariantList") {
    *outTs = "any[]";
    return true;
  }
  return false;
}

bool mapTypeToTs(const QString &typeName, QString *outTs) {
  const QString normalized = normalizeType(typeName);
  if (normalized == "void") {
    *outTs = "void";
    return true;
  }
  if (mapScalarToTs(normalized, outTs)) {
    return true;
  }
  if (normalized.startsWith("QList<") || normalized.startsWith("QVector<") ||
      normalized.startsWith("QSet<")) {
    QStringList args;
    if (!splitTemplateArgs(normalized, &args) || args.size() != 1) {
      return false;
    }
    QString innerTs;
    if (!mapScalarToTs(normalizeType(args[0]), &innerTs)) {
      return false;
    }
    *outTs = innerTs + "[]";
    return true;
  }
  if (normalized.startsWith("QMap<") || normalized.startsWith("QHash<")) {
    QStringList args;
    if (!splitTemplateArgs(normalized, &args) || args.size() != 2) {
      return false;
    }
    const QString keyType = normalizeType(args[0]);
    QString valueTs;
    if (keyType != "QString") {
      return false;
    }
    if (!mapScalarToTs(normalizeType(args[1]), &valueTs)) {
      return false;
    }
    *outTs = "Record<string, " + valueTs + ">";
    return true;
  }
  return false;
}

QString toPascalCase(const QString &name) {
  QString result;
  bool capitalizeNext = true;
  for (QChar ch : name) {
    if (!ch.isLetterOrNumber()) {
      capitalizeNext = true;
      continue;
    }
    if (capitalizeNext) {
      result.append(ch.toUpper());
      capitalizeNext = false;
    } else {
      result.append(ch);
    }
  }
  return result.isEmpty() ? QStringLiteral("HostApi") : result;
}

QString safeParamName(const QString &name, int index) {
  if (!name.isEmpty()) {
    return name;
  }
  return QStringLiteral("arg%1").arg(index);
}

bool isClassExposed(const QMetaObject *meta) {
  for (int i = 0; i < meta->classInfoCount(); ++i) {
    const QMetaClassInfo info = meta->classInfo(i);
    if (QString::fromLatin1(info.name()) == QStringLiteral("HostApi.Expose")) {
      return QString::fromLatin1(info.value()) == QStringLiteral("true");
    }
  }
  return false;
}

QString classExportName(const QMetaObject *meta, const QString &fallback) {
  for (int i = 0; i < meta->classInfoCount(); ++i) {
    const QMetaClassInfo info = meta->classInfo(i);
    if (QString::fromLatin1(info.name()) == QStringLiteral("HostApi.Name")) {
      return QString::fromLatin1(info.value());
    }
  }
  return fallback;
}

ClassInfo buildClassInfo(const QMetaObject *meta, const QString &exportName, QStringList *warnings) {
  ClassInfo info;
  info.name = exportName;
  info.cppName = QString::fromLatin1(meta->className());

  const int methodStart = meta->methodOffset();
  const int methodEnd = meta->methodCount();
  for (int i = methodStart; i < methodEnd; ++i) {
    const QMetaMethod method = meta->method(i);
    const QString methodName = QString::fromLatin1(method.name());
    const QList<QByteArray> paramTypes = method.parameterTypes();
    const QList<QByteArray> paramNames = method.parameterNames();

    QList<ParamInfo> params;
    bool supported = true;
    for (int p = 0; p < paramTypes.size(); ++p) {
      const QString qtType = QString::fromLatin1(paramTypes[p]);
      QString tsType;
      if (!mapTypeToTs(qtType, &tsType)) {
        supported = false;
        if (warnings) {
          warnings->append(QStringLiteral("Unsupported param type %1 on %2::%3")
                               .arg(qtType, info.cppName, methodName));
        }
      }
      ParamInfo param;
      param.name = safeParamName(QString::fromLatin1(paramNames.value(p)), p);
      param.qtType = qtType;
      param.tsType = tsType;
      params.append(param);
    }

    if (method.methodType() == QMetaMethod::Signal) {
      if (!supported) {
        continue;
      }
      SignalInfo signalInfo;
      signalInfo.name = methodName;
      signalInfo.params = params;
      info.signalInfos.append(signalInfo);
      continue;
    }

    if (method.access() != QMetaMethod::Public) {
      continue;
    }

    if (method.methodType() != QMetaMethod::Method && method.methodType() != QMetaMethod::Slot) {
      continue;
    }

    const int returnTypeId = method.returnType();
    QString returnType = QString::fromLatin1(QMetaType(returnTypeId).name());
    if (returnType.isEmpty()) {
      returnType = QStringLiteral("void");
    }
    QString tsReturn;
    if (!mapTypeToTs(returnType, &tsReturn)) {
      supported = false;
      if (warnings) {
        warnings->append(QStringLiteral("Unsupported return type %1 on %2::%3")
                             .arg(returnType, info.cppName, methodName));
      }
    }

    if (!supported) {
      continue;
    }

    MethodInfo methodInfo;
    methodInfo.name = methodName;
    methodInfo.qtReturn = returnType;
    methodInfo.tsReturn = tsReturn;
    methodInfo.returnsVoid = (tsReturn == "void");
    methodInfo.params = params;
    info.methods.append(methodInfo);
  }

  return info;
}

QJsonObject toJson(const ClassInfo &info) {
  QJsonObject obj;
  obj.insert(QStringLiteral("name"), info.name);
  obj.insert(QStringLiteral("cppName"), info.cppName);

  QJsonArray methods;
  for (const auto &method : info.methods) {
    QJsonObject methodObj;
    methodObj.insert(QStringLiteral("name"), method.name);
    methodObj.insert(QStringLiteral("returnType"), method.qtReturn);
    methodObj.insert(QStringLiteral("tsReturn"), method.tsReturn);
    methodObj.insert(QStringLiteral("returnsVoid"), method.returnsVoid);
    QJsonArray params;
    for (const auto &param : method.params) {
      QJsonObject paramObj;
      paramObj.insert(QStringLiteral("name"), param.name);
      paramObj.insert(QStringLiteral("type"), param.qtType);
      paramObj.insert(QStringLiteral("tsType"), param.tsType);
      params.append(paramObj);
    }
    methodObj.insert(QStringLiteral("params"), params);
    methods.append(methodObj);
  }
  obj.insert(QStringLiteral("methods"), methods);

  QJsonArray signalArray;
  for (const auto &signal : info.signalInfos) {
    QJsonObject signalObj;
    signalObj.insert(QStringLiteral("name"), signal.name);
    QJsonArray params;
    for (const auto &param : signal.params) {
      QJsonObject paramObj;
      paramObj.insert(QStringLiteral("name"), param.name);
      paramObj.insert(QStringLiteral("type"), param.qtType);
      paramObj.insert(QStringLiteral("tsType"), param.tsType);
      params.append(paramObj);
    }
    signalObj.insert(QStringLiteral("params"), params);
    signalArray.append(signalObj);
  }
  obj.insert(QStringLiteral("signals"), signalArray);
  return obj;
}

bool writeFile(const QString &path, const QString &content) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }
  QTextStream stream(&file);
  stream << content;
  return true;
}

QString generateCppHeader() {
  QString text;
  text += "#pragma once\n\n";
  text += "#include <QJsonObject>\n";
  text += "#include <QList>\n";
  text += "#include <QObject>\n";
  text += "#include <QString>\n\n";
  text += "class QWebChannel;\n\n";
  text += "struct HostApiObjectInfo {\n";
  text += "  QString name;\n";
  text += "  QObject *instance = nullptr;\n";
  text += "};\n\n";
  text += "QList<HostApiObjectInfo> registerHostApiObjects(QWebChannel *channel, QObject *parent);\n";
  text += "QJsonObject hostApiSchema();\n";
  return text;
}

QString generateCppSource(const QList<ClassInfo> &classes, const QJsonObject &schema) {
  QString text;
  text += "#include \"HostApiGenerated.h\"\n";
  text += "\n";
  text += "#include <QByteArray>\n";
  text += "#include <QJsonDocument>\n";
  text += "#include <QJsonObject>\n";
  text += "#include <QWebChannel>\n";
  text += "\n";

  for (const auto &info : classes) {
    text += "#include \"" + info.cppName + ".h\"\n";
  }
  text += "\n";

  const QByteArray schemaJson = QJsonDocument(schema).toJson(QJsonDocument::Compact);
  text += "static const char kHostApiSchemaJson[] = R\"JSON(";
  text += QString::fromUtf8(schemaJson);
  text += ")JSON\";\n\n";

  text += "QJsonObject hostApiSchema() {\n";
  text += "  static QJsonObject cached;\n";
  text += "  static bool initialized = false;\n";
  text += "  if (!initialized) {\n";
  text += "    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(kHostApiSchemaJson));\n";
  text += "    if (doc.isObject()) {\n";
  text += "      cached = doc.object();\n";
  text += "    }\n";
  text += "    initialized = true;\n";
  text += "  }\n";
  text += "  return cached;\n";
  text += "}\n\n";

  text += "QList<HostApiObjectInfo> registerHostApiObjects(QWebChannel *channel, QObject *parent) {\n";
  text += "  QList<HostApiObjectInfo> objects;\n";
  text += "  if (!channel) {\n";
  text += "    return objects;\n";
  text += "  }\n";

  for (const auto &info : classes) {
    text += "  {\n";
    text += "    auto *instance = new " + info.cppName + "(parent);\n";
    text += "    const QString name = QStringLiteral(\"" + info.name + "\");\n";
    text += "    channel->registerObject(QStringLiteral(\"HostApi_\") + name, instance);\n";
    text += "    objects.append({name, instance});\n";
    text += "  }\n";
  }
  text += "  return objects;\n";
  text += "}\n";
  return text;
}

QString generateDts(const QList<ClassInfo> &classes) {
  QString text;
  text += "// Generated HostApi types. Do not edit.\n\n";
  text += "export interface HostApiSchemaParam {\n";
  text += "  name: string;\n";
  text += "  type: string;\n";
  text += "  tsType: string;\n";
  text += "}\n\n";
  text += "export interface HostApiSchemaMethod {\n";
  text += "  name: string;\n";
  text += "  returnType: string;\n";
  text += "  tsReturn: string;\n";
  text += "  returnsVoid: boolean;\n";
  text += "  params: HostApiSchemaParam[];\n";
  text += "}\n\n";
  text += "export interface HostApiSchemaSignal {\n";
  text += "  name: string;\n";
  text += "  params: HostApiSchemaParam[];\n";
  text += "}\n\n";
  text += "export interface HostApiSchemaObject {\n";
  text += "  name: string;\n";
  text += "  cppName: string;\n";
  text += "  methods: HostApiSchemaMethod[];\n";
  text += "  signals: HostApiSchemaSignal[];\n";
  text += "}\n\n";
  text += "export interface HostApiSchema {\n";
  text += "  version: string;\n";
  text += "  eventTypes: string[];\n";
  text += "  objects: HostApiSchemaObject[];\n";
  text += "}\n\n";

  for (const auto &info : classes) {
    const QString ifaceName = toPascalCase(info.name);
    text += "export interface " + ifaceName + "Api {\n";
    for (const auto &method : info.methods) {
      QStringList params;
      for (const auto &param : method.params) {
        params.append(param.name + ": " + param.tsType);
      }
      const QString returnType = method.returnsVoid ? "Promise<void>" : "Promise<" + method.tsReturn + ">";
      text += "  " + method.name + "(" + params.join(", ") + "): " + returnType + ";\n";
    }

    if (info.signalInfos.isEmpty()) {
      text += "  registerEventHandler(eventName: string, handler: (...args: any[]) => void): void;\n";
      text += "  removeEventHandler(eventName: string, handler: (...args: any[]) => void): void;\n";
    } else {
      for (const auto &signal : info.signalInfos) {
        QStringList params;
        for (const auto &param : signal.params) {
          params.append(param.name + ": " + param.tsType);
        }
        const QString handler = "(" + params.join(", ") + ") => void";
        text += "  registerEventHandler(eventName: \"" + signal.name + "\", handler: " + handler + "): void;\n";
        text += "  removeEventHandler(eventName: \"" + signal.name + "\", handler: " + handler + "): void;\n";
      }
    }
    text += "  __raw?: any;\n";
    text += "}\n\n";
  }

  text += "export interface HostApiRoot {\n";
  text += "  version: string;\n";
  text += "  schema: HostApiSchema;\n";
  text += "  validEventTypes: string[];\n";
  text += "  sendData(payload: any): void;\n";
  text += "  setOutput(text: string): void;\n";
  text += "  getInput(): Promise<string>;\n";
  text += "  addEventListener(eventName: string, handler: (payload: any) => void): void;\n";
  text += "  removeEventListener(eventName: string, handler: (payload: any) => void): void;\n";
  for (const auto &info : classes) {
    const QString ifaceName = toPascalCase(info.name);
    text += "  " + info.name + ": " + ifaceName + "Api;\n";
  }
  text += "}\n\n";

  text += "declare global {\n";
  text += "  interface Window {\n";
  text += "    HostApi: HostApiRoot;\n";
  text += "    HostApiExpectedVersion?: string;\n";
  text += "  }\n";
  text += "}\n";
  return text;
}

QString generateAngularService(const ClassInfo &info) {
  const QString className = toPascalCase(info.name) + "Service";
  QString text;
  text += "import { Injectable } from \"@angular/core\";\n\n";
  text += "@Injectable({ providedIn: \"root\" })\n";
  text += "export class " + className + " {\n";
  text += "  private get api() {\n";
  text += "    if (!window || !window.HostApi || !window.HostApi." + info.name + ") {\n";
  text += "      throw new Error(\"HostApi is not ready.\");\n";
  text += "    }\n";
  text += "    return window.HostApi." + info.name + ";\n";
  text += "  }\n\n";

  for (const auto &method : info.methods) {
    QStringList params;
    QStringList paramNames;
    for (const auto &param : method.params) {
      params.append(param.name + ": " + param.tsType);
      paramNames.append(param.name);
    }
    const QString returnType = method.returnsVoid ? "Promise<void>" : "Promise<" + method.tsReturn + ">";
    text += "  " + method.name + "(" + params.join(", ") + "): " + returnType + " {\n";
    text += "    return this.api." + method.name + "(" + paramNames.join(", ") + ");\n";
    text += "  }\n\n";
  }

  if (info.signalInfos.isEmpty()) {
    text += "  registerEventHandler(eventName: string, handler: (...args: any[]) => void): void {\n";
    text += "    this.api.registerEventHandler(eventName, handler);\n";
    text += "  }\n\n";
    text += "  removeEventHandler(eventName: string, handler: (...args: any[]) => void): void {\n";
    text += "    this.api.removeEventHandler(eventName, handler);\n";
    text += "  }\n";
  } else {
    for (const auto &signal : info.signalInfos) {
      QStringList params;
      QStringList paramNames;
      for (const auto &param : signal.params) {
        params.append(param.name + ": " + param.tsType);
        paramNames.append(param.name);
      }
      const QString handler = "(" + params.join(", ") + ") => void";
      text += "  registerEventHandler(eventName: \"" + signal.name + "\", handler: " + handler + "): void {\n";
      text += "    this.api.registerEventHandler(eventName, handler);\n";
      text += "  }\n\n";
      text += "  removeEventHandler(eventName: \"" + signal.name + "\", handler: " + handler + "): void {\n";
      text += "    this.api.removeEventHandler(eventName, handler);\n";
      text += "  }\n\n";
    }
    if (!info.signalInfos.isEmpty()) {
      text.chop(2);
      text += "\n";
    }
  }

  text += "}\n";
  return text;
}

QString generatePackageJson(const QString &packageName) {
  const QString version = hostApiVersion();
  QJsonObject root;
  root.insert(QStringLiteral("name"), packageName);
  root.insert(QStringLiteral("version"), version);
  root.insert(QStringLiteral("description"), QStringLiteral("Generated HostApi types and Angular services."));
  root.insert(QStringLiteral("types"), QStringLiteral("hostapi.d.ts"));
  QJsonArray files;
  files.append(QStringLiteral("hostapi.d.ts"));
  files.append(QStringLiteral("angular/"));
  root.insert(QStringLiteral("files"), files);
  QJsonObject exports;
  exports.insert(QStringLiteral("."), QStringLiteral("./hostapi.d.ts"));
  exports.insert(QStringLiteral("./angular/*"), QStringLiteral("./angular/*"));
  root.insert(QStringLiteral("exports"), exports);

  QJsonDocument doc(root);
  return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

} // namespace

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  QCommandLineParser parser;
  parser.setApplicationDescription("HostApi code generator");
  parser.addHelpOption();

  QCommandLineOption outputDirOpt(QStringList() << "o" << "output-dir",
                                  "Output directory for generated files.",
                                  "path");
  QCommandLineOption packageNameOpt(QStringList() << "p" << "package-name",
                                    "NPM package name for generated types.",
                                    "name", "@qt-web/hostapi");
  parser.addOption(outputDirOpt);
  parser.addOption(packageNameOpt);
  parser.process(app);

  const QString outputDir = parser.value(outputDirOpt);
  if (outputDir.isEmpty()) {
    QTextStream(stderr) << "--output-dir is required.\n";
    return 1;
  }

  const QString packageName = parser.value(packageNameOpt);

  QDir out(outputDir);
  if (!out.exists() && !out.mkpath(".")) {
    QTextStream(stderr) << "Failed to create output dir: " << outputDir << "\n";
    return 1;
  }

  const QString angularDir = out.filePath("angular");
  QDir().mkpath(angularDir);
  const QString npmDir = out.filePath("npm");
  QDir().mkpath(npmDir);
  const QString npmAngularDir = QDir(npmDir).filePath("angular");
  QDir().mkpath(npmAngularDir);

  QList<ClassInfo> classes;
  QStringList warnings;

  struct ClassDescriptor {
    const QMetaObject *meta;
    QString exportName;
  };

  QList<ClassDescriptor> descriptors;

#define HOSTAPI_ADD_CLASS(Type, ExportName) \
  descriptors.append({&Type::staticMetaObject, QStringLiteral(ExportName)});
  HOSTAPI_CLASS_LIST(HOSTAPI_ADD_CLASS)
#undef HOSTAPI_ADD_CLASS

  for (const auto &desc : descriptors) {
    if (!isClassExposed(desc.meta)) {
      warnings.append(QStringLiteral("Class %1 is missing HOSTAPI_EXPOSE")
                          .arg(QString::fromLatin1(desc.meta->className())));
    }
    const QString exportName = classExportName(desc.meta, desc.exportName);
    classes.append(buildClassInfo(desc.meta, exportName, &warnings));
  }

  for (const auto &warning : warnings) {
    QTextStream(stderr) << "HostApiGenerator: " << warning << "\n";
  }

  QJsonObject schema;
  schema.insert(QStringLiteral("version"), hostApiVersion());
  schema.insert(QStringLiteral("eventTypes"), QJsonArray::fromStringList(hostApiEventTypes()));
  QJsonArray objectsArray;
  for (const auto &info : classes) {
    objectsArray.append(toJson(info));
  }
  schema.insert(QStringLiteral("objects"), objectsArray);

  const QString headerPath = out.filePath("HostApiGenerated.h");
  const QString sourcePath = out.filePath("HostApiGenerated.cpp");
  const QString schemaPath = out.filePath("hostapi_schema.json");
  const QString dtsPath = out.filePath("hostapi.d.ts");
  const QString npmDtsPath = QDir(npmDir).filePath("hostapi.d.ts");
  const QString packageJsonPath = QDir(npmDir).filePath("package.json");

  if (!writeFile(headerPath, generateCppHeader())) {
    QTextStream(stderr) << "Failed to write " << headerPath << "\n";
    return 1;
  }

  if (!writeFile(sourcePath, generateCppSource(classes, schema))) {
    QTextStream(stderr) << "Failed to write " << sourcePath << "\n";
    return 1;
  }

  const QString schemaJson = QString::fromUtf8(QJsonDocument(schema).toJson(QJsonDocument::Indented));
  if (!writeFile(schemaPath, schemaJson)) {
    QTextStream(stderr) << "Failed to write " << schemaPath << "\n";
    return 1;
  }

  const QString dts = generateDts(classes);
  if (!writeFile(dtsPath, dts)) {
    QTextStream(stderr) << "Failed to write " << dtsPath << "\n";
    return 1;
  }
  if (!writeFile(npmDtsPath, dts)) {
    QTextStream(stderr) << "Failed to write " << npmDtsPath << "\n";
    return 1;
  }

  for (const auto &info : classes) {
    const QString fileName = toPascalCase(info.name) + "Service.ts";
    const QString angularPath = QDir(angularDir).filePath(fileName);
    const QString npmAngularPath = QDir(npmAngularDir).filePath(fileName);
    const QString content = generateAngularService(info);
    if (!writeFile(angularPath, content)) {
      QTextStream(stderr) << "Failed to write " << angularPath << "\n";
      return 1;
    }
    if (!writeFile(npmAngularPath, content)) {
      QTextStream(stderr) << "Failed to write " << npmAngularPath << "\n";
      return 1;
    }
  }

  if (!writeFile(packageJsonPath, generatePackageJson(packageName))) {
    QTextStream(stderr) << "Failed to write " << packageJsonPath << "\n";
    return 1;
  }

  return 0;
}
