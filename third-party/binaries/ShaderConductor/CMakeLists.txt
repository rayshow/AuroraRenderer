set(ModuleName "ShaderConductorBinary")
set(DEPENDENT_LIBS "Lib/Release/ShaderConductor.lib;")

target_link_libraries(${ModuleName} PRIVATE ${DEPENDENT_LIBS})
set_cpp_options(${ModuleName} 20 TRUE)
