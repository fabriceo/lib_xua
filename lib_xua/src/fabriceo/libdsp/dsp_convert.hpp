/*
 * dsp_convert.hpp
 *
 *  Created on: 30 oct. 2025
 *      Author: fabrice
 */

#ifndef DSP_CONVERT_HPP_
#define DSP_CONVERT_HPP_

#include <stdint.h>
#include <limits.h>
#include <math.h>

#if defined(__XS2__) || defined(__XS3__)
#include <xs1.h>
#include <platform.h>
#define DSP_USE_XCORE (1)
#else
#undef DSP_USE_XCORE
#endif


//gives a float ratio based on given deciBell value
#ifndef dB
#define dB(x) (powf(10.0,(x)/20.0f))
#endif
//gives a float deciBell value based on given float ratio
#ifndef to_dB
#define to_dB(x) (20.0f*log10f(x))
#endif


namespace qconv {

//conversion and calc routines across qnm formats and combinations

inline void q64_saturate(int64_t *a, const int32_t m) { //m between 1 .. 31
    if ((m>0)&&(m < 31)) {
#ifdef DSP_USE_XCORE
    int32_t hi = *a >>32; int32_t lo = *a & 0xFFFFFFFF;
    asm volatile("lsat %0,%1,%2":"=r"(hi),"=r"(lo):"r"(m),"0"(hi),"1"(lo));
    *a = ((int64_t)hi << 32) | lo;
#else
    int64_t pone = 1 << (m+32); int64_t none = -pone;
    if (*a>=pone) *a = (pone-1);
    else if (*a<none) *a = none;
#endif
    }
}

inline void q32_saturate(int32_t *a, const int32_t m) {
    if ((m>0)&&(m < 31)) {
#ifdef DSP_USE_XCORE
    asm volatile("lsat %0,%1,%2":"=r"(*a):"r"(0),"r"(m),"0"(*a));
#else
     int32_t pone = 1 << m; int32_t none = -pone;
    if (*a >= pone) *a = (pone-1);
    else if (*a < none) *a = none;
#endif
    }
}


//convert a raw integer (without mantissa) to a qnm 32bits with mantissa m bits
inline void q32_int(int32_t * a, int32_t b, const int32_t m) {
#ifdef DSP_USE_XCORE
    //extend to 64bits, saturate and extract
    int32_t z=0; asm volatile("lsat %0,%1,%2 \n\t lextract %0,%0,%1,%2,32":"=r"(*a):"r"(z),"r"(32-m),"0"(*a));
#else
    q32_saturate(&b,32-m);
    *a = b << m;
#endif
} //OK

//round up qnm 32bits by adding 0.5 -> can overload
inline void q32_roundup(int32_t *a, const int32_t m) {
    *a += (1UL<<(m-1));
} //OK

//convert a qnm 32 bit with m bits mantissa to a basic integer without mantissa
inline void int_q32(int32_t * a, int32_t b, const int32_t m) {
    //round and  shift left
    q32_roundup(&b,m); 
    *a = b >> m;
} //OK

//convert a float value into a qnm 32 bits
inline void q32_float(int32_t * a, float b, const int32_t m) {
    //mul by 1<<mant and keep integer part
    b *= (1UL << m); *a = b;
} //OK

//convert a qnm 32 bits to a float format
inline void float_q32(float * a, int32_t b, const int32_t m) { //divide by 1<<mant
    float f = b; f /= (1UL << m); *a = f;
} //OK

//convert a float value into a qnm 32 bits
inline void q32_double(int32_t * a, double b, const int32_t m) {
    //mul by 1<<mant and keep integer part
    b *= (1UL << m); *a = b;
} //OK

//convert a qnm 32 bits to a float format
inline void double_q32(double * a, int32_t b, const int32_t m) { //divide by 1<<mant
    double f = b; f /= (1UL << m); *a = f;
} //OK

//convert a given qnm (mantissa n) to a target qnm (mantissa m), providing m-n information
inline void q32_q32(int32_t * a, int32_t b, const int32_t m_n) {
    if (m_n==0) *a = b;
    else
#ifdef DSP_USE_XCORE
    if (m_n<0) { //target a is larger than b, dont need saturation, example 28<-31 delta = -3
        // sign extend+lextract
        int32_t sign = b >> 31; // sign extend
        asm("lextract %0,%1,%2,%3,32":"=r"(*a):"r"(sign),"r"(b),"r"(-m_n));
    } else { //target a is smaller than b,
        // put 0 on lsb to create 64 bits , saturate and lextract
        int32_t z = 0;
        asm volatile("lsat %0,%1,%2 \n\t lextract %0,%0,%1,%2,32":"=r"(*a):"r"(z),"r"(m_n),"0"(*a));
    }
#else
    if (m_n<0) { //target a is larger than b, dont need saturation, example 28<-31 delta = -3
        *a = b >> -m_n;
    } else { //target a is smaller than b,
        *a = b << m_n;  //could overload, should saturate first
    }
#endif
} //OK

//convert a raw integer (without mantissa) to a qnm 64bits with mantissa m2 bits
inline void q64_int(int64_t * a, int32_t b, const int32_t ma) {
#ifdef DSP_USE_XCORE
    //extend 64bits, saturate , extract and extend with 0 lsb
    int32_t z=0; int32_t hi;
    asm volatile("lsat %0,%1,%2 ; lextract %0,%0,%1,%2,32":"=r"(hi),"=r"(z):"r"(64-ma),"0"(b),"1"(z));
    *a = ((int64_t)hi << 32) | z;
#else
    q32_saturate(&b,32-ma);
    *a = (int64_t)b << ma; //could overload
#endif
} //OK

//convert a qnm 64 bit with m2 bits mantissa to a basic integer without mantissa
inline void int_q64(int32_t * a, int64_t b, const int32_t ma) {
    // remove lowest bits and extend sign, round up, shift right
    b >>= 32; b+= (1UL << (ma-33)); *a = ((int32_t)b >> (ma-32));
} //OK

//convert given float as a 64 bit qnm, given mantissa ma
inline void q64_float(int64_t * a, float b, const int32_t ma) {
    float f = b; f /= (1ULL << ma); *a = f;
} //OK

inline void float_q64(float * a, int64_t b, const int32_t ma) {
    float f = b; f /= (1UL << ma); *a = f;
} //OK

//convert a float value into a qnm 32 bits
inline void q64_double(int64_t * a, double b, const int32_t m) {
    //mul by 1<<mant and keep integer part
    b *= (1UL << m); *a = b;
} //OK

inline void double_q64(double * a, int64_t b, const int32_t ma) {
    double f = b; f /= (1UL << ma); *a = f;
} //OK

//convert given qnm 32bits mantissa mb to a qnm 64bits mantissa ma
inline void q64_q32(int64_t * a, int32_t b, const int32_t ma, const int32_t mb) {
    //hi = shift left (m2-32-m1), lo = shigt right
    const int32_t m_n = (ma-32-mb);
    if (m_n==0) *a = (int64_t)b << 32;
    else if (m_n<0) { // target a is larger than b
        //just ashr+shr
        int32_t hi = b >> (-m_n); int32_t lo = b << (32+m_n);
        *a = ((int64_t)hi << 32) | lo;
    } else { // target a is smaller than b, requires saturation
    #ifdef DSP_USE_XCORE
        int32_t hi = b,lo=0;
        asm volatile("lsat %0,%1,%2 \n\t lextract %0,%0,%1,%2,32":"=r"(hi),"=r"(lo):"r"(m_n),"0"(b),"1"(lo));
        *a = ((int64_t)hi << 32) | lo;
    #else
        q32_saturate(&b,m_n);
        *a = (int64_t)b << (32+m_n);
    #endif
        }
} //OK

//convert a given qnm 64 bit mantissa mb to a qnm 32 bits mantiassa ma
inline void q32_q64(int32_t * a, int64_t b, const int32_t m32, const int32_t m64) {
    const int32_t m_n = (m64-32-m32);
    if (m_n==0) *a = (b >> 32);
    else if (m_n<0) { //// target a is smaller than b
    #ifdef DSP_USE_XCORE
        int32_t hi = b >> 32; uint32_t lo = b & 0xFFFFFFFF;
        asm volatile("lsat %0,%1,%2 \n\t lextract %1,%0,%1,%2,32":"=r"(hi),"=r"(lo):"r"(32+m_n),"0"(hi),"1"(lo));
        *a = lo;
    #else
        b >>= 32-m_n; 
        *a = b;
    #endif
    } else {
        int32_t hi = b >> 32; hi >>= m_n; *a = hi;
    }
} //OK

//convert given qnm 32bits mantissa m to a qnm 64bits mantissa ma
inline void q64_q64(int64_t * a, int64_t b, const int32_t m_n) {
    //hi = shift left (m2-32-m1), lo = shigt right
    if (m_n==0) *a = b;
    else if (m_n<0) { // target a is larger than b
        *a = (b >> -m_n);
    } else { // target a is smaller than b, requires saturation
        //saturation TODO
        *a = (b << m_n);
    }
} //OK


inline void q64_q32_add(int64_t * a, int64_t b, int32_t m) { } // convert to b to 64 bit and simply a+b
inline void q64_q32_sub(int64_t * a, int64_t b, int32_t m) { } // convert to b to 64 bit and simply a+b
inline void q64_q32_mul(int64_t * a, int32_t b, int32_t m2,int32_t m) { } //  see avdsp gain

inline void q64_q64_add(int64_t * a, int64_t b) { } // simply a+b
inline void q64_q64_sub(int64_t * a, int64_t b) { } // simply a+b
inline void q64_q64_mul(int64_t * a, int32_t b, int32_t m2,int32_t m) { } //  see avdsp mul

}

