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


EXTERNC_ON
void xua_dsp_init_(unsigned n);
EXTERNC_OFF

extern  xua_dsp_head_t xua_dsp_head;

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

#endif /* FO_DSP_BASIC_H_ */
