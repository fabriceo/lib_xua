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
dspFilterCoefs *  dspFilterCoefsCopy(dspFilterCoefs * pdest,dspFilterCoefs *psource, uint32_t format) {
    while (1) {
        if (format == fp32) 
            for (int j=0; j<5; j++) {
                pdest->b0 = psource->b0; pdest->b1 = psource->b1; pdest->b2 = psource->b2;
                pdest->a1 = psource->a1; pdest->a2 = psource->a2;
            }
        else {
            int32_t * pdi = (int32_t*)pdest;
            pdi[0] = q32_float(psource->b0,format);
            pdi[1] = q32_float(psource->b1,format);
            pdi[2] = q32_float(psource->b2,format);
            pdi[3] = q32_float(psource->a1,format);
            pdi[4] = q32_float(psource->a2,format);
        }
        pdest++;
        //test if this filter has another section just after
        psource++;
    }
    return pdest;
}

#endif

float *  dspFilterCoefsCalcSlew(float * pdest, float *psource, uint32_t format) {
    const float slewFactor = 1.0/32;
    while (1) {
        if (format == fp32) {
            float * pd = pdest;
            float * ps = psource;
            for (int i =0; i<5; i++) {
                float delta = (pd[i] - ps[i]);
                delta *= slewFactor;
                pd[i] += delta;  
            }
        } else {
            int32_t * pd = (int32_t*)pdest;
            int32_t * ps = (int32_t*)psource;
            for (int i =0; i<5; i++) {
                int32_t delta = (pd[i] - ps[i]);
                delta >>= 5;
                pd[i] += delta;  
            }
        }
        pdest++;
        psource++;
    }
    return pdest;
}



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
const char * dsp_filter_name[] = { 
  "FNONE","LPBE2","LPBE3","LPBE4","LPBE5","LPBE6","LPBE7","LPBE8",
  "     ","HPBE2","HPBE3","HPBE4","HPBE5","HPBE6","HPBE7","HPBE8",
  "     ","LPBe2","LPBe3","LPBe4","LPBe5","LPBe6","LPBe7","LPBe8",
  "     ","HPBe2","HPBe3","HPBe4","HPBe5","HPBe6","HPBe7","HPBe8",
  "     ","LPBU2","LPBU3","LPBU4","LPBU5","LPBU6","LPBU7","LPBU8",
  "     ","HPBU2","HPBU3","HPBU4","HPBU5","HPBU6","HPBU7","HPBU8",
  "     ","LPLR2","LPLR3","LPLR4","     ","LPLR6","     ","LPLR8",
  "     ","HPLR2","HPLR3","HPLR4","     ","HPLR6","     ","HPLR8",
  "LP1  ","LP2  ","HP1  ","HP2  ","LS1  ","LS2  ","HS1  ","HS2  ",
  "AP1  ","AP2  ","PEAK ","NOTCH","BP0DB","BPQ  ","HILB ","LT   "
};


//compute filters order based on ftype
//expected to be optimized at compile time
static inline uint32_t dspFilterGetOrder(const char f) {
    const char sections[] = {  1,2,1,2,1,2,1,2, 1,2,2,2,2,2,4,2 };
    if (f <= FHPLR8) return 1+(f & 7);
    else return sections[f-FLP1];
}