//dsp_fast_float from xmos lib_dsp in s32_t

#define DSP_FLOAT_ZERO_EXP (INT_MAX/2)
#define DSP_FLOAT_U32_ZERO {0, DSP_FLOAT_ZERO_EXP}
#define DSP_FLOAT_S32_ZERO {0, DSP_FLOAT_ZERO_EXP}
#define DSP_FLOAT_U32_ONE {UINT_MAX, -32}
#define DSP_FLOAT_S32_ONE {INT_MAX, -31}

/**
 * Floating point struct with S32 mantissa.
 */
typedef struct {
    int32_t m;      ///< Mantissa.
    int32_t e;      ///< Exponent.
} dsp_ff32_t;


//count leading zeroes
int32_t dsp_clz_i32(const int32_t a) {
    int32_t res;
    #if defined(__XS2__) || defined(__XS3__)
    asm("clz %0,%1":"=r"(res):"r"(a));
    #else
    if (a==0) return 32;
    int32_t b = a;
    res = 0;
    while (b && (b & 0xFF000000) == 0) { res += 8; b <<= 8; }
    while (b && (b & 0x80000000) == 0) { res++;    b <<= 1; }
    #endif
    return res;
}

//count leading sign bits
int32_t dsp_cls_i32(int32_t idata)
{
    //TODO check both results
    int32_t x;
#if defined (__XS3A__)
    asm volatile("cls %0, %1" : "=r"(x)  : "r"(idata));
#elif defined(__XS2A__)
    asm volatile("clz %0, %1 ; not %1,%1 ; clz %1,%1 ; add %0,%0,%1" : "=r"(x)  : "r"(idata));
#else
    //TODO could be optimized    
    x = (dsp_clz_i32(idata) + dsp_clz_i32(~idata));
#endif
    return x;
}


