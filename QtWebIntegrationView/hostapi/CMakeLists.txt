add_library(HostApiContracts STATIC
  ExampleApi.cpp
  ExampleApi.h
  HostApiClassList.h
  HostApiClasses.h
  HostApiEventTypes.cpp
  HostApiEventTypes.h
  HostApiMacros.h
  HostApiVersion.h
)

target_include_directories(HostApiContracts
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HostApiContracts
  PUBLIC
    Qt6::Core
)
