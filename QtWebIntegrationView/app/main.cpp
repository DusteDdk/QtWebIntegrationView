#include <QApplication>
#include <QByteArray>

#include "MainWindow.h"
#include "WebHost/WebHost.h"

namespace {

void configureSoftwareRendering() {
  if (!qEnvironmentVariableIsSet("WEBHOST_FORCE_SOFTWARE")) {
    return;
  }

  qInfo() << "WebHost forcing software rendering. Unset WEBHOST_FORCE_SOFTWARE to use GPU.";
  QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);

  if (!qEnvironmentVariableIsSet("QSG_RHI_BACKEND")) {
    qputenv("QSG_RHI_BACKEND", "software");
  }
  if (!qEnvironmentVariableIsSet("QT_QUICK_BACKEND")) {
    qputenv("QT_QUICK_BACKEND", "software");
  }

  QByteArray flags = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
  if (!flags.contains("--use-gl=")) {
    flags += " --use-gl=swiftshader";
  }
  if (!flags.contains("--disable-gpu")) {
    flags += " --disable-gpu";
  }
  if (!flags.contains("--disable-gpu-compositing")) {
    flags += " --disable-gpu-compositing";
  }
  qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags.trimmed());
}

} // namespace

int main(int argc, char *argv[]) {
  configureSoftwareRendering();
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  WebHost::registerUrlScheme();

  QApplication app(argc, argv);

  MainWindow window;
  window.show();

  return app.exec();
}
