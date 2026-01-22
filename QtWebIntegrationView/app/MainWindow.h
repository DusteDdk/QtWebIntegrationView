#pragma once

#include <QJsonValue>
#include <QMainWindow>

class QLabel;
class QCheckBox;
class QLineEdit;
class QPushButton;
class WebHost;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

private:
  void handleSendData(const QJsonValue &value);
  void updateProvideButtonState();
  void providePendingInput();

  WebHost *m_webHost = nullptr;
  QLabel *m_outputLabel = nullptr;
  QLineEdit *m_input = nullptr;
  QPushButton *m_btnA = nullptr;
  QPushButton *m_btnB = nullptr;
  QPushButton *m_btnProvide = nullptr;
  QCheckBox *m_autoProvide = nullptr;
  QString m_pendingInputUuid;
};