//helpers normally from math.h
#ifndef M_SQRT1_2
#define M_SQRT1_2 (0.707106781186547524400844362104849039)
#endif
#ifndef M_PI
#define M_PI (3.14159265358979323846264338327950288)
#endif


    //this table provide a list of elementary filters to create 
    //Bessels 2/3/4/6/8, Butterworth or Linkwitz rilley filters
    struct filtersTable_s { char ftype; char btype; float F; float Q; } filtersTable[] = {
    { FLPBE2,       FLP2, 1.0               , 0.57735026919 },
    { FLPBE3db2,    FLP2, 1.27201964951     , 0.57735026919 },
    { FHPBE2,       FHP2, 1.0               , 0.57735026919 },
    { FHPBE3db2,    FHP2, 1.0/1.27201964951 , 0.57735026919 },
    { FLPBU2,       FLP2, 1.0               , M_SQRT1_2 },
    { FHPBU2,       FHP2, 1.0               , M_SQRT1_2 },
    { FLPLR2,       FLP2, 1.0               , 0.5 },
    { FHPLR2,       FHP2, 1.0               , 0.5 },

    { FLPBE3,       FLP2, 0.94160002653     , 0.691046625825 },
    { FLPBE3,       FLP1, 1.03054454544     , 0.0 },
    { FLPBE3db3,    FLP2, 1.32267579991     , 0.691046625825 },
    { FLPBE3db3,    FLP1, 1.44761713315     , 0.0 },
    { FHPBE3,       FHP2, 1.0/0.941600026533, 0.691046625825 },
    { FHPBE3,       FHP1, 1.0/1.03054454544 , 0.0 },
    { FHPBE3db3,    FHP2, 1.0/1.32267579991 , 0.691046625825 },
    { FHPBE3db3,    FHP1, 1.0/1.44761713315 , 0 },
    { FLPBU3,       FLP2, 1.0               , 1.0 },
    { FLPBU3,       FLP1, 1.0               , 0.0 },
    { FHPBU3,       FHP2, 1.0               , 1.0 },
    { FHPBU3,       FHP1, 1.0               , M_SQRT1_2 },
    { FLPLR3,       FLP2, 1.0               , 0.5 },
    { FLPLR3,       FLP1, 1.0               , 0.0 },
    { FHPLR3,       FHP2, 1.0               , 0.5 },
    { FHPLR3,       FHP1, 1.0               , 0.5 },

    { FLPBE4,       FLP2, 0.944449808226    , 0.521934581669 },
    { FLPBE4,       FLP2, 1.05881751607     , 0.805538281842 },
    { FLPBE3db4,    FLP2, 1.43017155999     , 0.521934581669 },
    { FLPBE3db4,    FLP2, 1.60335751622     , 0.805538281842 },
    { FHPBE4,       FHP2, 1.0/0.944449808226, 0.521934581669 },
    { FHPBE4,       FHP2, 1.0/1.05881751607 , 0.805538281842 },
    { FHPBE3db4,    FHP2, 1.0/1.43017155999 , 0.521934581669 },
    { FHPBE3db4,    FHP2, 1.0/1.60335751622 , 0.805538281842 },
    { FLPBU4,       FLP2, 1.0               , 0.54119610 },
    { FLPBU4,       FLP2, 1.0               , 1.3065630 },
    { FHPBU4,       FHP2, 1.0               , 0.54119610 },
    { FHPBU4,       FHP2, 1.0               , 1.3065630 },
    { FLPLR4,       FLP2, 1.0               , M_SQRT1_2 },
    { FLPLR4,       FLP2, 1.0               , M_SQRT1_2 },
    { FHPLR4,       FHP2, 1.0               , M_SQRT1_2 },
    { FHPLR4,       FHP2, 1.0               , M_SQRT1_2 },

    { FLPBE6,       FLP2, 0.928156550439    , 0.510317824749 },
    { FLPBE6,       FLP2, 0.977488555538    , 0.611194546878 },
    { FLPBE6,       FLP2, 1.10221694805     , 1.02331395383 },
    { FLPBE3db6,    FLP2, 1.60391912877     , 0.510317824749 },
    { FLPBE3db6,    FLP2, 1.68916826762     , 0.611194546878  },
    { FLPBE3db6,    FLP2, 1.9047076123      , 1.02331395383 },
    { FHPBE6,       FHP2, 1.0/0.928156550439, 0.510317824749 },
    { FHPBE6,       FHP2, 1.0/0.977488555538, 0.611194546878 },
    { FHPBE6,       FHP2, 1.0/1.10221694805 , 1.02331395383 },
    { FHPBE3db6,    FHP2, 1.0/1.60391912877 , 0.510317824749 },
    { FHPBE3db6,    FHP2, 1.0/1.68916826762 , 0.611194546878 },
    { FHPBE3db6,    FHP2, 1.0/1.9047076123  , 1.02331395383 },
    { FLPBU6,       FLP2, 1.0               , 0.51763809 },
    { FLPBU6,       FLP2, 1.0               , M_SQRT1_2 },
    { FLPBU6,       FLP2, 1.0               , 1.9318517 },
    { FHPBU6,       FHP2, 1.0               , 0.51763809 },
    { FHPBU6,       FHP2, 1.0               , M_SQRT1_2 },
    { FHPBU6,       FHP2, 1.0               , 1.9318517 },
    { FLPLR6,       FLP2, 1.0               , 0.5 },
    { FLPLR6,       FLP2, 1.0               , 1.0 },
    { FLPLR6,       FLP2, 1.0               , 1.0 },
    { FHPLR6,       FHP2, 1.0               , 0.5 },
    { FHPLR6,       FHP2, 1.0               , 1.0 },
    { FHPLR6,       FHP2, 1.0               , 1.0 },

    { FLPBE8,       FLP2, 0.920583104484    , 0.505991069397 },
    { FLPBE8,       FLP2, 0.948341760923    , 0.559609164796 },
    { FLPBE8,       FLP2, 1.01102810214     , 0.710852074442 },
    { FLPBE8,       FLP2, 1.13294518316     , 1.22566942541 },
    { FLPBE3db8,    FLP2, 1.77846591177     , 0.505991069397 },
    { FLPBE3db8,    FLP2, 1.8320926012      , 0.559609164796  },
    { FLPBE3db8,    FLP2, 1.95319575902     , 0.710852074442 },
    { FLPBE3db8,    FLP2, 2.18872623053     , 1.22566942541 },
    { FHPBE8,       FHP2, 1.0/0.920583104484, 0.505991069397 },
    { FHPBE8,       FHP2, 1.0/0.948341760923, 0.559609164796 },
    { FHPBE8,       FHP2, 1.0/1.01102810214 , 0.710852074442 },
    { FHPBE8,       FHP2, 1.0/1.13294518316 , 1.22566942541 },
    { FHPBE3db8,    FHP2, 1.0/1.77846591177 , 0.505991069397 },
    { FHPBE3db8,    FHP2, 1.0/1.8320926012  , 0.559609164796 },
    { FHPBE3db8,    FHP2, 1.0/1.95319575902 , 0.710852074442 },
    { FHPBE3db8,    FHP2, 1.0/2.18872623053 , 1.22566942541 },
    { FLPBU8,       FLP2, 1.0               , 0.50979558 },
    { FLPBU8,       FLP2, 1.0               , 0.60134489 },
    { FLPBU8,       FLP2, 1.0               , 0.89997622 },
    { FLPBU8,       FLP2, 1.0               , 2.5629154 },
    { FHPBU8,       FHP2, 1.0               , 0.50979558 },
    { FHPBU8,       FHP2, 1.0               , 0.60134489 },
    { FHPBU8,       FHP2, 1.0               , 0.89997622 },
    { FHPBU8,       FHP2, 1.0               , 2.5629154 },
    { FLPLR8,       FLP2, 1.0               , 0.54119610 },
    { FLPLR8,       FLP2, 1.0               , 1.3065630 },
    { FLPLR8,       FLP2, 1.0               , 0.54119610 },
    { FLPLR8,       FLP2, 1.0               , 1.3065630 },
    { FHPLR8,       FHP2, 1.0               , 0.54119610 },
    { FHPLR8,       FHP2, 1.0               , 1.3065630 },
    { FHPLR8,       FHP2, 1.0               , 0.54119610 },
    { FHPLR8,       FHP2, 1.0               , 1.3065630 },
    { FNONE }   //end of table indicator
};//table