dsp_ff32_t dsp_neg_ff32(const dsp_ff32_t a){
    dsp_ff32_t r;
    r.e = a.e;
    if(a.m == INT_MIN) r.m = INT_MAX;
    else r.m = -a.m;
    return r;
}


uint32_t dsp_normalise_ff32(dsp_ff32_t *val){
    if(val->m == 0){
        val->m = 0;
        val->e = DSP_FLOAT_ZERO_EXP;
        return 0;
    }
    const uint32_t count = dsp_cls_i32(val->m) - 1;
    val->m <<= count;
    val->e -= count;
    return count;
}

int32_t  dsp_denormalise_and_saturate_ff32(const dsp_ff32_t a, const int32_t output_exp){
    if(a.e > output_exp){
        if(a.m > 0){
            return INT_MAX;
        } else {
            return -INT_MAX;
        }
    } else {
        return a.m >> (output_exp - a.e);
    }
}

//things

dsp_ff32_t dsp_abs_ff32(const dsp_ff32_t a){
    #if defined(__XS2__) || defined (__XS3__)
    dsp_ff32_t b; b.e = a.e; 
    int32_t tmp;
    asm("#aloc %0":"=r"(tmp));
    asm("ashr %2,%1,32 ; xor %0,%1,%2 ; sub %0,%0,%2":"=r"(b.m):"r"(a.m),"r"(tmp));
    return b;
    #else
    if(a.m < 0) return dsp_neg_ff32(a);
    else return a;
    #endif
}

//equalities

int32_t dsp_gte_ff32(const dsp_ff32_t a, const dsp_ff32_t b){
    if(a.e > b.e) {
        int32_t v = b.m >> (a.e - b.e);
        return a.m >= v;
    } else {
        int32_t v = a.m >> (b.e - a.e);
        return v >= b.m;
    }
}

int32_t dsp_eq_ff32(const dsp_ff32_t a, const dsp_ff32_t b){
    if(a.e > b.e){
        int32_t v = b.m >> (a.e - b.e);
        return a.m == v;
    } else {
        int32_t v = a.m >> (b.e - a.e);
        return v == b.m;
    }
}

