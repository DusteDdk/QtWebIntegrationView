#include "MainWindow.h"

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#include "WebHost/WebHost.h"

namespace {

QString jsonValueToString(const QJsonValue &value) {
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

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Qt WebHost Integration View");

  auto *central = new QWidget(this);
  auto *layout = new QVBoxLayout(central);
  layout->setContentsMargins(12, 12, 12, 12);

  m_webHost = new WebHost(central);
  m_webHost->setObjectName("WebHost");
#ifdef WEBHOST_DEFAULT_QRC
  m_webHost->setRootQrc();
#endif

  m_outputLabel = new QLabel("Output: (empty)", central);
  m_outputLabel->setObjectName("lbl_output");

  m_input = new QLineEdit(central);
  m_input->setObjectName("le_input");
  m_input->setPlaceholderText("Type input to return to HostApi.getInput()");

  m_autoProvide = new QCheckBox("Provide input immediately", central);
  m_autoProvide->setChecked(true);

  m_btnProvide = new QPushButton("Provide Input (no request)", central);
  m_btnProvide->setEnabled(false);

  m_btnA = new QPushButton("Action One", central);
  m_btnA->setObjectName("btn_a");

  m_btnB = new QPushButton("Action Two", central);
  m_btnB->setObjectName("btn_b");

  auto *outputRow = new QHBoxLayout();
  outputRow->addWidget(m_outputLabel, 1);

  auto *inputRow = new QHBoxLayout();
  inputRow->addWidget(m_input, 1);

  auto *buttonRow = new QHBoxLayout();
  buttonRow->addWidget(m_btnProvide);
  buttonRow->addWidget(m_autoProvide);
  buttonRow->addWidget(m_btnA);
  buttonRow->addWidget(m_btnB);
  buttonRow->addStretch();

  layout->addWidget(m_webHost, 1);
  layout->addLayout(outputRow);
  layout->addLayout(inputRow);
  layout->addLayout(buttonRow);

  setCentralWidget(central);

  connect(m_webHost, &WebHost::signalSendData, this, &MainWindow::handleSendData);
  connect(m_webHost, &WebHost::signalSetOutput, this,
          [this](const QString &text) { m_outputLabel->setText(text); });
  connect(m_webHost, &WebHost::signalGetInput, this, [this](const QString &uuid) {
    m_pendingInputUuid = uuid;
    if (m_autoProvide->isChecked()) {
      providePendingInput();
    } else {
      updateProvideButtonState();
    }
  });

  connect(m_btnProvide, &QPushButton::clicked, this, [this]() { providePendingInput(); });
  connect(m_autoProvide, &QCheckBox::toggled, this, [this](bool checked) {
    if (checked && !m_pendingInputUuid.isEmpty()) {
      providePendingInput();
      return;
    }
    updateProvideButtonState();
  });

  connect(m_btnA, &QPushButton::clicked, this,
          [this]() { m_webHost->slotTriggerEvent("actionOne"); });
  connect(m_btnB, &QPushButton::clicked, this,
          [this]() { m_webHost->slotTriggerEvent("actionTwo"); });
}

void MainWindow::handleSendData(const QJsonValue &value) {
  statusBar()->showMessage("sendData: " + jsonValueToString(value), 4000);
}

void MainWindow::updateProvideButtonState() {
  if (m_pendingInputUuid.isEmpty()) {
    m_btnProvide->setText("Provide Input (no request)");
    m_btnProvide->setEnabled(false);
    return;
  }

  m_btnProvide->setText(QString("Provide Input for %1").arg(m_pendingInputUuid));
  m_btnProvide->setEnabled(!m_autoProvide->isChecked());
}

void MainWindow::providePendingInput() {
  if (m_pendingInputUuid.isEmpty()) {
    updateProvideButtonState();
    return;
  }

  m_webHost->slotProvideInput(m_pendingInputUuid, m_input->text());
  m_pendingInputUuid.clear();
  updateProvideButtonState();
}
