cmake_minimum_required(VERSION 3.21)
project(QtWebIntegrationView VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets WebEngineWidgets WebChannel Test)

enable_testing()

option(WEBHOST_USE_QRC "Use Qt resources for the WebHost root by default." OFF)
option(WEBHOST_COPY_WEB "Copy web assets to runtime directory." ON)

set(WEB_SOURCE_DIR ${CMAKE_SOURCE_DIR}/web)
set(WEB_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/web)

set(WEB_QRC_FILE ${CMAKE_BINARY_DIR}/generated/web.qrc)
file(GLOB_RECURSE WEB_ASSET_FILES CONFIGURE_DEPENDS ${WEB_SOURCE_DIR}/*)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)
execute_process(
  COMMAND ${CMAKE_COMMAND} -DWEB_ROOT=${WEB_SOURCE_DIR} -DOUTPUT=${WEB_QRC_FILE}
          -P ${CMAKE_SOURCE_DIR}/scripts/generate_web_qrc.cmake
)
set_source_files_properties(${WEB_QRC_FILE} PROPERTIES GENERATED TRUE)

add_subdirectory(hostapi)
add_subdirectory(tools/HostApiGenerator)
add_subdirectory(WebHost)
add_subdirectory(app)
add_subdirectory(tests)

set(HOSTAPI_GEN_DIR ${CMAKE_BINARY_DIR}/generated/hostapi)
set(HOSTAPI_GENERATED_CPP ${HOSTAPI_GEN_DIR}/HostApiGenerated.cpp)
set(HOSTAPI_GENERATED_H ${HOSTAPI_GEN_DIR}/HostApiGenerated.h)
set(HOSTAPI_GENERATED_SCHEMA ${HOSTAPI_GEN_DIR}/hostapi_schema.json)
set(HOSTAPI_GENERATED_DTS ${HOSTAPI_GEN_DIR}/hostapi.d.ts)
set(HOSTAPI_GENERATED_NPM_DIR ${HOSTAPI_GEN_DIR}/npm)
set(HOSTAPI_GENERATED_NPM_DTS ${HOSTAPI_GENERATED_NPM_DIR}/hostapi.d.ts)
set(HOSTAPI_GENERATED_NPM_PACKAGE ${HOSTAPI_GENERATED_NPM_DIR}/package.json)
set(HOSTAPI_GENERATED_ANGULAR_DIR ${HOSTAPI_GEN_DIR}/angular)
set(HOSTAPI_GENERATED_NPM_ANGULAR_DIR ${HOSTAPI_GENERATED_NPM_DIR}/angular)
set(HOSTAPI_GENERATED_ANGULAR_SERVICE ${HOSTAPI_GENERATED_ANGULAR_DIR}/ExampleService.ts)
set(HOSTAPI_GENERATED_NPM_ANGULAR_SERVICE ${HOSTAPI_GENERATED_NPM_ANGULAR_DIR}/ExampleService.ts)

set(HOSTAPI_INPUTS
  ${CMAKE_SOURCE_DIR}/hostapi/ExampleApi.cpp
  ${CMAKE_SOURCE_DIR}/hostapi/ExampleApi.h
  ${CMAKE_SOURCE_DIR}/hostapi/HostApiClassList.h
  ${CMAKE_SOURCE_DIR}/hostapi/HostApiClasses.h
  ${CMAKE_SOURCE_DIR}/hostapi/HostApiEventTypes.cpp
  ${CMAKE_SOURCE_DIR}/hostapi/HostApiEventTypes.h
  ${CMAKE_SOURCE_DIR}/hostapi/HostApiMacros.h
  ${CMAKE_SOURCE_DIR}/hostapi/HostApiVersion.h
  ${CMAKE_SOURCE_DIR}/tools/HostApiGenerator/main.cpp
)

add_custom_command(
  OUTPUT
    ${HOSTAPI_GENERATED_CPP}
    ${HOSTAPI_GENERATED_H}
  BYPRODUCTS
    ${HOSTAPI_GENERATED_SCHEMA}
    ${HOSTAPI_GENERATED_DTS}
    ${HOSTAPI_GENERATED_NPM_DTS}
    ${HOSTAPI_GENERATED_NPM_PACKAGE}
    ${HOSTAPI_GENERATED_ANGULAR_SERVICE}
    ${HOSTAPI_GENERATED_NPM_ANGULAR_SERVICE}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${HOSTAPI_GEN_DIR}
  COMMAND $<TARGET_FILE:HostApiGenerator> --output-dir ${HOSTAPI_GEN_DIR}
  DEPENDS ${HOSTAPI_INPUTS} HostApiGenerator
  COMMENT "Generating HostApi glue and TypeScript artifacts"
)

add_custom_target(HostApiCodegen DEPENDS ${HOSTAPI_GENERATED_CPP} ${HOSTAPI_GENERATED_H})

target_sources(WebHost PRIVATE ${HOSTAPI_GENERATED_CPP} ${HOSTAPI_GENERATED_H})
target_include_directories(WebHost PRIVATE ${HOSTAPI_GEN_DIR})
add_dependencies(WebHost HostApiCodegen)
add_dependencies(QtWebIntegrationView HostApiCodegen)
add_dependencies(WebHostTests HostApiCodegen)

if (WEBHOST_USE_QRC)
  target_compile_definitions(WebHost PUBLIC WEBHOST_DEFAULT_QRC)
  target_compile_definitions(QtWebIntegrationView PRIVATE WEBHOST_DEFAULT_QRC)
  target_compile_definitions(WebHostTests PRIVATE WEBHOST_DEFAULT_QRC)
endif()

if (WEBHOST_COPY_WEB)
  add_custom_target(copy_web ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${WEB_SOURCE_DIR} ${WEB_OUTPUT_DIR}
    COMMENT "Copying web assets to runtime directory"
  )

  add_dependencies(QtWebIntegrationView copy_web)
  add_dependencies(WebHostTests copy_web)
endif()