//arithemetic

dsp_ff32_t dsp_add_ff32(const dsp_ff32_t a, const dsp_ff32_t b){
    dsp_ff32_t r;
#if defined(__XS2__)
int32_t tmp1,tmp2;
asm("#aloc %0,%1":"=r"(tmp1),"=r"(tmp2));
//13 inst or 3 if one value is 0
asm(
    "{ lss %5, %3, %1 ; bf %0,.Lmove%= }    \n\t"
    "ashr %0,%0,1                           \n\t"
    "{ sub %4, %1, %3 ; bf %2,.Lend%= }     \n\t"
    "{ sub %5,%3,%1 ; bf %5,.Ladd1%= }      \n\t"
    "{ add %4,%4,1 ; add %1,%1,1 }          \n\t"
    "ashr %5,%2,%4                          \n\t"
    "{ add %0,%0,%5 ; bu .Ladd2%=: }        \n\t"
".Lmove%=:                                  \n\t"
    "{ mov %0,%2 ; mov %1,%3 }              \n\t"
     "bu .Lend%=                            \n\t"
".Ladd1%=:                                  \n\t"
    "ashr %0,%0,%5                          \n\t"
    "ashr %5,%2,1                           \n\t"
    "{ add %0,%0,%5 ; add %1,%3,1 }         \n\t"
".Ladd2%=:                                  \n\t"
     "ashr %5,%0,32                         \n\t"   //get sign
     "xor %4,%0,%5                          \n\t"   //abs
     "sub %4,%4,%5                          \n\t"
     "clz %4,%4                             \n\t"   //count
     "sub %4,%4,1                           \n\t"   //reduce one as clz count the sign bit
    "{ shl %0,%0,%4 ; sub %1,%1,%4 }        \n\t"
".Lend%=:"
    :"=r"(r.m),"=r"(r.e):"r"(b.m),"r"(b.e),"r"(tmp1),"r"(tmp2),"0"(a.m),"1"(a.e));
//         %0.       %1.      %2.      %3.      %4.       %5
#elif defined (__XS3__)
int32_t tmp1,tmp2;
asm("#aloc %0,%1":"=r"(tmp1),"=r"(tmp2));
//9 inst or 3 if one value is 0
asm(
    "{ lss %5, %3, %1 ; bf %0,.Lmove%= }     \n\t"
    "ashr %0,%0,1                           \n\t"
    "{ sub %4, %1, %3 ; bf %2,.Lend%= }     \n\t"
    "{ sub %5,%3,%1 ; bf %5,.Ladd1%= }      \n\t"
    "{ add %4,%4,1 ; add %1,%1,1 }          \n\t"
    "ashr %5,%2,%4                          \n\t"
    "{ add %0,%0,%5 ; bu .Ladd2%=: }        \n\t"
".Lmove%=:                                  \n\t"
    "{ mov %0,%2 ; mov %1,%3 }              \n\t"
     "bu .Lend%=                            \n\t"
".Ladd1%=:                                  \n\t"
    "ashr %0,%0,%5                          \n\t"
    "ashr %5,%2,1                           \n\t"
    "{ add %0,%0,%5 ; add %1,%3,1 }         \n\t"
".Ladd2%=:                                  \n\t"
     "cls %4,%0                             \n\t"
    "{ shl %0,%0,%4 ; sub %1,%1,%4 }        \n\t"
".Lend%=:"
    :"=r"(r.m),"=r"(r.e):"r"(b.m),"r"(b.e),"r"(tmp1),"r"(tmp2),"0"(a.m),"1"(a.e));
//         %0.       %1.      %2.      %3.      %4.       %5
#else

    if(a.m == 0){ return b;
        //r.m = b.m; r.e = b.e; return r;
    }
    if(b.m == 0){ return a;
        //r.m = a.m; r.e = a.e; return r;
    }

    if(a.e > b.e){
        r.m = (a.m >> 1) + (b.m>>(a.e - b.e + 1)); //Lose 1b of precision when adding 0
        r.e = a.e + 1;
    } else {
        r.m = (b.m >> 1) + (a.m>>(b.e - a.e + 1));
        r.e = b.e + 1;
    }
    dsp_normalise_ff32(&r);
#endif
    return r;
}

dsp_ff32_t dsp_sub_ff32(const dsp_ff32_t a, const dsp_ff32_t b){
    dsp_ff32_t c = dsp_neg_ff32(b);
    return dsp_add_ff32(a, c);
}


