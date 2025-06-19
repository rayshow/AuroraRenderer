#pragma once
#if AR_PLATFORM_ANDROID

#include<sys/time.h>
#include"type.h"

PROJECT_NAMESPACE_BEGIN

struct AndroidTime  
{
	// android uses BSD time code from GenericPlatformTime
	static double Seconds()
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return ((double) ts.tv_sec) + (((double) ts.tv_nsec) / 1000000000.0);
	}

	static uint32 Cycles()
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return (uint32) ((((uint64)ts.tv_sec) * 1000000ULL) + (((uint64)ts.tv_nsec) / 1000ULL));
	}

	static uint64 Cycles64()
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return ((((uint64)ts.tv_sec) * 1000000ULL) + (((uint64)ts.tv_nsec) / 1000ULL));
	}
};

using TimeMeasure = AndroidTime;

PROJECT_NAMESPACE_END

#endif