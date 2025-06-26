set(ModuleName "DrawRawTriangle")

add_library(ShaderConductorDyn SHARED IMPORTED)
set(ShaderConductorDynPath ${CMAKE_SOURCE_DIR}/third-party/binaries/ShaderConductor/Bin/Debug/ShaderConductor.dll)
set(ShaderConductorLibPath ${CMAKE_SOURCE_DIR}/third-party/binaries/ShaderConductor/Lib/Debug/ShaderConductor.lib)
#target_link_libraries(${ModuleName} PRIVATE )
set_target_properties(ShaderConductorDyn PROPERTIES IMPORTED_LOCATION_DEBUG ${ShaderConductorDynPath})
set_target_properties(ShaderConductorDyn PROPERTIES IMPORTED_LOCATION_RELEASE ${ShaderConductorDynPath})
set_target_properties(ShaderConductorDyn PROPERTIES IMPORTED_IMPLIB_DEBUG ${ShaderConductorLibPath})
set_target_properties(ShaderConductorDyn PROPERTIES IMPORTED_IMPLIB_RELEASE ${ShaderConductorLibPath})

message(STATUS "ShaderConductorDynPath: ${ShaderConductorDynPath} ")

project(${ModuleName})
add_executable(${ModuleName} ${BUILD_RUNNING_TYPE} DrawRawTriangle.cpp)
target_include_directories(${ModuleName} PUBLIC ${CMAKE_SOURCE_DIR}/third-party/Vulkan/Include)
target_include_directories(${ModuleName} PUBLIC ${CMAKE_SOURCE_DIR}/third-party/D3D12/directx)
target_include_directories(${ModuleName} PUBLIC ${CMAKE_SOURCE_DIR}/third-party/binaries/ShaderConductor/Include)
target_include_directories(${ModuleName} PUBLIC ${PROJECT_SOURCE_DIR}/src/runtime)
target_include_directories(${ModuleName} PUBLIC ${PROJECT_SOURCE_DIR}/src/developer/shader_compiler)


# CMAKE_SOURCE_DIR    root dir 
# PROJECT_SOURCE_DIR  project() dir 
# CMAKE_CURRENT_SOURCE_DIR  current CMakeLists.txt dir
string(REPLACE "/" "/" PROJECT_SOURCE_DIR_ESCAPED "${PROJECT_SOURCE_DIR}")
target_compile_definitions(${ModuleName} PRIVATE
    "WIDE_PROJECT_SOURCE_DIR=L\"${PROJECT_SOURCE_DIR_ESCAPED}\"" 
    "PROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR_ESCAPED}\""
)

target_link_libraries(${ModuleName} LINK_PUBLIC SRC ShaderConductorDyn shader_compiler digest)

add_custom_command ( TARGET ${ModuleName} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:ShaderConductorDyn> $<TARGET_FILE_DIR:${ModuleName}>
)


set_cpp_options(${ModuleName} 20 TRUE)