dsp_ff32_t dsp_mul_ff32(const dsp_ff32_t a, const dsp_ff32_t b){
/*

    ldc r5,0 ; ldc r6,0
    maccs r5,r6,r0,r2
    clz r7,r5 ; ldc r8,32
    sub r8,r7 ; 
    lextract r5,r5,r6,r8,32
    add r8,r1,r3 ; sub r7,r7,1
    add r6,r8,r7

*/

    dsp_ff32_t r;
//TODO to be optimized
    int64_t va = a.m;
    int64_t vb = b.m;

    r.m = (va*vb)>>31;
    r.e = a.e + b.e + 31;

    dsp_normalise_ff32(&r);

    return r;
}

//not required by dsp library (yet)
#if 0


/**
 * Floating point struct with U32 mantissa.
 */
typedef struct {
    uint32_t m;     ///< Mantissa.
    int32_t  e;     ///< Exponent.
} dsp_u32_float_t;

uint32_t dsp_normalise_u32(dsp_u32_float_t *val){
    if(val->m == 0){
        val->m = 0;
        val->e = DSP_FLOAT_ZERO_EXP;
        return 0;
    }
    const uint32_t count = dsp_clz_i32(val->m);
    val->m <<= count;
    val->e -= count;
    return count;
}

dsp_ff32_t dsp_u32_to_ff32(const dsp_u32_float_t a){
    dsp_ff32_t r;
    r.m = a.m >> 1;
    r.e = a.e + 1;
    return r;
}


uint32_t dsp_denormalise_and_saturate_u32(const dsp_u32_float_t a, const int32_t output_exp){
    if(a.e > output_exp){
        return UINT_MAX;
    } else {
        return a.m >> (output_exp - a.e);
    }
}

dsp_u32_float_t dsp_abs_s32_to_u32(const dsp_ff32_t a){
    dsp_u32_float_t r;
    if(a.m > 0){
        r.m = a.m;
        r.e = a.e;
    } else {
        dsp_ff32_t t = dsp_neg_ff32(a);
        r.m = t.m;
        r.e = t.e;
    }
    r.m <<= 1;
    r.e -= 1;
    return r;
}

int32_t dsp_gte_u32_u32(const dsp_u32_float_t a, const dsp_u32_float_t b){
    if(a.e > b.e){
        int32_t v = b.m >> (a.e - b.e);
        return a.m >= v;
    } else {
        int32_t v = a.m >> (b.e - a.e);
        return v >= b.m;
    }
}


int32_t dsp_eq_u32_u32(const dsp_u32_float_t a, const dsp_u32_float_t b){
    if(a.e > b.e){
        int32_t v = b.m >> (a.e - b.e);
        return a.m == v;
    } else {
        int32_t v = a.m >> (b.e - a.e);
        return v == b.m;
    }
}


dsp_u32_float_t dsp_add_u32_u32(const dsp_u32_float_t a, const dsp_u32_float_t b){
    dsp_u32_float_t r;

    if(a.m == 0){
        r.m = b.m;
        r.e = b.e;
        return r;
    }
    if(b.m == 0){
        r.m = a.m;
        r.e = a.e;
        return r;
    }
    if(a.e > b.e){
        r.m = (a.m >> 1) + (b.m>>(a.e - b.e + 1));
        r.e = a.e + 1;
    } else {
        r.m = (b.m >> 1) + (a.m>>(b.e - a.e + 1));
        r.e = b.e + 1;
    }

    dsp_normalise_u32(&r);
    return r;
}


dsp_ff32_t dsp_sub_u32_u32(const dsp_u32_float_t a, const dsp_u32_float_t b){
    dsp_ff32_t d = dsp_u32_to_ff32(b);
    dsp_ff32_t c = dsp_neg_ff32(d);
    dsp_ff32_t e = dsp_u32_to_ff32(a);
    return dsp_add_ff32(e, c);
}

dsp_ff32_t dsp_mul_s32_u32(const dsp_ff32_t a, const dsp_u32_float_t b){
    dsp_ff32_t r;

    int64_t va = a.m;
    int64_t vb = b.m>>1;

    r.m = (va*vb)>>31;
    r.e = a.e + b.e + 32;

    dsp_normalise_ff32(&r);

    return r;
}


dsp_u32_float_t dsp_mul_u32_u32(const dsp_u32_float_t a, const dsp_u32_float_t b){
    dsp_u32_float_t r;

    uint64_t va = a.m;
    uint64_t vb = b.m;

    r.m = (va*vb)>>32;
    r.e = a.e + b.e + 32;

    dsp_normalise_u32(&r);
    return r;
}

