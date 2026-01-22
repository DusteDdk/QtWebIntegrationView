#include "ExampleApi.h"

ExampleApi::ExampleApi(QObject *parent) : QObject(parent) {}

QString ExampleApi::echo(const QString &text) {
  return text;
}

int ExampleApi::add(int a, int b) {
  return a + b;
}

void ExampleApi::setStatus(const QString &status) {
  if (status == m_status) {
    return;
  }
  m_status = status;
  emit statusChanged(m_status);
}
