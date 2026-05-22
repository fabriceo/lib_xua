#ifndef _DSP_FILTER_CALC_H_
#define _DSP_FILTER_CALC_H_

#include <stdint.h>
#include <string.h>     //for memset
#include <math.h>


//default biquads coeeficient always computed in float 32bits format 
struct dspFilterCoefs { 
    const unsigned int sizeofthis = sizeof(this);
    float b0, b1, b2, a1, a2;           //a1 and a2 are negated when computed
    uint8_t filterSections : 8;        //number of remaining section for the filter order example: LR8 = 4 sections
    unsigned bypass : 1;                //set to 1 to bypass this filter and associated sections
    unsigned mute : 1;                  //set to 1 to mute the whole filter containing this one
    unsigned spare : 14; 
    void reset()  { b0 = b1 = b2 = a1 = a2 = 0.0; filterSections = 0; bypass = 0; mute = 0; }
    void setBypass(bool val=true) { bypass = val ? 1 : 0; }
    void clrBypass() { bypass = 0; }
    void setMute(bool val=true) { mute = val ? 1 : 0; }
    void clrMute() { mute = 0; }
    //compute the "shift" factor needed to bring back filter coefficient within -2..+2
    unsigned calcShl(const unsigned int mant);
};

//biquads coefficient used for integer routines
struct __attribute__((aligned(8))) dspFilterCoefsInt {
    //do not change coefficient order
    const unsigned int sizeofthis = sizeof(this);
    int b0,b1,b2,a1,a2,zero;
    void reset() { b0 = b1 = b2 = a1 = a2 = zero = 0; }
};


//biquads coefficient used for routines using VPU
struct dspFilterCoefsVPU {
    //do not change coefficient order
    const unsigned int sizeofthis = sizeof(this);
    int b0,b1,b2,zero,a1,a2,zero2,zero3;
    void reset() { b0 = b1 = b2 = a1 = a2 = zero = zero2 = zero3 = 0; }
};

//used to convert a float number to an integer coded with fractional part (mantissa)
struct dspInt { 
    const int max32 = 0x7FFFFFFF;
    const int min32 = - max32;
    const float max = (1ULL << 31);
    const float min = -max;
    const unsigned int mant;
    const float mul;
    const float fmul;
    const bool overmax;
    const bool undermin;
    const bool over;
    const int  val;
    //default mantissa = 30, for -2 < coefs < +2
    dspInt(float f, unsigned int mant_ = 30) : 
        mant( mant_ ), mul( 1ULL << mant_ ), fmul( f*mul ),
        overmax( fmul >= max ), undermin( fmul <= min ), over( overmax || undermin ),
        val( overmax ? max32 : (undermin ? min32 : (int)fmul ) ) {  };
    operator int () const { return val; }
};


//used to convert a float number to an integer coded with fractional part (mantissa)
struct dspInt64 { 
    const long long  max64 = 0x7FFFFFFFFFFFFFFF;
    const long long  min64 = - max64;
    const double max = (1ULL << 63);
    const double min = -max;
    const unsigned int mant;
    const double mul;
    const double fmul;
    const bool overmax;
    const bool undermin;
    const bool over;
    const long long val;
    dspInt64(double f, unsigned int mant_ = 61) : 
        mant( mant_ ), mul( 1ULL << mant_ ), fmul( f*mul ),
        overmax( fmul >= max ), undermin( fmul <= min ), over( overmax || undermin ),
        val( overmax ? max64 : (undermin ? min64 : (int)fmul ) ) {  };
    operator long long () const { return val; }
};


//used to convert a float coded IEEE as a 32 bits integer, and opposite.
static inline uint32_t asuint32 (float f) {
    union { float f; uint32_t i; } u = {f};
    return u.i; }

static inline float asfloat (uint32_t i) {
    union { uint32_t i; float f; } u = {i};
    return u.f; }


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
    FHILB=78,
    FBP0DB2=76, FBP0DB4=80, FBP0DB6=81, FBP0DB8=82, 
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
extern const char * dsp_filter_name[83];

//compute filters order based on ftype
//expected to be optimized at compile time
static inline uint32_t dspFilterGetOrder(const char f) {
    const char sections[] = {  1,2,1,2,1,2,1,2, 1,2,2,2,2,2,4,2, 2,4,6 };
    if (f <= FHPLR8) return 1+(f & 7);
    else return sections[f-FLP1];
}

//this table provide a list of elementary filters to create 
//Bessels 2/3/4/6/8, Butterworth or Linkwitz rilley filters
struct filtersTable_s { char ftype; char btype; float F; float Q; };
extern filtersTable_s filtersTable[107];

//compute a set of coefficient for a simple 1st or 2nd order fiter type, at given fs (sampling rate)
//coefficient will be soredt in ptr->b0...a2
int32_t dspFilterCaclCoefs(char ftype, float fs, float F, float Q, float G, dspFilterCoefs &coefs);

//compute a set of coefficient for a linkwitz rilley fiter type, at given fs (sampling rate)
int32_t dspFilterCaclCoefsLT(char ftype, float fs, float F, float Q, float G, float Fp, float Qp, dspFilterCoefs &coefs);

int32_t dspFilterCaclCoefsMultiple(char ftype, float fs, float F, float G, dspFilterCoefs &coefs);

template<typename TCoefs>
//convert filter coeffcient, from float record to integer record, either Int or VPU
unsigned dspFilterCoefsConvert(dspFilterCoefs cfloat[], TCoefs cint[], const int order, const unsigned int mant, bool calcSHL, bool reduceA1 = false) {
    bool over = false;
    int shltot = 0;
    for (int i=0; i < order; i++, cint++, cfloat++ ) {
        cint->reset();
        int shl = calcSHL ? cfloat->calcShl(mant) : 0;
        dspInt b0(cfloat->b0, mant - shl);
        over |= b0.over;
        cint->b0 = b0;
        dspInt b1(cfloat->b1, mant - shl);
        over |= b1.over;
        cint->b1 = b1;
        dspInt b2(cfloat->b2, mant - shl);
        over |= b2.over;
        cint->b2 = b2;
        dspInt a1(cfloat->a1 - ( reduceA1 ? 1.0 : 0.0), mant - shl);
        over |= a1.over;
        cint->a1 = a1;
        dspInt a2(cfloat->a2, mant - shl);
        over |= a2.over;
        cint->a2 = a2;
        if (calcSHL && (i==(order-1))) cint->zero = mant - shltot;
    }
    return calcSHL ? shltot : over;
}


#endif // _DSP_FILTER_CALC_H_
