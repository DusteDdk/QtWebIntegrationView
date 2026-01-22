add_executable(WebHostTests
  test_webhost.cpp
)

target_link_libraries(WebHostTests
  PRIVATE
    Qt6::Test
    Qt6::Widgets
    WebHost
)

set_target_properties(WebHostTests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

add_test(NAME WebHostTests COMMAND WebHostTests)
set_tests_properties(WebHostTests PROPERTIES
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
