add_executable(HostApiGenerator
  main.cpp
)

target_link_libraries(HostApiGenerator
  PRIVATE
    Qt6::Core
    HostApiContracts
)

target_include_directories(HostApiGenerator
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
