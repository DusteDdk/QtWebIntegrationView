#pragma once

#include <QJsonValue>
#include <QStringList>
#include <QWidget>

class QWebChannel;
class QWebEnginePage;
class QWebEngineProfile;
class QWebEngineUrlRequestInterceptor;
class QWebEngineView;

class HostBridge;

class WebHost : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QStringList validEventTypes READ validEventTypes CONSTANT)

public:
  explicit WebHost(QWidget *parent = nullptr);
  explicit WebHost(const QString &webRoot, QWidget *parent = nullptr);
  ~WebHost() override = default;

  static void registerUrlScheme();

  void setRootDir(const QString &webRoot);
  void setRootQrc();

  QStringList validEventTypes() const;

signals:
  void signalSendData(QJsonValue value);
  void signalSetOutput(QString output);
  void signalGetInput(QString uuid);

public slots:
  void slotProvideInput(QString uuid, QString input);
  void slotTriggerEvent(QString actionId, QJsonValue payload = QJsonValue::Null);

private:
  enum class RootMode { Directory, Qrc };

  void initialize(const QString &webRoot);
  void applyWindowBackground();
  void injectHostApiBootstrap();
  void loadRoot();

  QWebEngineView *m_view = nullptr;
  QWebEngineProfile *m_profile = nullptr;
  QWebEnginePage *m_page = nullptr;
  QWebEngineUrlRequestInterceptor *m_interceptor = nullptr;
  QWebChannel *m_channel = nullptr;
  HostBridge *m_bridge = nullptr;
  QString m_webRoot;
  QString m_qrcRoot;
  RootMode m_rootMode = RootMode::Directory;
  QStringList m_validEventTypes;
};
