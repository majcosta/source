#include "profiler.h"

#ifdef PROFILER_ENABLED
__int64 PerfManager::getCPUCount() const
{
	_asm rdtsc
}
#endif
