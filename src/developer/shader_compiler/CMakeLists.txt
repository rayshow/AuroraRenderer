set(ModuleName "shader_compiler")

add_library(ShaderConductorDyn SHARED IMPORTED)
set(ShaderConductorDynPath ${CMAKE_SOURCE_DIR}/third-party/binaries/ShaderConductor/Bin/Debug/ShaderConductor.dll)
set(ShaderConductorLibPath ${CMAKE_SOURCE_DIR}/third-party/binaries/ShaderConductor/Lib/Debug/ShaderConductor.lib)
#target_link_libraries(${ModuleName} PRIVATE )
set_target_properties(ShaderConductorDyn PROPERTIES IMPORTED_LOCATION_DEBUG ${ShaderConductorDynPath})
set_target_properties(ShaderConductorDyn PROPERTIES IMPORTED_LOCATION_RELEASE ${ShaderConductorDynPath})
set_target_properties(ShaderConductorDyn PROPERTIES IMPORTED_IMPLIB_DEBUG ${ShaderConductorLibPath})
set_target_properties(ShaderConductorDyn PROPERTIES IMPORTED_IMPLIB_RELEASE ${ShaderConductorLibPath})

# add
file(GLOB_RECURSE HEADERS LIST_DIRECTORIES TRUE  "*.h")
file(GLOB_RECURSE CPP_FILES LIST_DIRECTORIES TRUE "*.cpp")
add_library(${ModuleName} STATIC ${CPP_FILES} ${HEADERS})
target_link_libraries(${ModuleName} LINK_PUBLIC SRC ShaderConductorDyn digest)
target_include_directories(${ModuleName} PUBLIC ${PROJECT_SOURCE_DIR}/src/runtime)
target_include_directories(${ModuleName} PUBLIC ${CMAKE_SOURCE_DIR}/third-party/binaries/ShaderConductor/Include)
set_cpp_options(${ModuleName} 20 TRUE)