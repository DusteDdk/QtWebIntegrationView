#include "WebHost/WebHost.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QResource>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlScheme>
#include <QWebEngineView>
#include <QUuid>

#include "HostApiEventTypes.h"
#include "HostApiGenerated.h"
#include "HostApiVersion.h"

static void ensureWebResourcesRegistered() {
  static bool registered = false;
  if (registered) {
    return;
  }
  Q_INIT_RESOURCE(web);
  registered = true;
}

namespace {

class WebRootInterceptor : public QWebEngineUrlRequestInterceptor {
public:
  explicit WebRootInterceptor(const QString &rootDir, QObject *parent = nullptr)
      : QWebEngineUrlRequestInterceptor(parent) {
    setRoot(rootDir);
  }

  void setRootDir(const QString &rootDir) {
    setRoot(rootDir);
  }

  void interceptRequest(QWebEngineUrlRequestInfo &info) override {
    const QUrl url = info.requestUrl();
    if (!url.isLocalFile()) {
      return;
    }

    const QString localPath = QDir::cleanPath(url.toLocalFile());
    QFileInfo fileInfo(localPath);
    const QString absolutePath = fileInfo.absoluteFilePath();
    const QString canonicalPath = fileInfo.exists() ? fileInfo.canonicalFilePath() : QString();
    const QString resolvedPath = canonicalPath.isEmpty() ? absolutePath : canonicalPath;

    if (resolvedPath.isEmpty() || !resolvedPath.startsWith(m_rootAbsolute)) {
      info.block(true);
      qWarning() << "WebHost blocked file request:" << url << "resolved to" << resolvedPath;
      return;
    }

    qInfo() << "WebHost file request:" << url;
  }

private:
  void setRoot(const QString &rootDir) {
    QString absolute = QDir(rootDir).absolutePath();
    if (!absolute.endsWith(QDir::separator())) {
      absolute += QDir::separator();
    }
    m_rootAbsolute = absolute;
  }

  QString m_rootAbsolute;
};

class WebHostPage : public QWebEnginePage {
public:
  explicit WebHostPage(QWebEngineProfile *profile, QObject *parent = nullptr)
      : QWebEnginePage(profile, parent) {}

protected:
  void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message,
                                int lineNumber, const QString &sourceId) override {
    const char *levelText = "info";
    switch (level) {
      case QWebEnginePage::JavaScriptConsoleMessageLevel::ErrorMessageLevel:
        levelText = "error";
        break;
      case QWebEnginePage::JavaScriptConsoleMessageLevel::WarningMessageLevel:
        levelText = "warn";
        break;
      default:
        break;
    }
    qInfo() << "WebHost JS console:" << levelText << message << "line" << lineNumber
            << "source" << sourceId;
  }
};

} // namespace

class HostBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY(QStringList validEventTypes READ validEventTypes CONSTANT)
  Q_PROPERTY(QString hostApiVersion READ hostApiVersion CONSTANT)
  Q_PROPERTY(QJsonObject hostApiSchema READ hostApiSchema CONSTANT)

public:
  explicit HostBridge(const QStringList &validEventTypes, const QString &hostApiVersion,
                      const QJsonObject &hostApiSchema, QObject *parent = nullptr)
      : QObject(parent),
        m_validEventTypes(validEventTypes),
        m_hostApiVersion(hostApiVersion),
        m_hostApiSchema(hostApiSchema) {}

  QStringList validEventTypes() const { return m_validEventTypes; }
  QString hostApiVersion() const { return m_hostApiVersion; }
  QJsonObject hostApiSchema() const { return m_hostApiSchema; }

  Q_INVOKABLE void sendData(const QVariant &data) {
    emit sendDataRequested(QJsonValue::fromVariant(data));
  }

  Q_INVOKABLE void setOutput(const QString &text) { emit setOutputRequested(text); }

  Q_INVOKABLE QString requestInput() {
    const QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    emit inputRequested(uuid);
    return uuid;
  }

  void notifyInputProvided(const QString &uuid, const QString &input) {
    emit inputProvided(uuid, input);
  }

signals:
  void sendDataRequested(QJsonValue value);
  void setOutputRequested(QString text);
  void inputRequested(QString uuid);
  void inputProvided(QString uuid, QString input);

private:
  QStringList m_validEventTypes;
  QString m_hostApiVersion;
  QJsonObject m_hostApiSchema;
};

