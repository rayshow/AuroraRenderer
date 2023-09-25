set(ModuleName "SRC")

file(GLOB_RECURSE HEADERS LIST_DIRECTORIES TRUE  "*.h")
file(GLOB_RECURSE CPP_FILES LIST_DIRECTORIES TRUE "*.cpp")
add_library(${ModuleName} STATIC ${CPP_FILES} ${HEADERS})
target_include_directories(${ModuleName} PUBLIC ${PROJECT_SOURCE_DIR}/third-party/Vulkan/Include)
target_include_directories(${ModuleName} PUBLIC ${PROJECT_SOURCE_DIR}/src)
set_cpp_options(${ModuleName} 20 TRUE)
