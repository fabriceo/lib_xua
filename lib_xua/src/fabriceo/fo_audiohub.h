/*
 * fo_audiohub.h
 *
 *  Created on: 22 oct. 2025
 *      Author: fabriceo
 *
 *      collection of DEFINE or procedures used INSIDE the standard audiohub routines.
 *      organized to be "fully transparent" if the below feature flags are not set in xua_conf.h
 *
 */


#ifndef FO_AUDIOHUB_H_
#define FO_AUDIOHUB_H_

#ifdef __xua_conf_h_exists__
    #include "xua_conf.h"
#endif

#ifdef __debug_conf_h_exists__
    #include "debug_print.h"
#endif

/************ AUDIOHUB EXTENSIONS ***************/

//test depending on feature allowed or not in xua_conf.h
#if defined( XUA_AUDIOHUB_TIMING ) && (XUA_AUDIOHUB_TIMING==1)

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

static void XUA_TIMING_PRINT(unsigned _fs) {
#ifdef __debug_conf_h_exists__
    debug_printf("TIMING at %d : cycle = %4d, left = %4d (%4d), right = %d (%d)\n",_fs,xua_timing_cycle,xua_timing[0],xua_timing_max[0],xua_timing[1],xua_timing_max[1]);
#endif
}

#else
#define XUA_TIMESTAMP_LEFT_IN()   do { } while(0)
#define XUA_TIMESTAMP_LEFT_OUT()  do { } while(0)
#define XUA_TIMESTAMP_RIGHT_IN()  do { } while(0)
#define XUA_TIMESTAMP_RIGHT_OUT() do { } while(0)
#define XUA_TIMING_RESET()        do { } while(0)
#define XUA_TIMING_PRINT(fs)      do { } while(0)
#endif // XUA_AUDIOHUB_TIMING==1


#if defined( XUA_TIMEOUT_CHECK ) && ( XUA_TIMEOUT_CHECK == 1 )

#include "fo_timeout.h"

static unsigned XUA_TIMEOUT_RAISED = 0;
static unsigned XUA_TIMEOUT_PREV   = 0;
static unsigned XUA_TIMEOUT_DELTA  = 0;
static unsigned XUA_TIMEOUT_COUNT  = 0;
#define XUA_TIMEOUT_DELAY  (PLATFORM_REFERENCE_HZ / MIN_FREQ * 2 )
#define XUA_TIMEOUT_RESET() { \
    XUA_TIMEOUT_RAISED = XUA_TIMEOUT_PREV = XUA_TIMEOUT_DELTA = XUA_TIMEOUT_COUNT = 0; }

#else
#define XUA_TIMEOUT_RESET() do { } while(0)
#endif // XUA_TIMEOUT_CHECK



#if defined( XUA_AUDIOHUB_DSP_TASKS ) && ( XUA_AUDIOHUB_DSP_TASKS >=1 )

#ifndef XUA_DSP_BUFFER_SIZE
#define XUA_DSP_BUFFER_SIZE             (0)
#endif

#include "fo_dsp_basic.h"

#define XUA_DSP_INIT()                  xua_dsp_init()
#define XUA_DSP_CONFIG(sr)              xua_dsp_config(sr)
#define XUA_DSP_LAUNCH_TASKS()          xua_dsp_launch_tasks()
#define XUA_DSP_SAVE_SYNCHRONIZER()     xua_dsp_save_synchronizer()
#define XUA_DSP_TASK(x)                 xua_dsp_task(x)
#define XUA_DSP_STOP_ALL()              xua_dsp_stop_all()
#define XUA_DSP_TRIGGER_LEFT()          xua_dsp_trigger()
#define XUA_DSP_TRIGGER_RIGHT()         do { } while(0)

#else

#define XUA_DSP_LAUNCH_TASKS()          do { } while(0)
#define XUA_DSP_INIT()                  do { } while(0)
#define XUA_DSP_TASK(x)                 do { } while(0)
#define XUA_DSP_SAVE_SYNCHRONIZER()     do { } while(0)
#define XUA_DSP_STOP_ALL()              do { } while(0)
#define XUA_DSP_CONFIG(sr)              do { } while(0)
#define XUA_DSP_TRIGGER_LEFT()          do { } while(0)
#define XUA_DSP_TRIGGER_RIGHT()         do { } while(0)

#endif

#if defined( XUA_AUDIOHUB_DIVIDE_SPDIF_CLK ) && (XUA_AUDIOHUB_DIVIDE_SPDIF_CLK > 0)

#endif

#endif /* FO_AUDIOHUB_H_ */