namespace {

QString jsonValueToJs(const QJsonValue &value) {
  QJsonArray wrapper;
  if (value.isUndefined()) {
    wrapper.append(QJsonValue::Null);
  } else {
    wrapper.append(value);
  }
  QByteArray json = QJsonDocument(wrapper).toJson(QJsonDocument::Compact);
  QString text = QString::fromUtf8(json);
  if (text.size() >= 2) {
    return text.mid(1, text.size() - 2);
  }
  return QStringLiteral("null");
}

QString colorToCssRgba(const QColor &color) {
  return QStringLiteral("rgba(%1, %2, %3, %4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(QString::number(color.alphaF(), 'f', 3));
}

QString resolveWebRoot(const QString &preferredRoot) {
  QFileInfo preferredInfo(preferredRoot);
  if (preferredInfo.exists() && preferredInfo.isDir()) {
    return QDir(preferredInfo.absoluteFilePath()).absolutePath();
  }

  const QString appDir = QCoreApplication::applicationDirPath();
  if (!appDir.isEmpty()) {
    const QString fallback = QDir(appDir).filePath("web");
    QFileInfo fallbackInfo(fallback);
    if (fallbackInfo.exists() && fallbackInfo.isDir()) {
      return QDir(fallbackInfo.absoluteFilePath()).absolutePath();
    }
  }

  return QDir(preferredRoot).absolutePath();
}

} // namespace

WebHost::WebHost(QWidget *parent) : WebHost(QDir::current().filePath("web"), parent) {}

WebHost::WebHost(const QString &webRoot, QWidget *parent) : QWidget(parent) {
  qRegisterMetaType<QJsonValue>("QJsonValue");
  qRegisterMetaType<QJsonObject>("QJsonObject");
  qRegisterMetaType<QJsonArray>("QJsonArray");
  initialize(webRoot);
}

void WebHost::registerUrlScheme() {
  static bool registered = false;
  if (registered) {
    return;
  }

  QWebEngineUrlScheme scheme("webhost");
  scheme.setFlags(QWebEngineUrlScheme::LocalScheme | QWebEngineUrlScheme::LocalAccessAllowed |
                  QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::ViewSourceAllowed);
  scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
  QWebEngineUrlScheme::registerScheme(scheme);
  registered = true;
}

QStringList WebHost::validEventTypes() const {
  return m_validEventTypes;
}

void WebHost::setRootDir(const QString &webRoot) {
  m_rootMode = RootMode::Directory;
  m_webRoot = resolveWebRoot(webRoot);
  if (m_interceptor) {
    auto *interceptor = static_cast<WebRootInterceptor *>(m_interceptor);
    interceptor->setRootDir(m_webRoot);
  }
  if (m_page) {
    m_page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    m_page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
  }
  loadRoot();
}

void WebHost::setRootQrc() {
  m_rootMode = RootMode::Qrc;
  if (m_qrcRoot.isEmpty()) {
    m_qrcRoot = QStringLiteral("qrc:/web");
  }
  ensureWebResourcesRegistered();
  if (m_page) {
    m_page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    m_page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
  }
  loadRoot();
}

void WebHost::slotProvideInput(QString uuid, QString input) {
  if (m_bridge) {
    m_bridge->notifyInputProvided(uuid, input);
  }
}

void WebHost::slotTriggerEvent(QString actionId, QJsonValue payload) {
  if (!m_page) {
    return;
  }

  const QString actionIdJs = jsonValueToJs(QJsonValue(actionId));
  const QString payloadJs = jsonValueToJs(payload);
  const QString script = QStringLiteral(
      "if (window.HostApi && window.HostApi.__dispatchEvent) { "
      "window.HostApi.__dispatchEvent(%1, %2); "
      "}")
                              .arg(actionIdJs, payloadJs);
  m_page->runJavaScript(script);
}

void WebHost::applyWindowBackground() {
  if (!m_page) {
    return;
  }

  QWidget *topLevel = window();
  const QPalette palette = topLevel ? topLevel->palette() : QApplication::palette();
  const QColor color = palette.color(QPalette::Window);
  const QString rgba = colorToCssRgba(color);
  const QString script = QStringLiteral(
      "document.documentElement.style.setProperty('--host-window-color', '%1');"
      "if (document.body) { document.body.style.background = '%1'; }")
                              .arg(rgba);
  m_page->runJavaScript(script);
}

void WebHost::initialize(const QString &webRoot) {
  m_validEventTypes = hostApiEventTypes();
#ifdef WEBHOST_DEFAULT_QRC
  m_rootMode = RootMode::Qrc;
#endif
  m_webRoot = resolveWebRoot(webRoot);
  m_qrcRoot = QStringLiteral("qrc:/web");

  if (m_rootMode == RootMode::Qrc) {
    ensureWebResourcesRegistered();
  }

  qInfo() << "WebHost resolved web root:" << m_webRoot;

  m_view = new QWebEngineView(this);
  m_profile = new QWebEngineProfile(this);
  const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (!appDataPath.isEmpty()) {
    m_profile->setPersistentStoragePath(appDataPath);
    m_profile->setCachePath(QDir(appDataPath).filePath("cache"));
  }

  m_interceptor = new WebRootInterceptor(m_webRoot, m_profile);
  m_profile->setUrlRequestInterceptor(m_interceptor);

  m_page = new WebHostPage(m_profile, this);
  m_page->settings()->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);
  m_page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,
                                   m_rootMode == RootMode::Directory);
  m_page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
  m_channel = new QWebChannel(this);

  registerHostApiObjects(m_channel, this);
  m_bridge = new HostBridge(m_validEventTypes, hostApiVersion(), hostApiSchema(), this);

  m_page->setWebChannel(m_channel);
  m_channel->registerObject("HostBridge", m_bridge);

  connect(m_bridge, &HostBridge::sendDataRequested, this, &WebHost::signalSendData);
  connect(m_bridge, &HostBridge::setOutputRequested, this, &WebHost::signalSetOutput);
  connect(m_bridge, &HostBridge::inputRequested, this, &WebHost::signalGetInput);
  connect(m_page, &QWebEnginePage::loadStarted, this,
          [this]() { qInfo() << "WebHost load started:" << m_page->url(); });
  connect(m_page, &QWebEnginePage::loadFinished, this, [this](bool ok) {
    qInfo() << "WebHost load finished:" << ok << "url:" << m_page->url();
    applyWindowBackground();
    if (ok) {
      injectHostApiBootstrap();
    }
  });

  m_view->setPage(m_page);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_view);

  loadRoot();
}

