if (NOT WEB_QRC_FILE)
  set(WEB_QRC_FILE ${CMAKE_SOURCE_DIR}/web/web.qrc)
endif()

add_library(WebHost STATIC
  src/WebHost.cpp
  include/WebHost/WebHost.h
  ${WEB_QRC_FILE}
)

target_include_directories(WebHost
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(WebHost
  PUBLIC
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    HostApiContracts
)
