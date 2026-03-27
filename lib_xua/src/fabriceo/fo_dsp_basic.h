/*
 * xua_dsp_basic.h
 *
 *  Created on: 25 oct. 2025
 *      Author: fabriceo
 *
 *      basic hooks and minimum routines to dispatch dsp treatments
 *      over multiple tasks by audio hub on same tile
 *
 */

#ifndef FO_DSP_BASIC_H_
#define FO_DSP_BASIC_H_

#include "fo_helpers.h"


extern unsigned xua_dsp_synchronizer;

static inline void xua_dsp_save_synchronizer() {
    int s; //synchronizer is stored in R5 when enterring here from "__start_other_cores"
    asm volatile("stw r5,dp[xua_dsp_synchronizer] ; mov %0,r5":"=r"(s)::"memory","r5");
    if ((s & ~0x700) != 3) __builtin_trap();
}
static inline void xua_dsp_clear_synchronizer() {
    asm volatile("stw %0,dp[xua_dsp_synchronizer]"::"r"(0));
}
static inline unsigned xua_dsp_get_synchronizer() {
    int s; asm volatile("ldw %0,dp[xua_dsp_synchronizer]":"=r"(s));
    return s;
}

#ifdef __xua_conf_h_exists__
    #include "xua_conf.h"
#endif

typedef struct {
    char * XCUNSAFE runaddr; // absolute address of the task running status in xua_dsp_head.running
    int time;                // total time spent between MSYNC and SSYNC
#ifdef XUA_DSP_USER_TCB
    //place holder for additional task parameters
    XUA_DSP_USER_TCB
#endif
} xua_dsp_tcb_t;

// table for 8 possible dsp tasks, using 8bits per task status for atomic load/store with "ldd" assembly
typedef struct {
    union u_64_8x8 runids;
    union u_64_8x8 runable;      //pattern of task expected to run (8x8bits)
    union u_64_8x8 runlast;      //last known value of running, to detect overload across samples
    union u_64_8x8 running;      //status of the 8 tasks, usefull to wait end of all tasks.
    xua_dsp_tcb_t tcb[8];          //pointer on task TCB
} xua_dsp_head_t;

#ifndef xua_dsp_task
#define xua_dsp_task(x) xua_dsp_task_(x)
#endif

#ifndef xua_dsp_core
#define xua_dsp_core(x) xua_dsp_core_(x)
#endif

#ifndef xua_dsp_trigger
#define xua_dsp_trigger() xua_dsp_trigger_()
#endif


//all functions below are declared "weak" in fo_dsp_basic.c
EXTERNC_ON
void xua_dsp_init(unsigned n);
void xua_dsp_reset(unsigned n);
unsigned xua_dsp_trigger_();
void xua_dsp_core_(const unsigned n);
void xua_dsp_task_(const unsigned x);
void xua_dsp_task_1(const unsigned n);
void xua_dsp_task_2(const unsigned n);
void xua_dsp_task_3(const unsigned n);
void xua_dsp_task_4(const unsigned n);
void xua_dsp_task_5(const unsigned n);
void xua_dsp_task_6(const unsigned n);
void xua_dsp_task_7(const unsigned n);
EXTERNC_OFF

extern  xua_dsp_head_t xua_dsp_head;

#endif /* FO_DSP_BASIC_H_ */
