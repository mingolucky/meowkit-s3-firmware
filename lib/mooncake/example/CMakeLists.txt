add_executable(basic_demo ./basic_demo.cpp)
target_link_libraries(basic_demo ${PROJECT_NAME})

add_executable(multiple_apps_demo ./multiple_apps_demo.cpp)
target_link_libraries(multiple_apps_demo ${PROJECT_NAME})

add_executable(extension_test ./extension_test.cpp)
target_link_libraries(extension_test ${PROJECT_NAME})

add_executable(ability_manager_test ./ability_manager_test.cpp)
target_link_libraries(ability_manager_test ${PROJECT_NAME})

add_executable(ui_ability_test ./ui_ability_test.cpp)
target_link_libraries(ui_ability_test ${PROJECT_NAME})

add_executable(app_ability_test ./app_ability_test.cpp)
target_link_libraries(app_ability_test ${PROJECT_NAME})

add_executable(singleton_test ./singleton_test.cpp)
target_link_libraries(singleton_test ${PROJECT_NAME})
