#pragma once


#if BUILD_VULKAN_PLAYER
const char kLogPrefix[] = "rs_debug_player-UE4";
#elif BUILD_VULKAN_LAYER
const char kLogPrefix[] = "rs_debug_layer-UE4";
#else
const char kLogPrefix[] = "rs_debug_Other-UE4";
#endif
constexpr char kLogFilePath[] = "/sdcard/Android/data/com.tencent.toaa/debuglayer.txt";
