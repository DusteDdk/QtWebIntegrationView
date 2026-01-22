#pragma once

#include <QObject>
#include <QString>

#include "HostApiMacros.h"

class ExampleApi : public QObject {
  Q_OBJECT
  HOSTAPI_EXPOSE
  HOSTAPI_NAME("example")

public:
  explicit ExampleApi(QObject *parent = nullptr);

  Q_INVOKABLE QString echo(const QString &text);
  Q_INVOKABLE int add(int a, int b);

public slots:
  void setStatus(const QString &status);

signals:
  void statusChanged(QString status);

private:
  QString m_status;
};