//compute a set of coefficient for a simple 1st or 2nd order fiter type, at given fs (sampling rate)
//coefficient will be soredt in ptr->b0...a2
int32_t dspFilterCaclCoefs(char ftype, float fs, float F, float Q, float G, float * ptr) {

    typedef float f_t;
    f_t tw2, a0, alpha, w0, cw0, sw0;
    f_t b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    int32_t order = dspFilterGetOrder(ftype);
    if (order == 0) {
        b0 = 1.0f; 
    } else if (order == 1) {                //stability condition : abs(a1)<1
        tw2 = tan(M_PI * F / fs); 
        w0 = cw0 = sw0 = 0.0f;
    } else {                                // stability condition : abs(a2)<1 && abs(a1)<(1+a2)
        tw2 = 0.0f;
        w0  = M_PI * 2.0f * F / fs;  
        cw0 = cos(w0);
        sw0 = sin(w0);
        if (Q != 0.0f) alpha = sw0 / 2.0 / Q; else alpha = 1.0;
        a0 = (1.0f + alpha);
        a1 = -(-2.0f * cw0) / a0;            // sign is changed to accomodate convention
        a2 = -(1.0f - alpha ) / a0;          // and coeficients are normalized vs a0
    }

    switch (ftype) {

    case FLP1: { 
        alpha = 1.0f + tw2;
        a1 = ( 1.0f - tw2 ) / alpha;
        b0 = tw2 / alpha * G;
        b1 = b0;
        break; }

    case FHP1: { 
        alpha = 1.0f + tw2;
        a1 = ( (1.0f-tw2) / alpha );
        b0 =  1.0f / alpha * G;
        b1 = -b0;
        break; }

    case FLS1: { 
        f_t A = sqrt(G);
        a0 = tw2 + A;
        a1 = -( tw2 - A ) / a0;
        b0 =  ( G * tw2 + A) / a0;
        b1 =  ( G * tw2 - A) / a0;
        break; }

    case FHS1: { 
        f_t A = sqrt(G);
        a0 = A * tw2 + 1.0f;
        a1 = -( A * tw2 - 1.0f ) / a0;
        b0 =  ( A * tw2 + G )   / a0;
        b1 =  ( A * tw2 - G )   / a0;
        break; }

    case FAP1: { 
        alpha = (tw2 - 1.0f) / (tw2 + 1.0f) ;
        a1 = -alpha;
        b0 = alpha * G;
        b1 = G ;
        break; }

    case FLP2: { 
        b1 = (1.0f - cw0) / a0 * G;
        b0 = b1 / 2.0f;
        b2 = b0;
        break; }

    case FHP2: { 
        b1 = -(1.0f + cw0) / a0 * G;
        b0 = - b1 / 2.0f;
        b2 = b0;
        break; }

    case FAP2: { 
        b0 = -a2 * G;
        b1 = -a1 * G;
        b2 =  G;
        break; }

    case FNOTCH: { 
        b0 = 1.0f / a0 * G;
        b1 = -a1 * G;
        b2 = b0;
        break; }

    case FBP0DB: { 
        b0= alpha / a0 * G;
        b1 = 0;
        b2 = -b0;
        break; }

    case FBPQ: { 
        b0= sw0/2.0f / a0 * G;
        b1 = 0.0f;
        b2 = -b0;
        break; }

    case FLS2: { 
        f_t A = sqrt(G);
        f_t sqA = sqrt( A );
        a0 = ( A + 1.0f) + ( A - 1.0f ) * cw0 + 2.0 * sqA * alpha;
        a1 = -(-2.0f *( (A-1.0f) + (A+1.0f) * cw0 ) ) / a0;
        a2 = -( (A + 1.0f) + (A - 1.0f) * cw0 - 2.0f * sqA * alpha ) / a0;
        b0 = ( A * ( ( A + 1.0f) - ( A - 1.0f) * cw0 + 2.0f * sqA * alpha ) ) / a0;
        b1 = ( 2.0f * A * ( ( A - 1.0f ) - ( A + 1.0f ) * cw0 ) ) / a0;
        b2 = ( A * ( ( A + 1.0f ) - ( A - 1.0f ) * cw0 - 2.0f * sqA * alpha )) / a0;
        break; }

    case FHS2: { 
        f_t A = sqrt(G);
        f_t sqA = sqrt( A );
        a0 = ( A + 1.0f) - ( A - 1.0f ) * cw0 + 2.0f * sqA * alpha;
        a1 = -(2.0f *( (A-1.0f) - (A+1.0f) * cw0 ) ) / a0;
        a2 = -( (A + 1.0f) - (A - 1.0f) * cw0 - 2.0f * sqA * alpha ) / a0;
        b0 = ( A * ( ( A + 1.0f) + ( A - 1.0f) * cw0 + 2.0f * sqA * alpha ) ) / a0;
        b1 = ( -2.0f * A * ( ( A - 1.0f ) + ( A + 1.0f ) * cw0 ) ) / a0;
        b2 = ( A * ( ( A + 1.0f ) + ( A - 1.0f ) * cw0 - 2.0f * sqA * alpha )) / a0;
        break; }

    case FPEAK: { 
        f_t A = sqrt(G);
        a0 = 1.0f + alpha / A;
        a1 = 2.0f * cw0 / a0;
        a2 = -(1.0f - alpha / A ) / a0;
        b0 = (1.0f + alpha * A)   / a0;
        b1 = -2.0f * cw0 / a0;
        b2 = (1.0f - alpha * A)   / a0;
        break; }

    default : //fallthrough
    case FNONE : { b0 = 1.0f; b1 = b2 = a1 = a2 = 0.0f; break; }
    }
    //update target record
    ptr[0] = b0; ptr[1] = b1; ptr[2] = b2; 
    ptr[3] = 0;  ptr[4] = a1; ptr[5] = a2;
    return 1;
}

