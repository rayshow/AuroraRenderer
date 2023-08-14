project(App)
add_executable(${PROJECT_NAME} window/main.cpp)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC 3rd::staticlib 3rd::dynamiclib)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE_DIR:3rd::dynamiclib>/$<TARGET_FILE_NAME:3rd::dynamiclib>"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
