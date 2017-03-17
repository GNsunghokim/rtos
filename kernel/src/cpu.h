#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>
#include <stdbool.h>

#define CPU_FEATURE_SSE_4_1		1
#define CPU_FEATURE_SSE_4_2		2
#define CPU_FEATURE_MONITOR_MWAIT	3
#define CPU_FEATURE_MWAIT_INTERRUPT	4
#define CPU_FEATURE_TURBO_BOOST		5
#define CPU_FEATURE_INVARIANT_TSC	6

void cpu_init();
bool cpu_has_feature(int feature);

#endif /* __CPU_H__ */
