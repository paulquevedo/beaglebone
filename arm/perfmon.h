#ifndef __PERFMON_H__
#define __PERFMON_H__

/* Events 0x00-0x1D defined in sC12.8 of DDI0406C
 * Events after are defined in each processors TRM */
enum {
    PERF_MON_SW_INCR = 0,
    PERF_MON_L1I_CACHE_REFILL,
    PERF_MON_L1I_TLB_REFILL,
    PERF_MON_L1D_CACHE_REFILL,
    PERF_MON_L1D_CACHE,
    PERF_MON_L1D_TLB_REFILL,

    PERF_MON_CYCLE_COUNTER = 0xffff,
};

extern int _perfmon_add(uint32_t counterSel, uint32_t event);
extern int _perfmon_reset(uint32_t counterSel);
extern int _perfmon_enable(void);
extern int _perfmon_disable(void);

extern uint32_t _perfmon_get(uint32_t counterSel);
#endif
