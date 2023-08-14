#pragma once

#if defined(RHI_EXPORT)
#define RHI_API __declspec(dllexport)
#else 
#define RHI_API __declspec(dllimport) 
#endif 
RHI_API void dynamiclibtest();