//Warning: This is not to be extenally accessable
static dsp_u32_float_t s32_to_u32(const dsp_ff32_t a){
    dsp_u32_float_t r;
    r.m = a.m << 1;
    r.e = a.e - 1;
    return r;
}

//Warning: This is not to be extenally accessable
static dsp_u32_float_t neg_s32_to_u32(const dsp_ff32_t a){
    dsp_u32_float_t r;
    r.m = (-a.m)<<1;
    r.e = a.e - 1;
    return r;
}


dsp_u32_float_t dsp_div_u32_u32(const dsp_u32_float_t a, const dsp_u32_float_t b){
    if(b.m == 0){
      dsp_u32_float_t r = {0x80000000,-31};
      return r;
    }
    uint32_t numerator = a.m;
    int32_t numerator_exp = a.e;

    uint32_t denominator = b.m;
    int32_t denominator_exp = b.e;

    if(numerator == denominator){
        dsp_u32_float_t r = {0x80000000, numerator_exp +1 - denominator_exp -32};
        return r;
    }


    if (numerator >= denominator){
        numerator >>=1;
        numerator_exp += 1;
    }

    uint64_t t = (((uint64_t) numerator)<<32);

    dsp_u32_float_t r;

    int32_t temp;
    uint32_t hi = t>>32;
    uint32_t lo = t;
    asm volatile("ldivu %0, %1, %2, %3, %4":"=r"(r.m), "=r"(temp): "r"(hi), "r"(lo), "r"(denominator));

    r.e = numerator_exp - denominator_exp -32;

    return r;
}


dsp_ff32_t dsp_div_s32_s32(const dsp_ff32_t a, const dsp_ff32_t b){
    //TODO to be optimized
    if(b.m == 0){
      dsp_ff32_t r = {0x40000000,-30};
      return r;
    }
    if(a.m > 0){
        dsp_u32_float_t ua = s32_to_u32(a);

        if(b.m > 0){
            dsp_u32_float_t ub = s32_to_u32(b);
            dsp_u32_float_t c = dsp_div_u32_u32(ua, ub);
            return dsp_u32_to_ff32(c);
        } else {
            dsp_u32_float_t ub = neg_s32_to_u32(b);
            dsp_u32_float_t c = dsp_div_u32_u32(ua, ub);
            dsp_ff32_t d = dsp_u32_to_ff32(c);
            return dsp_neg_ff32(d);
        }
    } else {
        dsp_u32_float_t ua = neg_s32_to_u32(a);

        if(b.m > 0){
            dsp_u32_float_t ub = s32_to_u32(b);
            dsp_u32_float_t c = dsp_div_u32_u32(ua, ub);
            dsp_ff32_t d = dsp_u32_to_ff32(c);
            return dsp_neg_ff32(d);
        } else {
            dsp_u32_float_t ub = neg_s32_to_u32(b);
            dsp_u32_float_t c = dsp_div_u32_u32(ua, ub);
            return dsp_u32_to_ff32(c);
        }
    }

}

dsp_ff32_t dsp_div_s32_u32(const dsp_ff32_t a, const dsp_u32_float_t b){
    if(b.m == 0){
      dsp_ff32_t r = {0x40000000,-30};
      return r;
    }
    if(a.m < 0){
        dsp_ff32_t neg_a = dsp_neg_ff32(a);

        dsp_u32_float_t c = s32_to_u32(neg_a);

        dsp_u32_float_t d = dsp_div_u32_u32(c, b);

        dsp_ff32_t e = dsp_u32_to_ff32(d);

        return dsp_neg_ff32(e);
    } else {
        dsp_u32_float_t c = s32_to_u32(a);

        dsp_u32_float_t d = dsp_div_u32_u32(c, b);

        return dsp_u32_to_ff32(d);
    }
}

uint32_t dsp_sqrt30_xs2(uint32_t);
void dsp_sqrt_calc_exp(const int32_t int_exp, const  uint32_t hr, int32_t * shl, int32_t * out_exp);

