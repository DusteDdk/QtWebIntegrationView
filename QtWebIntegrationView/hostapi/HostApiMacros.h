#pragma once

// Marks a QObject-derived class as eligible for HostApi code generation.
#define HOSTAPI_EXPOSE \
  Q_CLASSINFO("HostApi.Expose", "true")

// Optional explicit name override for HostApi export.
#define HOSTAPI_NAME(name) \
  Q_CLASSINFO("HostApi.Name", name)
