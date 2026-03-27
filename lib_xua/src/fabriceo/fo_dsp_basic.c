/*
 * fo_dsp_basic.c
 *
 *  Created on: 25 oct. 2025
 *      Author: fabriceo
 *
 *      basic routines placeholder to handle DSP tasks in lib_xua
 *      with synchronization by AudiohubMainLoop
 *
 */
#include <xs1.h>
#include <string.h>
#include "fo_helpers.h"

#include "fo_dsp_basic.h"

//default structure for handling dsp task start/sync/stop within xua_audiohub
 xua_dsp_head_t xua_dsp_head;

//contains timestamp when audio hub is triggering all the dsp tasks.
volatile int xua_dsp_timestart;
unsigned xua_dsp_synchronizer = 0;  //reflects dsp tasks running or not
unsigned XUA_DSP_BUFF_OFS;

void xua_dsp_init_(unsigned n) {
    memset(&xua_dsp_head,0,sizeof(xua_dsp_head));
}

//called just before launching Audiomainloop and all the dsptaks
void xua_dsp_reset(unsigned n) __attribute__ ((weak));
void xua_dsp_reset(unsigned n) { xua_dsp_init_(n); }

//called just before launching Audiomainloop and all the dsptaks
void xua_dsp_init(unsigned n)  __attribute__ ((weak));
void xua_dsp_init(unsigned n) { xua_dsp_init_(n); }


void xua_dsp_core_(const unsigned n) __attribute__ ((weak));
void xua_dsp_core_(const unsigned n) {  }

void xua_dsp_task_(const unsigned x) __attribute__ ((weak));
void xua_dsp_task_(const unsigned x) {
    asm volatile("#xua_dsp_task_:");
    //sequences below are fully optimized for maximizing performance inside while (1) loop
    //all addresses are computed before the while (1) and stored in registers r4..r9 (saved by calleee)
    xua_dsp_head.runids._8[x] = (char)get_local_tile_id();
    if (xua_dsp_head.runable._8[x]) asm volatile("nop");
    xua_dsp_head.tcb[x].time = 0;
    xua_dsp_head.running._8[x] = 0;
    while (1) {
        //wait for master task to trigger us
        asm volatile("ssync":::"memory");
        //check if we are supposed to finish our task
        if (xua_dsp_head.runable._8[x] == 0) break;
        //launch the formal dsp core content
        xua_dsp_core(x);
        //clear our local flag to show master taht we are finished
        xua_dsp_head.running._8[x] = 0;
        //compute and store completion time
        xua_dsp_head.tcb[x].time = gettime() - xua_dsp_timestart;
    }
    //special value to show the task is finished and will not re-enter
    xua_dsp_head.tcb[x].time = 3;
}

unsigned xua_dsp_trigger_() __attribute__ ((weak));
unsigned xua_dsp_trigger_() {
    asm volatile("#xua_dsp_trigger_:");
    int sync = xua_dsp_get_synchronizer();
    if(sync) {
        int time = gettime();
        xua_dsp_head.runlast = xua_dsp_head.running;
        //check if a dsp tasks is not finished, then do not trigger a next cycle yet.
        if (xua_dsp_head.runlast._64) { return (unsigned)&xua_dsp_head.runlast._64; }
        //trigger all tasks registered against our synchronizer
        asm volatile("msync res[%0]"::"r"(sync));
        xua_dsp_timestart = time;
        xua_dsp_head.running = xua_dsp_head.runable;
    }
    return 0;
}


void xua_dsp_task_1(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_1(const unsigned n) { xua_dsp_task(1); }
void xua_dsp_task_2(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_2(const unsigned n) { xua_dsp_task(2); }
void xua_dsp_task_3(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_3(const unsigned n) { xua_dsp_task(3); }
void xua_dsp_task_4(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_4(const unsigned n) { xua_dsp_task(4); }
void xua_dsp_task_5(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_5(const unsigned n) { xua_dsp_task(5); }
void xua_dsp_task_6(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_6(const unsigned n) { xua_dsp_task(6); }
void xua_dsp_task_7(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_7(const unsigned n) { xua_dsp_task(7); }