//compute a set of coefficient for a linkwitz rilley fiter type, at given fs (sampling rate)
int32_t dspFilterCaclCoefsLT(char ftype, float fs, float F, float Q, float G, float Fp, float Qp, float * ptr) {

    float a0,c0,c1,d0,d1,fc,gn,gn2;
    d0 = (2.0f * M_PI * F  ); d0 *= d0;
    c0 = (2.0f * M_PI * Fp ); c0 *= c0;
    d1 = (2.0f * M_PI * F  / Q);
    c1 = (2.0f * M_PI * Fp / Qp);
    fc = (F + Fp) / 2.0f;
    gn = 2.0f * M_PI * fc / tan( M_PI * fc/fs);
    gn2 = gn * gn;
    a0 = c0 + gn * c1 + gn2;
    ptr[0] = (d0 + gn * d1 + gn2 ) / a0 * G;
    ptr[1] = 2.0f * (d0 - gn2)     / a0 * G;
    ptr[2] = (d0 - gn * d1 + gn2)  / a0 * G;
    ptr[3] = 0.0f;
    ptr[4] = - 2.0f * (c0 - gn2)   / a0;
    ptr[5] = - (c0 - gn * c1 + gn2)/ a0;
    return 1;
}

int32_t dspFilterCaclCoefsMultiple(char ftype, float fs, float F, float G, float * pcoefs) {
    filtersTable_s * pf = filtersTable;
    float gain = G;
    do {
        if (pf->ftype == ftype) {
            //multiple section? : apply gain on first one only if G<1.0
            if (pf[1].ftype == ftype) gain = (fabs(G)>=1.0?1.0:G);       
            while (1) {
                dspFilterCaclCoefs(pf->btype, fs, F * pf->F, pf->Q, gain, pcoefs);
                if (pf[1].ftype != ftype) return 1;   //finished
                //next one is same type
                if (pf[2].ftype == ftype) gain = 1.0;
                else gain = (fabs(G)>=1.0?G:1.0);
                pcoefs++;
                pf++; 
            } 
        }
        pf++;
    } while (pf->ftype != FNONE);
    return 0;
}


#endif // _DSP_FILTER_CALC_H_
