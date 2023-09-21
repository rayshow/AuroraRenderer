#if RS_PLATFORM_ANDROID

#include"android_application.h"

void android_main(android_app* app)
{
	AndroidApplication::GuardMain();
}

#endif