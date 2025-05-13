set(ModuleName "DrawRawTriangle")



project(${ModuleName})
add_executable(${ModuleName} ${BUILD_RUNNING_TYPE} DrawRawTriangle.cpp)
target_include_directories(${ModuleName} PUBLIC ${CMAKE_SOURCE_DIR}/third-party/Vulkan/Include)
target_include_directories(${ModuleName} PUBLIC ${CMAKE_SOURCE_DIR}/third-party/D3D12/directx)
target_include_directories(${ModuleName} PUBLIC ${PROJECT_SOURCE_DIR}/src)


# CMAKE_SOURCE_DIR    root dir 
# PROJECT_SOURCE_DIR  project() dir 
# CMAKE_CURRENT_SOURCE_DIR  current CMakeLists.txt dir
string(REPLACE "\\" "/" PROJECT_SOURCE_DIR_ESCAPED "${PROJECT_SOURCE_DIR}")
target_compile_definitions(${ModuleName} PRIVATE
    "PROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR_ESCAPED}\""
)

target_link_libraries(${ModuleName} LINK_PUBLIC SRC)
set_cpp_options(${ModuleName} 20 TRUE)
