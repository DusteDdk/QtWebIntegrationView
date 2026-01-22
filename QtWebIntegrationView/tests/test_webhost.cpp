#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QWebEnginePage>
#include <QWebEngineView>

#include "WebHost/WebHost.h"
#include "HostApiVersion.h"

namespace {

QVariant runJavaScriptSync(QWebEnginePage *page, const QString &script) {
  QVariant result;
  QEventLoop loop;
  page->runJavaScript(script, [&](const QVariant &value) {
    result = value;
    loop.quit();
  });
  loop.exec();
  return result;
}

bool waitForLoad(QWebEngineView *view, int timeoutMs) {
  if (!view->page()->isLoading()) {
    return true;
  }

  QSignalSpy spy(view, &QWebEngineView::loadFinished);
  return spy.wait(timeoutMs);
}

bool waitForHostApi(QWebEnginePage *page, int timeoutMs) {
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < timeoutMs) {
    QVariant ready = runJavaScriptSync(page, "typeof window.HostApi !== 'undefined'");
    if (ready.toBool()) {
      return true;
    }
    QTest::qWait(50);
  }
  return false;
}

} // namespace

class WebHostTests : public QObject {
  Q_OBJECT

private slots:
  void testAddRemoveListeners();
  void testRemoveInvalidEventType();
  void testHostApiVersion();
  void testExampleApi();
};

void WebHostTests::testAddRemoveListeners() {
  WebHost host;
  host.show();

  auto *view = host.findChild<QWebEngineView *>();
  QVERIFY(view != nullptr);
  QVERIFY(waitForLoad(view, 10000));
  QVERIFY(waitForHostApi(view->page(), 5000));

  runJavaScriptSync(view->page(),
                    "window.__testCount = 0;"
                    "window.__handler = function(payload) { window.__testCount++; };"
                    "window.HostApi.addEventListener('actionOne', window.__handler);");

  QJsonObject payload;
  payload.insert("value", 42);
  host.slotTriggerEvent("actionOne", payload);
  QTest::qWait(100);

  QVariant count = runJavaScriptSync(view->page(), "window.__testCount;");
  QCOMPARE(count.toInt(), 1);

  runJavaScriptSync(view->page(),
                    "window.HostApi.removeEventListener('actionOne', window.__handler);");

  host.slotTriggerEvent("actionOne", payload);
  QTest::qWait(100);

  count = runJavaScriptSync(view->page(), "window.__testCount;");
  QCOMPARE(count.toInt(), 1);
}

void WebHostTests::testRemoveInvalidEventType() {
  WebHost host;
  host.show();

  auto *view = host.findChild<QWebEngineView *>();
  QVERIFY(view != nullptr);
  QVERIFY(waitForLoad(view, 10000));
  QVERIFY(waitForHostApi(view->page(), 5000));

  QVariant result = runJavaScriptSync(
      view->page(),
      "try { window.HostApi.removeEventListener('bogus', function() {}); "
      "  'no_error'; } catch (e) { e.message; }");

  QCOMPARE(result.toString(), QStringLiteral("eventType bogus not found."));
}

void WebHostTests::testHostApiVersion() {
  WebHost host;
  host.show();

  auto *view = host.findChild<QWebEngineView *>();
  QVERIFY(view != nullptr);
  QVERIFY(waitForLoad(view, 10000));
  QVERIFY(waitForHostApi(view->page(), 5000));

  QVariant version = runJavaScriptSync(view->page(), "window.HostApi.version;");
  QCOMPARE(version.toString(), hostApiVersion());
}

void WebHostTests::testExampleApi() {
  WebHost host;
  host.show();

  auto *view = host.findChild<QWebEngineView *>();
  QVERIFY(view != nullptr);
  QVERIFY(waitForLoad(view, 10000));
  QVERIFY(waitForHostApi(view->page(), 5000));

  runJavaScriptSync(view->page(),
                    "window.__exampleValue = null;"
                    "window.HostApi.example.echo('ping').then(function(value) {"
                    "  window.__exampleValue = value;"
                    "});");

  QTest::qWait(200);
  QVariant echoValue = runJavaScriptSync(view->page(), "window.__exampleValue;");
  QCOMPARE(echoValue.toString(), QStringLiteral("ping"));

  runJavaScriptSync(view->page(),
                    "window.__statusCount = 0;"
                    "window.__statusValue = null;"
                    "window.HostApi.example.registerEventHandler('statusChanged', function(status) {"
                    "  window.__statusCount += 1;"
                    "  window.__statusValue = status;"
                    "});"
                    "window.HostApi.example.setStatus('ready');");

  QTest::qWait(200);
  QVariant statusCount = runJavaScriptSync(view->page(), "window.__statusCount;");
  QVariant statusValue = runJavaScriptSync(view->page(), "window.__statusValue;");
  QCOMPARE(statusCount.toInt(), 1);
  QCOMPARE(statusValue.toString(), QStringLiteral("ready"));
}

int main(int argc, char **argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
          "--no-sandbox --disable-setuid-sandbox --disable-gpu --headless "
          "--disable-software-rasterizer --disable-dev-shm-usage");
  qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1");
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  WebHost::registerUrlScheme();

  QApplication app(argc, argv);
  WebHostTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_webhost.moc"
