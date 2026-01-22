#pragma once

#include <QString>

constexpr const char kHostApiVersion[] = "0.1.0";

inline QString hostApiVersion() {
  return QString::fromLatin1(kHostApiVersion);
}
