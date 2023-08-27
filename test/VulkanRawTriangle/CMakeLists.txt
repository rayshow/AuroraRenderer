#project(VulkanRawTriangle)
add_executable(DrawRawTriangle DrawRawTriangle.cpp)
target_include_directories(DrawRawTriangle PUBLIC ${PROJECT_SOURCE_DIR}/third-party/Vulkan/Include)