set(ModuleName "TestDXC")
project(${ModuleName})
add_executable(${ModuleName} ${BUILD_RUNNING_TYPE} TestDXC.cpp)



target_include_directories(${ModuleName} PUBLIC ${PROJECT_SOURCE_DIR}/src/runtime)
target_include_directories(${ModuleName} PUBLIC ${CMAKE_SOURCE_DIR}/third-party/binaries/dxc/inc)
target_include_directories(${ModuleName} PUBLIC ${CMAKE_SOURCE_DIR}/third-party/CLI11/include)

add_dependencies(${ModuleName} dxc SRC)
target_link_libraries(${ModuleName} LINK_PUBLIC dxc SRC)


set_cpp_options(${ModuleName} 20 FALSE)
