#include "framework.h"
#include "perf.h"


LARGE_INTEGER g_llFreq;
LARGE_INTEGER g_llStart, g_llEnd;


void fn_vStartTimer( void )
{
	QueryPerformanceFrequency(&g_llFreq);
	QueryPerformanceCounter(&g_llStart);
}

float fn_xStopTimer( void )
{
	QueryPerformanceCounter(&g_llEnd);
	float xElapsed = (float)(g_llEnd.QuadPart - g_llStart.QuadPart) * 1000.0f / (float)g_llFreq.QuadPart;
	return xElapsed;
}
