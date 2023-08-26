project(VulkanRawTriangle)
add_executable(${PROJECT_NAME} DrawRawTriangle.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/third-party/Vulkan/Include)