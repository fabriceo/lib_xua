/*
 * fo_helpers.h
 *
 *  Created on: 25 oct. 2025
 *      Author: fabriceo
 *
 *      some helpers function or inline, also for inferfacing with cpp
 *
 *
 */

#ifndef FO_HELPERS_H_
#define FO_HELPERS_H_


#ifndef XUA_JOIN0
#define XUA_JOIN0(x,y) x ## y
#define XUA_JOIN(x,y) XUA_JOIN0(x,y)
#endif

//usefull defines
#ifdef __XC__
#  ifndef XCUNSAFE
#    define XCUNSAFE unsafe
#  endif
#  ifndef XCTIMER
#    define XCTIMER  timer
#  endif
#  ifndef XCPORT
#    define XCPORT   port
#  endif
#  ifndef XCCHAN
#    define XCCHAN   chanend
#  endif
#else
#  ifndef XCUNSAFE
#    define XCUNSAFE
#  endif
#  ifndef XCTIMER
#    define XCTIMER  unsigned
#  endif
#  ifndef XCPORT
#    define XCPORT   unsigned
#  endif
#  ifndef XCCHAN
#    define XCCHAN   unsigned
#  endif
#endif


//used to access 64bits variable as array of 8 bytes
union u_64_8x8 {
    unsigned long long _64;
    char _8[8];
    unsigned u[2];
    int i[2];
};

#ifndef EXTERNC
#if defined( __cplusplus )
#define EXTERNC     extern "C"
#define EXTERNC_ON  extern "C" {
#define EXTERNC_OFF }
#else
#define EXTERNC extern
#define EXTERNC_ON
#define EXTERNC_OFF
#endif
#endif


#ifndef __XC__
/* Support for xCORE  channels in C */
#ifndef null
#define null (0)
#endif
#ifndef xcoutuint
#define xcoutuint(c, x)   asm ("out res[%0], %1" :: "r" (c), "r" (x))
#endif
#ifndef xcoutct
#define xcoutct(c, x)     asm ("outct res[%0], %1" :: "r" (c), "r" (x))
#endif
#ifndef xcchkct
#define xcchkct(c, x)     asm ("chkct res[%0], %1" :: "r" (c), "r" (x))
#endif
#ifndef xcinuint
static inline unsigned xcinuint_(unsigned c) { int res; asm ("in %0, res[%1]" :"=r"(res): "r" (c)); return res; }
#define xcinuint(c) xcinuint_(c)
#endif
#ifndef xcinct
static inline unsigned xcinct_(unsigned c) { int res; asm ("inct %0, res[%1]" :"=r"(res): "r" (c)); return res; }
#define xcinct(c) xcinct_(c)
#endif
#endif

#ifndef gettime
static inline int gettime_() {
    int time; asm volatile("gettime %0":"=r"(time)); return time;
}
#define gettime() gettime_()
#endif
#if 0
static inline unsigned getcoreid() {
    int core;
    asm volatile("get r11,id ; mov %0,r11":"=r"(core)::"r11");
    return core;
}
#endif

#if 0
static inline unsigned getcoretimer() {
    unsigned tmr;
    asm volatile("get r11,id ; ldaw %0,dp[__timers] ; ldw %0,%0[r11]":"=r"(tmr)::"r11");
    return tmr;
}
#endif
#endif /* FO_HELPERS_H_ */