dsp_u32_float_t dsp_sqrt_u32(const dsp_u32_float_t a){
    dsp_u32_float_t b;
    int32_t shl;
    if(a.m != 0){

        dsp_sqrt_calc_exp(a.e, dsp_clz_i32(a.m), &shl, &b.e);

#if (defined(__XS2A__) || defined (__XS3A__))
        if(shl > 0)
            b.m = dsp_sqrt30_xs2(a.m<<shl);
        else
            b.m = dsp_sqrt30_xs2(a.m>>(-shl));
#endif
        dsp_normalise_u32(&b);
        return b;
    } else {
        dsp_u32_float_t b = {0, DSP_FLOAT_ZERO_EXP};
        return b;
    }
}
void dsp_exponential_average_u32(dsp_u32_float_t *x, dsp_u32_float_t new_sample, uint32_t alpha){

    dsp_u32_float_t s;
    s.m = new_sample.m;
    s.e = new_sample.e;

    uint32_t c = dsp_clz_i32(alpha);
    uint32_t d = dsp_clz_i32(UINT_MAX - alpha);
    x->m = (uint32_t)(((uint64_t)x->m* (uint64_t)alpha)>>(32 -c));
    s.m = (uint32_t)(((uint64_t)s.m* (uint64_t)(UINT_MAX - alpha))>>(32- d));

    x->e -= c;
    s.e -= d;

    dsp_normalise_u32(x);
    dsp_normalise_u32(&s);

    *x = dsp_add_u32_u32(*x, s);

}

#endif



/**
 * @brief Convert a float value to a fixed point int32 number in
 *        q format. If the value of x is outside the fixed point range,
 *        this will overflow.
 * 
 * @param x A floating point value
 * @param q Q format of the output
 * @return int32_t x in q fixed point format
 */
static inline int32_t _float2fixed( float x, int32_t q )
{
#ifdef __XS3A__
  int32_t sign, exp, mant;
  asm("fsexp %0, %1, %2": "=r" (sign), "=r" (exp): "r" (x));
  asm("fmant %0, %1": "=r" (mant): "r" (x));
  if(sign){mant = -mant;}
  // mant to q
  int32_t shr = -q - exp + 23;
  return mant >>= shr;
#else
  if ( x < 0.0f ) return (((float)(1u << q))       * x - 0.5f);
  if ( x > 0.0f ) return (((float)((1u << q) - 1)) * x + 0.5f);
  return 0;
#endif
}

/**
 * @brief Convert a float value to a fixed point int32 number in
 *        q format. If the value of x is outside the positive 
 *        fixed point range,this will overflow.
 * 
 * @param x A floating point value
 * @param q Q format of the output
 * @return int32_t x in q fixed point format
 */
static inline int32_t _positive_float2fixed(float x, int32_t q)
{
#ifdef __XS3A__
  int32_t sign, exp, mant;
  asm("fsexp %0, %1, %2": "=r" (sign), "=r" (exp): "r" (x));
  asm("fmant %0, %1": "=r" (mant): "r" (x));
  // mant to q
  int32_t shr = -q - exp + 23;
  return mant >>= shr;
#else
  return ((float)((1u << q) - 1)) * x + 0.5f;
#endif
}


//generate an int32 with mantissa q from a floating point
static inline int32_t q32_float(float x, const int q) {
    #if defined (__XS3__)
    int32_t sign, exp, mant;
    asm volatile("fsexp %0, %1, %2" : "=r"(sign),"=r"(exp):"r"(x));
    asm(    "fmant %0, %0                   \n\t"
            "{ sext %1,1 ; add %2,%2,%3 }   \n\t"   //signe extend; exp += q
            "xor %0,%0,%1                   \n\t"   //partial abs(mant)
            "{ sub %0,%0,%1 ; ldc %1,23 }   \n\t"   //abs(mant), 
            "{ sub %1,%1,%2 }               \n\t"   //shr = 23 - exp - q
            "ashr %0,%0,%1                   \n\t"
        : "=r"(mant) : "r" (sign), "r" (exp), "r"(q) , "0" (x):);
    return mant;
    #elif defined (__XS2__)
    int32_t exp, mant, tmp1, tmp2;
    asm volatile("# registers alloc %0,%1,%2" : "=r"(exp),"=r"(tmp1),"=r"(tmp2));
    asm volatile(
            "{ ldc %3,23    ; shl %1,%0,8 }     \n\t"
            "{ shl %2,%0,1  ; shl %3,%3,%3 }    \n\t"
            "{ shr %2,%2,24 ; or %1,%1,%3 }     \n\t"
            "{ add %2,%2,%4 ; mkmsk %3,7 }      \n\t"
            "{ sub %2,%3,%2 ; ldc %3,0 }        \n\t"
            "ashr %0,%0,32                      \n\t"
            "{ lss %3,%3,%2 ; shr %1,%1,%2 }    \n\t"
            "{ bf %3,.Lgood%= ; mkmsk %3,32 }   \n\t"
            "shr %1,%3,1                        \n\t"
        ".Lgood%=:                              \n\t"
            "xor %1,%1,%0                       \n\t"
            "sub %0,%1,%0                       \n\t"
    :"=r"(mant)"r"(tmp1),"r"(exp),"r"(tmp2),"r"(q):"0"(x));
    #else
    x *= (1ull << q);
    return x;
    #endif
}

