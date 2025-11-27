set(ModuleName "SRC")
set(DEPENDENT_LIBS "d3d12.lib;dxgi.lib;d3dcompiler.lib;")


file(GLOB_RECURSE HEADERS LIST_DIRECTORIES TRUE  "*.h")
file(GLOB_RECURSE CPP_FILES LIST_DIRECTORIES TRUE "*.cpp")
add_library(${ModuleName} STATIC ${CPP_FILES} ${HEADERS})
target_include_directories(${ModuleName} PUBLIC ${PROJECT_SOURCE_DIR}/src/developer)
target_link_libraries(${ModuleName} PRIVATE ${DEPENDENT_LIBS})
set_cpp_options(${ModuleName} 20 TRUE)
