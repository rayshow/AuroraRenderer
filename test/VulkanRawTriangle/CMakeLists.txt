set(ModuleName "DrawRawTriangle")


add_executable(${ModuleName} ${BUILD_RUNNING_TYPE} DrawRawTriangle.cpp)
target_include_directories(${ModuleName} PUBLIC ${PROJECT_SOURCE_DIR}/third-party/Vulkan/Include)
target_include_directories(${ModuleName} PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${ModuleName} LINK_PUBLIC SRC)
set_cpp_options(${ModuleName} 20 TRUE)
