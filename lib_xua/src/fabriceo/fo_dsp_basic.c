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

//default structure for handling dsp task start/sync/stop within xua_audiohub
xua_dsp_head_t xua_dsp_head;

//contains timestamp when audio hub is triggering all the dsp tasks.
int volatile xua_dsp_timestart;
unsigned volatile xua_dsp_synchronizer = 0;  //reflects dsp tasks running or not

//current index in the sample buffer
unsigned volatile XUA_DSP_BUFF_OFS = 0;
//previous index in the sample buffer
unsigned volatile XUA_DSP_BUFF_OLD = 0;
//number of words allocated for the DSP buffer (doubled for A/B switching)
unsigned volatile XUA_DSP_BUFF_SIZE = 0;
unsigned volatile XUA_DSP_TRIGGER_ENABLE = 0;

//called just before launching Audiomainloop and all the dsptaks
void xua_dsp_config(unsigned sampFreq)  __attribute__ ((weak));
void xua_dsp_config(unsigned sampFreq)  { xua_dsp_config_( sampFreq ); }
void xua_dsp_config_(unsigned sampFreq) { 
    xua_dsp_trigger_enable();
    debug_printf("xua_dsp_config(%d)\n",sampFreq);
}

//called once, just after hwinit()
void xua_dsp_init()  __attribute__ ((weak));
void xua_dsp_init()  { xua_dsp_init_(); }
void xua_dsp_init_() { 
    xua_dsp_clear_synchronizer();
    memset(&xua_dsp_head,0,sizeof(xua_dsp_head));
    debug_printf("xua_dsp_init() weak, clear dsp header record\n");
}

//called before starting all dsp tasks
void xua_dsp_launch_tasks()  __attribute__ ((weak));
void xua_dsp_launch_tasks()  { xua_dsp_launch_tasks_(); }
void xua_dsp_launch_tasks_() { 
    xua_dsp_clear_synchronizer();
    memset(&xua_dsp_head,0,sizeof(xua_dsp_head));
    XUA_DSP_BUFF_OFS = XUA_DSP_BUFF_SIZE; 
    XUA_DSP_BUFF_OLD = 0;
    #if defined(XUA_AUDIOHUB_DSP_TASKS)
    debug_printf("xua_dsp_launch_tasks() weak, enable %d tasks\n",XUA_AUDIOHUB_DSP_TASKS);
    xua_dsp_head.runable._8[0] = (XUA_AUDIOHUB_DSP_TASKS >= 1);
    xua_dsp_head.runable._8[1] = (XUA_AUDIOHUB_DSP_TASKS >= 2);
    xua_dsp_head.runable._8[2] = (XUA_AUDIOHUB_DSP_TASKS >= 3);
    xua_dsp_head.runable._8[3] = (XUA_AUDIOHUB_DSP_TASKS >= 4);
    xua_dsp_head.runable._8[4] = (XUA_AUDIOHUB_DSP_TASKS >= 5);
    xua_dsp_head.runable._8[5] = (XUA_AUDIOHUB_DSP_TASKS >= 6);
    xua_dsp_head.runable._8[6] = (XUA_AUDIOHUB_DSP_TASKS >= 7);
    xua_dsp_head.runable._8[7] = (XUA_AUDIOHUB_DSP_TASKS >= 8);
    #endif
}

//empty weak procedure should be overloaded by user program
void xua_dsp_core_(const unsigned n) __attribute__ ((weak));
void xua_dsp_core_(const unsigned n) {  }

