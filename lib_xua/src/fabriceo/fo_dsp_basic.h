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

static inline void xua_dsp_save_synchronizer() {
    asm volatile("#xua_dsp_save_synchronizer:");
    int s; //synchronizer is stored in R5 when enterring here from "__start_other_cores"
    //this approach is compatible with .XC compiler
    asm volatile("stw r5,dp[xua_dsp_synchronizer] ;mov %0,r5":"=r"(s)::"memory","r5");
    //verify that the value we got is looking like a synchronizer
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
    int time;                // total time spent between MSYNC and next SSYNC
#ifdef XUA_DSP_USER_TCB
    //place holder for additional task parameters
    XUA_DSP_USER_TCB
#endif
} xua_dsp_tcb_t;

// table for 8 maximum dsp tasks, using 8bits per task status for atomic load/store with "ldd" assembly
typedef struct {
    union u_64_8x8 runids;       //contains the cpu ID +1 for each launched task, zero when finished
    union u_64_8x8 runable;      //pattern of tasks expected to run (8x8bits)
    union u_64_8x8 runlast;      //last known value of running, to detect overload between 2 samples
    union u_64_8x8 running;      //status of the 8 tasks, usefull to wait till end of all tasks.
    xua_dsp_tcb_t tcb[8];        //pointer on task TCB
} xua_dsp_head_t;

#ifndef xua_dsp_task
#define xua_dsp_task(x) xua_dsp_task_(x)
#endif

#ifndef xua_dsp_core
#define xua_dsp_core(x) xua_dsp_core_(x)
#endif


//all functions below are declared "weak" in fo_dsp_basic.c
EXTERNC_ON
void xua_dsp_init(unsigned sampFreq);
void xua_dsp_reset(unsigned ofs);
void xua_dsp_stop_all();
unsigned xua_dsp_trigger();
void xua_dsp_core_(const unsigned n);
void xua_dsp_task_(const unsigned x);
EXTERNC_OFF

#ifdef __XC__
extern unsigned XUA_DSP_BUFF_OFS;       //offset of the sample buffer used by dsptasks (B/A)
extern unsigned XUA_DSP_BUFF_OLD;       //offset of the sample buffer used by buffman  (A/B)
extern unsigned XUA_DSP_BUFF_SIZE;
#else
extern unsigned volatile XUA_DSP_BUFF_OFS;       //offset of the sample buffer used by dsptasks (B/A)
extern unsigned volatile XUA_DSP_BUFF_OLD;       //offset of the sample buffer used by buffman  (A/B)
extern unsigned volatile XUA_DSP_BUFF_SIZE;
#endif

extern  xua_dsp_head_t xua_dsp_head;


#endif /* FO_DSP_BASIC_H_ */
