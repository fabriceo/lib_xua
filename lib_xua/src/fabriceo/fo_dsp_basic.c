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

#include "debug_print.h"

#include "fo_helpers.h"

#include "fo_dsp_basic.h"

extern     unsigned dummyCounter;

//default structure for handling dsp task start/sync/stop within xua_audiohub
xua_dsp_head_t xua_dsp_head;

//contains timestamp when audio hub is triggering all the dsp tasks.
int volatile xua_dsp_timestart;
unsigned volatile xua_dsp_synchronizer = 0;  //reflects dsp tasks running or not

unsigned volatile XUA_DSP_BUFF_OFS = 0;
unsigned volatile XUA_DSP_BUFF_OLD = 0;
//number of words allocated for the DSP buffer (doubled for A/B switching)
unsigned volatile XUA_DSP_BUFF_SIZE = 0;

//called just before launching Audiomainloop and all the dsptaks, with the buffer size
void xua_dsp_reset(unsigned buf) __attribute__ ((weak));
void xua_dsp_reset(unsigned buf) { 
    xua_dsp_clear_synchronizer();
    memset(&xua_dsp_head,0,sizeof(xua_dsp_head));
    XUA_DSP_BUFF_OFS = XUA_DSP_BUFF_SIZE =  buf; 
    XUA_DSP_BUFF_OLD = 0;
    #if defined(XUA_AUDIOHUB_DSP_TASKS)
    xua_dsp_head.runable._8[0] = (XUA_AUDIOHUB_DSP_TASKS >= 1);
    xua_dsp_head.runable._8[1] = (XUA_AUDIOHUB_DSP_TASKS >= 2);
    xua_dsp_head.runable._8[2] = (XUA_AUDIOHUB_DSP_TASKS >= 3);
    xua_dsp_head.runable._8[3] = (XUA_AUDIOHUB_DSP_TASKS >= 4);
    xua_dsp_head.runable._8[4] = (XUA_AUDIOHUB_DSP_TASKS >= 5);
    xua_dsp_head.runable._8[5] = (XUA_AUDIOHUB_DSP_TASKS >= 6);
    xua_dsp_head.runable._8[6] = (XUA_AUDIOHUB_DSP_TASKS >= 7);
    xua_dsp_head.runable._8[7] = (XUA_AUDIOHUB_DSP_TASKS >= 8);
    #else
    xua_dsp_head.runable._64 = 0;
    #endif
    debug_printf("xua_dsp_reset(%d)\n",buf);
}

//called just before launching Audiomainloop and all the dsptaks
void xua_dsp_init(unsigned sampFreq)  __attribute__ ((weak));
void xua_dsp_init(unsigned sampFreq) { 
    //launched at each "Audiomainloop" with a sampling rate value
    XUA_DSP_BUFF_OLD = 0;
    XUA_DSP_BUFF_OFS = XUA_DSP_BUFF_SIZE;
    //define that all tasks can be ran.
    debug_printf("xua_dsp_init(%d)\n",sampFreq);
}

//empty weak procedure should be overloaded by user program
void xua_dsp_core_(const unsigned n) __attribute__ ((weak));
void xua_dsp_core_(const unsigned n) {  }

void xua_dsp_task_(const unsigned x) __attribute__ ((weak));
void xua_dsp_task_(const unsigned x) {
    asm volatile("#xua_dsp_task_:");
    //sequences below are fully optimized for maximizing performance inside while (1) loop
    //all addresses are computed before the while (1) and stored in registers r4..r9 (saved by calleee)
    //delay_microseconds(1+x); //just to make atomic access easier
    xua_dsp_head.runids._8[x] = (char)(get_local_tile_id()+1);
    char volatile * prunnable = &xua_dsp_head.runable._8[x];
    char volatile * prunning  = &xua_dsp_head.running._8[x];
    int  volatile * ptime     = &xua_dsp_head.tcb[x].time;
    *prunning = 0; *ptime = 0; //*prunnable = 1;
    debug_printf("xua_dsp_task_(%d), runnable %d\n",x,*prunnable);
    while (1) {
        //wait for master task to trigger us
        asm volatile("ssync":::"memory");
        //debug_printf("xua_dsp_task_ %d\n",x);
        //check if we are supposed to finish our task
        if (*prunnable == 0) break;
        //launch the formal dsp core content
        xua_dsp_core(x);
        //clear our local flag to show master that we are finished
        *prunning = 0;
        //compute and store completion time
        int time = gettime() - xua_dsp_timestart;
        if (time > *ptime) *ptime = time;
    }
    xua_dsp_head.runids._8[x] = 0;
    debug_printf("xua_dsp_task_(%d) ended, %d ticks\n",x,*ptime);
}

unsigned xua_dsp_trigger() __attribute__ ((weak));
unsigned xua_dsp_trigger() {
    asm volatile("#xua_dsp_trigger:");
    int sync = xua_dsp_get_synchronizer();
    //debug_printf("xua_dsp_trigger x%x\n",sync);
    if(sync) {
        XUA_DSP_BUFF_OLD = XUA_DSP_BUFF_OFS;
        XUA_DSP_BUFF_OFS = XUA_DSP_BUFF_SIZE - XUA_DSP_BUFF_OFS;
        int time = gettime();
        //check if a dsp tasks is not finished, then do not trigger a next cycle yet.
        if ((xua_dsp_head.runlast._64 = xua_dsp_head.running._64)) { 
            return (unsigned)&xua_dsp_head.runlast._64; }
        if ((xua_dsp_head.running._64 = xua_dsp_head.runable._64)) {
            //trigger all tasks registered against our synchronizer
            asm volatile("msync res[%0]"::"r"(sync));
            xua_dsp_timestart = time;
        }
    }
    return 0;
}

void xua_dsp_stop_all() __attribute__ ((weak));
void xua_dsp_stop_all() {
    int sync = xua_dsp_get_synchronizer();
    debug_printf("xua_dsp_stop_all, synchronizer x%x, wait while running\n",sync);
    asm volatile("#xua_dsp_stop_all:");
    unsigned long long volatile * prunning = &xua_dsp_head.running._64;
    xua_dsp_head.runable._64 = 0;
    if (sync) {
        while(*prunning) {}
        asm volatile("msync res[%0]"::"r"(sync));
        debug_printf("dsp tasks finished\n");
        xua_dsp_clear_synchronizer();
    }
}