void xua_dsp_task_(const unsigned x) __attribute__ ((weak));
void xua_dsp_task_(const unsigned x) {
    asm volatile("#xua_dsp_task_:");
    //sequences below are fully optimized for maximizing performance inside while (1) loop
    //all addresses are computed before the while (1) and stored in registers r4..r9 (saved by calleee)
    xua_dsp_head.runids._8[x] = (char)(get_logical_core_id()+1);
    char volatile * prunnable = &xua_dsp_head.runable._8[x];
    //debug_printf("xua_dsp_task_(%d) weak started, core %d runnable %d\n",x,get_logical_core_id(),*prunnable);
    char volatile * prunning  = &xua_dsp_head.running._8[x];
    int  volatile * ptime     = &xua_dsp_head.tcb[x].time;
    int lastTime=0;
    *prunning = 0; *ptime = 0; //*prunnable = 1;
    while (1) {
        //wait for master task to trigger us
        asm volatile("ssync":::"memory");
        //check if we are supposed to finish our task
        if (*prunnable == 0) break;
        //launch the formal dsp core content
        xua_dsp_core(x);
        //clear our local flag to show master that we are finished
        *prunning = 0;
        //compute and store completion time
        int time = gettime() - xua_dsp_timestart;
        if (time > lastTime) *ptime = lastTime = time;
    }
    xua_dsp_head.runids._8[x] = 0;
    debug_printf("xua_dsp_task_(%d) ended, %d ticks\n",x,*ptime);
}

unsigned long long xua_dsp_trigger()  __attribute__ ((weak));
unsigned long long xua_dsp_trigger()  { return xua_dsp_trigger_(); }
unsigned long long xua_dsp_trigger_() {
    asm volatile("#xua_dsp_trigger_weak:");
    int sync = xua_dsp_get_synchronizer();
    //debug_printf("xua_dsp_trigger x%x\n",sync);
    if (XUA_DSP_TRIGGER_ENABLE && sync) {
        XUA_DSP_BUFF_OLD = XUA_DSP_BUFF_OFS;
        XUA_DSP_BUFF_OFS = XUA_DSP_BUFF_SIZE - XUA_DSP_BUFF_OFS;
        int time = gettime();
        //check if a dsp tasks is not finished, then do not trigger a next cycle yet.
        if ((xua_dsp_head.runlast._64 = xua_dsp_head.running._64)) { 
            return xua_dsp_head.runlast._64; }
        if ((xua_dsp_head.running._64 = xua_dsp_head.runable._64)) {
            //trigger all tasks registered against our synchronizer
            asm volatile("msync res[%0]"::"r"(sync));
            xua_dsp_timestart = time;
        }
    }
    return 0;
}

void xua_dsp_stop_all()  __attribute__ ((weak));
void xua_dsp_stop_all()  { xua_dsp_stop_all_(); }
void xua_dsp_stop_all_() {
    int sync = xua_dsp_get_synchronizer();
    debug_printf("xua_dsp_stop_all() weak, synchronizer x%x, wait while running\n",sync);
    asm volatile("#xua_dsp_stop_all_weak:");
    unsigned long long volatile * prunning = &xua_dsp_head.running._64;
    xua_dsp_head.runable._64 = 0;
    if (sync) {
        while(*prunning) { }
        asm volatile("msync res[%0]"::"r"(sync));
        debug_printf("xua_dsp_stop_all() done\n");
        xua_dsp_clear_synchronizer();
        XUA_DSP_TRIGGER_ENABLE = 0;
    }
}

//disable trigger process in audio task
void xua_dsp_trigger_disable()  __attribute__ ((weak));
void xua_dsp_trigger_disable()  { xua_dsp_trigger_disable_(); }
void xua_dsp_trigger_disable_() {
    XUA_DSP_TRIGGER_ENABLE = 0;
}

//disable trigger process and wait task completion
void xua_dsp_trigger_disable_wait()  __attribute__ ((weak));
void xua_dsp_trigger_disable_wait()  { xua_dsp_trigger_disable_wait_(); }
void xua_dsp_trigger_disable_wait_() {
    XUA_DSP_TRIGGER_ENABLE = 0;
    int sync = xua_dsp_get_synchronizer();
    unsigned long long volatile * prunning = &xua_dsp_head.running._64;
    if (sync) {
        while(*prunning) {}
    }
}

//enable the trigger process in audio task
void xua_dsp_trigger_enable() __attribute__ ((weak));
void xua_dsp_trigger_enable() { xua_dsp_trigger_enable_(); }
void xua_dsp_trigger_enable_() {
    XUA_DSP_TRIGGER_ENABLE = 1;
}