#ifndef _DSP_FILTER_CALC_H_
#define _DSP_FILTER_CALC_H_

#include <stdint.h>
#include <math.h>


//default biquads coeeficient always computed in float format and stored either in float or int32. 
struct dspFilterCoefs { 
    float b0, b1, b2, zero, a1, a2; 
};

static inline uint32_t asuint32 (float f) {
    union { float f; uint32_t i; } u = {f};
    return u.i; }

static inline float asfloat (uint32_t i) {
    union { uint32_t i; float f; } u = {i};
    return u.f; }

#if 0
//copy all the coefficients from source to dest, continuing until source->skip != 0
//pdest increment by 1 or 2 depending on skipFactor parameter to allow target coefficent for Slew
//if skip factor == 2 -> destination is set to "target" instead of active
dspFilterCoefs *  dspFilterCoefsCopy(dspFilterCoefs * pdest,dspFilterCoefs *psource, uint32_t format);

#endif

float *  dspFilterCoefsCalcSlew(float * pdest, float *psource, uint32_t format);

//list of all filters possible, declared as type to allow checking parameters provided

//filters requiring only a Frequency parameter F
typedef enum { 
    FNONE=0,
    FLPBE2=1,FLPBE3,FLPBE4,FLPBE5,FLPBE6,FLPBE7,FLPBE8,         // bessel
    FHPBE2=9,FHPBE3,FHPBE4,FHPBE5,FHPBE6,FHPBE7,FHPBE8,
    FLPBE3db2=17,FLPBE3db3,FLPBE3db4,FLPBE3db5,FLPBE3db6,FLPBE3db7,FLPBE3db8,    // bessel at -3db cutoff
    FHPBE3db2=25,FHPBE3db3,FHPBE3db4,FHPBE3db5,FHPBE3db6,FHPBE3db7,FHPBE3db8,
    FLPBU2=33,FLPBU3,FLPBU4,FLPBU5,FLPBU6,FLPBU7,FLPBU8,        // buterworth
    FHPBU2=41,FHPBU3,FHPBU4,FHPBU5,FHPBU6,FHPBU7,FHPBU8,
    FLPLR2=49,FLPLR3,FLPLR4,       FLPLR6=53,    FLPLR8=55,     // linkwitz rilley
    FHPLR2=57,FHPLR3,FHPLR4,       FHPLR6=61,    FHPLR8=63,

    FLP1=64, FHP1=66, FLS1=68, FHS1=70, FAP1=72                 //first order basic filter, no Q required
} dspFilters_F_t;


//filters requiring a Frequency parameter F and a Q factor
typedef enum {
    FLP2=65, FHP2=67, FAP2=73, FNOTCH=75, FBP0DB=76, FBPQ=77, 
    FHILB=78
} dspFilters_FQ_t;

//filters requiring a Frequency parameter F and a Q factor and a Gain G
typedef enum {
    FLS2=69, FHS2=71, FPEAK=74
} dspFilters_FQG_t;

//filters linkwitz transform requiring F, Q, Fp, Qp 
typedef enum {
    FLT = 79,
} dspFilters_FQ_FpQp_t;

//possibility to force an empty filter with predefined maximum order, no need to declare F
typedef enum {
    FLAST=0,
    F_ORDER_1 = 1, F_ORDER_2, F_ORDER_3, F_ORDER_4,
    F_ORDER_5,     F_ORDER_6, F_ORDER_7, F_ORDER_8
} dspFilterOrder_t;


//helper to display filter's name in same order as ftype value
extern const char * dsp_filter_name[80];

//compute filters order based on ftype
//expected to be optimized at compile time
static inline uint32_t dspFilterGetOrder(const char f) {
    const char sections[] = {  1,2,1,2,1,2,1,2, 1,2,2,2,2,2,4,2 };
    if (f <= FHPLR8) return 1+(f & 7);
    else return sections[f-FLP1];
}

//this table provide a list of elementary filters to create 
//Bessels 2/3/4/6/8, Butterworth or Linkwitz rilley filters
struct filtersTable_s { char ftype; char btype; float F; float Q; };
extern filtersTable_s filtersTable[97];

//compute a set of coefficient for a simple 1st or 2nd order fiter type, at given fs (sampling rate)
//coefficient will be soredt in ptr->b0...a2
int32_t dspFilterCaclCoefs(char ftype, float fs, float F, float Q, float G, float * ptr);

//compute a set of coefficient for a linkwitz rilley fiter type, at given fs (sampling rate)
int32_t dspFilterCaclCoefsLT(char ftype, float fs, float F, float Q, float G, float Fp, float Qp, float * ptr);

int32_t dspFilterCaclCoefsMultiple(char ftype, float fs, float F, float G, float * pcoefs);

#endif // _DSP_FILTER_CALC_H_
