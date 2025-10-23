/*
 * audiohub_fabriceo.h
 *
 *  Created on: 22 oct. 2025
 *      Author: fabriceo
 */


#ifndef AUDIOHUB_FABRICEO_H_
#define AUDIOHUB_FABRICEO_H_

#include "xua_fabriceo.h"
/************ AUDIOHUB EXTENSIONS ***************/

//test depending on flag set in xua_audiohub.c
extern port p_for_mclk_count_audio;      //declared in lib_xua/main.xc (line about 165+172)

//test depending on feature allowed or not in xua_conf.h
#if defined( XUA_AUDIOHUB_TIMING ) && (XUA_AUDIOHUB_TIMING==1)
//this is only relevant if we do printing

static int xua_timestamp_left;      //timer value after getting ADC or DAC written
static int xua_timestamp_right;     //timer value after getting ADC or DAC written
static int xua_timing_cycle;        // total time between 2 samples
static int xua_timing[2];           //total time to proceed LEFT (0) and RIGHT (1) cycles
static int xua_timing_max[2];       //maximum values since deliver loop started

#define XUA_TIMESTAMP_LEFT_IN()   { int time; asm volatile("gettime %0":"=r"(time)); \
                                    xua_timing[1] = time - xua_timestamp_right; }

#define XUA_TIMESTAMP_LEFT_OUT()  { int time = xua_timestamp_left; asm volatile("gettime %0":"=r"(xua_timestamp_left)); \
                                    xua_timing_cycle = xua_timestamp_left - time; \
                                    if (xua_timing[1] > xua_timing_max[1]) xua_timing_max[1] = xua_timing[1]; }

#define XUA_TIMESTAMP_RIGHT_IN()  { int time; asm volatile("gettime %0":"=r"(time)); \
                                    xua_timing[0] = time - xua_timestamp_left;  }

#define XUA_TIMESTAMP_RIGHT_OUT() { asm volatile("gettime %0":"=r"(xua_timestamp_right)); \
                                    if (xua_timing[0] > xua_timing_max[0]) xua_timing_max[0] = xua_timing[0]; }

// to be lauched just before 1st cycle.
#define XUA_TIMING_RESET() { xua_timing[0] = xua_timing[1] = xua_timing_max[0] = xua_timing_max[1] = 0; \
                             asm volatile("gettime %0":"=r"(xua_timestamp_right)); }

static void XUA_TIMING_PRINT() {
    debug_printf("cycle = %4d, left = %4d (%4d), right = %d (%d)\n",xua_timing_cycle,xua_timing[0],xua_timing_max[0],xua_timing[1],xua_timing_max[1]);
}

#else
#define XUA_TIMESTAMP_LEFT_IN()   do { } while(0)
#define XUA_TIMESTAMP_LEFT_OUT()  do { } while(0)
#define XUA_TIMESTAMP_RIGHT_IN()  do { } while(0)
#define XUA_TIMESTAMP_RIGHT_OUT() do { } while(0)
#define XUA_TIMING_RESET()        do { } while(0)
#define XUA_TIMING_PRINT()        do { } while(0)
#endif // XUA_AUDIOHUB_TIMING==1

#if defined( XUA_TIMEOUT_CHECK ) && ( XUA_TIMEOUT_CHECK == 1 )

static unsigned XUA_TIMEOUT_RAISED = 0;
static unsigned XUA_TIMEOUT_PREV = 0;
static unsigned XUA_TIMEOUT_DELTA = 0;
static unsigned XUA_TIMEOUT_COUNT = 0;
#define XUA_TIMEOUT_DELAY (PLATFORM_REFERENCE_HZ / MIN_FREQ * 2 )
#define XUA_TIMEOUT_RESET() { XUA_TIMEOUT_RAISED = XUA_TIMEOUT_PREV = XUA_TIMEOUT_DELTA = XUA_TIMEOUT_COUNT = 0; }

#else
#define XUA_TIMEOUT_RESET() do { } while(0)
#endif // XUA_TIMEOUT_CHECK

#if defined( XUA_AUDIOHUB_DSP_TASKS ) && ( XUA_AUDIOHUB_DSP_TASKS >=1 )

unsigned XUA_DSP_SYNCHRONIZER = 0;  //reflects dsp tasks running or not
unsigned XUA_DSP_BUFF_OFS = 0;      //offset of the sample buffer used by dsptasks (A/B)
#define XUA_DSP_SAVE_SYNCHRONIZER() asm volatile("stw r5,dp[XUA_DSP_SYNCHRONIZER]":::"memory","r5");
#define XUA_DSP_RESET() do { asm volatile("stw %0,dp[XUA_DSP_SYNCHRONIZER]"::"r"(0)); XUA_DSP_BUFF_OFS=0; xua_dsp_reset(XUA_AUDIOHUB_DSP_TASKS); } while(0)
#define XUA_DSP_TASK(x) while(1) { XUAFO_JOIN(xua_dsp_task_,x)(x); int s; asm volatile("ldw %0,dp[XUA_DSP_SYNCHRONIZER]":"=r"(s)); if (s==0) break;  }
#define XUA_DSP_KILL_ALL_TASKS(x) do { asm volatile("stw %0,dp[XUA_DSP_SYNCHRONIZER]"::"r"(0)); } while(0)
#define XUA_DSP_INIT(x) do { xua_dsp_init(x); } while(0)
#else
#define XUA_DSP_TASK(x)                 do { } while(0)
#define XUA_DSP_BUFF_OFS (0)
#define XUA_DSP_SAVE_SYNCHRONIZER()     do { } while(0)
#define XUA_DSP_KILL_ALL_TASKS(x)       do { } while(0)
#define XUA_DSP_INIT(x)                 do { } while(0)
#define XUA_DSP_RESET()                 do { } while(0)
#endif


#endif /* AUDIOHUB_FABRICEO_H_ */