//generate a floating point from an int32 with mantissa q
static inline float float_q32(int32_t x, const int q) {
    int32_t sign, exp;
    float fp;
    asm volatile("# reserving %0, %1" : "=r"(sign),"=r"(exp));
    asm(    "ashr %1,%0,32                       \n\t"   //get sign
            "xor %0,%0,%1                       \n\t"   // partial abs(x)
            "{ neg %3,%3 ; ldc %2,9 }           \n\t"   //-q, 32-23
            "{ sub %0,%0,%1 ; sub %2,%3,%2 }    \n\t"   //abs(x), exp = -q + 23-32
            "fmake %0, %1, %2, %1, %0           \n\t"   //fp, sign, exp, sign, abs(x)
        : "=r"(fp) : "r" (sign), "r" (exp), "r"(q) , "0" (x));
    return fp;
}

/*

    slew 216 : 20 inst / bq

        ldd r1,r2,r0[3]
        ldd r3,r4,r0[0]
        sub r1,r1,r3 ; sub r2,r2,r4
        ashr r1,r1,r5
        ashr r2,r2,r5
        add r3,r3,r1 ; add r4,r4,r2
        std r3,r4,r0[0]

        ldd r1,r2,r0[4]
        ldd r3,r4,r0[1]
        sub r1,r1,r3 ; sub r2,r2,r4
        ashr r1,r1,r5
        ashr r2,r2,r5
        add r3,r3,r1 ; add r4,r4,r2
        std r3,r4,r0[1]

        ldd r1,r2,r0[4]
        ldw r3,r0[5]
        sub r1,r1,r3 
        ashr r1,r1,r5
        add r3,r3,r1 
        std r3,r2,r0[1]

    slew 316 : 8 inst per bq
        ldaw rx,coefsforshift
  { vclrdr                   ; ldc r11, 0              }
  { vsetc r11                ; mkmsk r2,24           }
  { vldc r0[0]               ; shr r2,r2,4             }       load target
  { vlsub r1[0]              ;               }      sub active
  { vlmul rx[0]              ;              }    shr equivalent
  { vladd r1[0]              ;               }      add active
vstrpv r1[0], r2

fp32 : 3 x 9 27
    ldd r1,r2,r0[3]
    ldd r3,r4,r0[0]
    fsub r1,r1,r3
    fsub r2,r2,r4
    fmul r1,r1,r5
    fmul r2,r2,r5
    fadd r3,r3,r1
    fadd r4,r4,r2
    std r3,r4,r0[0]


biquad float ~12
    ldd
    ldd
    fmac
    fmac
    ldd
    ldd
    fmac
    fmac
    ldw
    fmac
    stw

    
0    xn     
1    xn-1    
2    xn-2    
3   xn/yn
4   xn-1/yn-1
5   xn-2/yn-2

6   xn/yn
7   xn-1/yn-1
8   xn-2/yn-2

9   xn/yn
10  xn-1/yn-1
11  xn-2/yn-2
// 7 + N * 6 inst
    ldc r11,20 ; dualentsp x
    std r4,r5,sp[]
r4 = 1111b
r5 = temporary state+4/8
r11 = mkmsk 20 or 1111 1111 0000 0000 0000
  { vclrdr                  ; mkmsk r4,4                }
  { vsetc r11               ; mkmsk r11,r11            }
  { vldr   states           ; add r5,state, 4          }
   stw sample, state[0]     //store xn
   vstrpv r5,r11           //store vector shifted xn -> xn-1, xn-1 -> xn-2, yn -> yn-1, yn-1 -> yn-2
    { vldc state            ; add state,r5,8           }
    { vlmaccr coefs         ; sub section,section,1    }
    vstrpv  state,r4       //store yn only (4 lsb of the R vector)
    { nop  ; bf section, end }
     ldw r11,cp[mask] 1111 1111 0000 0000 0000
loop:
  { vldr   state            ; add r5,state, 4          }
  vstrpv r5,r11
    { vldc state            ; add state,r5,8     }
    { vlmaccr coefs         ; sub section,section,1        }
    vstrpv  state,r4         //store yn
    { nop ; bt section, loop }
    ldd r4,r5,sp[]
    retsp x

*/


#endif /* DSP_CONVERT_HPP_ */
