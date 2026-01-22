add_executable(QtWebIntegrationView
  main.cpp
  MainWindow.cpp
  MainWindow.h
)

target_link_libraries(QtWebIntegrationView
  PRIVATE
    Qt6::Widgets
    WebHost
)
