add_library(dxc SHARED IMPORTED GLOBAL)
set(DXCPath ${CMAKE_SOURCE_DIR}/third-party/binaries/dxc)

set_target_properties(dxc PROPERTIES IMPORTED_LOCATION ${DXCPath}/bin/x64/Release/dxcompiler.dll)
set_target_properties(dxc PROPERTIES IMPORTED_IMPLIB   ${DXCPath}/lib/x64/Release/dxcompiler.lib)
set_target_properties(dxc PROPERTIES IMPORTED_LOCATION_DEBUG ${DXCPath}/bin/x64/Debug/dxcompiler.dll)
set_target_properties(dxc PROPERTIES IMPORTED_IMPLIB_DEBUG   ${DXCPath}/lib/x64/Debug/dxcompiler.lib)