void WebHost::loadRoot() {
  if (!m_page) {
    return;
  }

  if (m_rootMode == RootMode::Qrc) {
    const QString qrcRoot = m_qrcRoot.isEmpty() ? QStringLiteral("qrc:/web") : m_qrcRoot;
    const QString urlText = qrcRoot.endsWith('/') ? qrcRoot + "index.html"
                                                  : qrcRoot + "/index.html";
    m_page->setUrl(QUrl(urlText));
    return;
  }

  QFileInfo indexInfo(QDir(m_webRoot).filePath("index.html"));
  if (!indexInfo.exists()) {
    qWarning() << "WebHost index.html not found in" << m_webRoot
               << "current dir:" << QDir::currentPath();
  }

  m_page->setUrl(QUrl::fromLocalFile(indexInfo.absoluteFilePath()));
}

void WebHost::injectHostApiBootstrap() {
  if (!m_page) {
    return;
  }

  const QString script = QStringLiteral(R"JS(
(function () {

  function logError(message) {
    try {
      console.error(message);
    } catch (e) {
      // ignore
    }
  }

  function safeParseSchema(rawSchema) {
    if (!rawSchema) {
      return {};
    }
    if (typeof rawSchema === "string") {
      try {
        return JSON.parse(rawSchema);
      } catch (e) {
        logError("Failed to parse HostApi schema.");
        return {};
      }
    }
    return rawSchema;
  }

  function parseMajor(version) {
    if (!version || typeof version !== "string") {
      return null;
    }
    var parts = version.split(".");
    var major = parseInt(parts[0], 10);
    return Number.isNaN(major) ? null : major;
  }

  function isCompatible(expected, actual) {
    if (!expected || !actual) {
      return true;
    }
    if (expected === actual) {
      return true;
    }
    var expectedMajor = parseMajor(expected);
    var actualMajor = parseMajor(actual);
    return expectedMajor !== null && expectedMajor === actualMajor;
  }

  function dispatchCustomEvent(name, detail) {
    try {
      window.dispatchEvent(new CustomEvent(name, { detail: detail }));
    } catch (e) {
      var fallback = document.createEvent("Event");
      fallback.initEvent(name, false, false);
      fallback.detail = detail;
      window.dispatchEvent(fallback);
    }
  }

  if (window.HostApi && window.HostApi.__ready) {
    dispatchCustomEvent("HostApiReady", {
      version: window.HostApi.version,
      schema: window.HostApi.schema
    });
    return;
  }

  function wrapObject(rawObject, objectInfo) {
    var wrapped = {};
    var methods = objectInfo.methods || [];
    var signals = (objectInfo.signals || []).map(function (entry) {
      return entry && entry.name ? entry.name : entry;
    });

    methods.forEach(function (methodInfo) {
      if (!methodInfo || !methodInfo.name || typeof rawObject[methodInfo.name] !== "function") {
        return;
      }
      wrapped[methodInfo.name] = function () {
        var args = Array.prototype.slice.call(arguments);
        if (methodInfo.returnsVoid) {
          rawObject[methodInfo.name].apply(rawObject, args);
          return Promise.resolve();
        }
        return new Promise(function (resolve) {
          args.push(function (result) {
            resolve(result);
          });
          rawObject[methodInfo.name].apply(rawObject, args);
        });
      };
    });

    wrapped.registerEventHandler = function (eventName, handler) {
      if (signals.indexOf(eventName) === -1) {
        throw new Error("eventType " + eventName + " not found.");
      }
      if (rawObject[eventName] && typeof rawObject[eventName].connect === "function") {
        rawObject[eventName].connect(handler);
      }
    };

    wrapped.removeEventHandler = function (eventName, handler) {
      if (signals.indexOf(eventName) === -1) {
        throw new Error("eventType " + eventName + " not found.");
      }
      if (rawObject[eventName] && typeof rawObject[eventName].disconnect === "function") {
        rawObject[eventName].disconnect(handler);
      }
    };

    wrapped.__raw = rawObject;
    return wrapped;
  }

  function buildHostApi(bridge, schema, channel) {
    var listeners = {};
    var pendingInputs = {};
    var bufferedInputs = {};
    var validEventTypes = bridge.validEventTypes || [];
    var hostApiVersion = bridge.hostApiVersion || "0.0.0";

    function ensureEventType(eventType) {
      if (validEventTypes.indexOf(eventType) === -1) {
        throw new Error("eventType " + eventType + " not found.");
      }
    }

    function addEventListener(eventType, callback) {
      ensureEventType(eventType);
      if (!listeners[eventType]) {
        listeners[eventType] = [];
      }
      listeners[eventType].push(callback);
    }

    function removeEventListener(eventType, callback) {
      ensureEventType(eventType);
      var list = listeners[eventType] || [];
      var index = list.indexOf(callback);
      if (index !== -1) {
        list.splice(index, 1);
      }
    }

    function dispatchEvent(eventType, payload) {
      var list = listeners[eventType] || [];
      list.forEach(function (cb) {
        try {
          cb(payload);
        } catch (err) {
          console.error(err);
        }
      });
    }

    bridge.inputProvided.connect(function (uuid, input) {
      if (pendingInputs[uuid]) {
        pendingInputs[uuid](input);
        delete pendingInputs[uuid];
        return;
      }
      bufferedInputs[uuid] = input;
    });

    var api = {
      validEventTypes: validEventTypes,
      version: hostApiVersion,
      schema: schema || {},
      sendData: function (payload) {
        bridge.sendData(payload);
      },
      setOutput: function (text) {
        bridge.setOutput(text);
      },
      getInput: function () {
        return new Promise(function (resolve) {
          bridge.requestInput(function (uuid) {
            if (Object.prototype.hasOwnProperty.call(bufferedInputs, uuid)) {
              var input = bufferedInputs[uuid];
              delete bufferedInputs[uuid];
              resolve(input);
              return;
            }
            pendingInputs[uuid] = resolve;
          });
        });
      },
      addEventListener: addEventListener,
      removeEventListener: removeEventListener,
      __dispatchEvent: dispatchEvent,
      __ready: true
    };

    var objects = (schema && schema.objects) ? schema.objects : [];
    objects.forEach(function (objectInfo) {
      if (!objectInfo || !objectInfo.name) {
        return;
      }
      var channelName = "HostApi_" + objectInfo.name;
      var rawObject = channel.objects[channelName];
      if (!rawObject) {
        logError("HostApi object missing: " + channelName);
        return;
      }
      api[objectInfo.name] = wrapObject(rawObject, objectInfo);
    });

    return api;
  }

  function init() {
    if (typeof qt === "undefined" || !qt.webChannelTransport) {
      logError("Qt WebChannel bridge not available.");
      return;
    }
    new QWebChannel(qt.webChannelTransport, function (channel) {
      var bridge = channel.objects.HostBridge;
      if (!bridge) {
        logError("HostBridge not available.");
        return;
      }
      var schema = safeParseSchema(bridge.hostApiSchema);
      var hostApi = buildHostApi(bridge, schema, channel);
      window.HostApi = hostApi;

      var expected = window.HostApiExpectedVersion || window.__HOSTAPI_EXPECTED_VERSION;
      if (expected && !isCompatible(expected, hostApi.version)) {
        logError("HostApi version mismatch. expected=" + expected + " actual=" + hostApi.version);
        dispatchCustomEvent("HostApiVersionMismatch", {
          expected: expected,
          actual: hostApi.version
        });
      }

      dispatchCustomEvent("HostApiReady", {
        version: hostApi.version,
        schema: schema
      });
    });
  }

  function ensureWebChannel(ready) {
    if (typeof QWebChannel !== "undefined") {
      ready();
      return;
    }
    var script = document.createElement("script");
    script.src = "qrc:///qtwebchannel/qwebchannel.js";
    script.onload = function () {
      ready();
    };
    script.onerror = function () {
      logError("Failed to load qwebchannel.js");
    };
    (document.head || document.documentElement || document.body).appendChild(script);
  }

  ensureWebChannel(init);
})();
)JS");

  m_page->runJavaScript(script);
}

#include "WebHost.moc"
