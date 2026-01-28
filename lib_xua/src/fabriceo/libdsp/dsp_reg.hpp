#ifndef _DSP_QNM_H_
#define _DSP_QNM_H_
//http://casteyde.christian.free.fr/cpp/cours/online/book1.html

#include <stdint.h>

typedef struct  {  int32_t m; int32_t e; }      ff32_t;
typedef struct  {  int64_t m; int32_t e; }      ff64_t;
typedef struct  {  uint32_t lo; int32_t hi; }   hl_t;



#if 0 //not used
typedef union u_64 {
    int64_t ll; uint64_t ull; 
    uint32_tu_[2]; int32_t i_[2]; 
    int32_t i; uint32_tu; 
    double d; float f; float f_[2];
    hl_t hl;
    em_t em;
    char c[8];
} t_64;
#endif


//used to verify if 2 types are identical (based on C++ std type_trait library )
template <class _Tp, class _Up> 
struct type_is_same            { static const bool value = false; };
template <class _Tp>            
struct type_is_same<_Tp, _Tp>  { static const bool value = true; };


//used to create a type depending on a condition (based on C++ std type_trait library )
template <bool COND, typename IF, typename THEN>
    struct dsp_cond_type {typedef IF type;};
template <typename IF, typename THEN>
    struct dsp_cond_type< false, IF, THEN> {typedef THEN type;};


//provide a class containing base types depending on < N >
template <int32_t N> struct dsptype {
    static_assert( (N>=0)||(N<=64),"bad value" );
    typedef                                     //storage type
    typename dsp_cond_type<     (N==32),        float,
    typename dsp_cond_type<     (N==64),        double,
    typename dsp_cond_type<     (N>0)&&(N<32),  int32_t,
    typename dsp_cond_type<     (N>32)&&(N<64), int64_t,
                                                ff32_t // N==0                               
        >::type >::type >::type >::type         _t; //base type
};

//provide a class containing base types depending on < N >
template <int32_t N> struct dsptype32 {
    static_assert( (N<=0)||(N>32),"bad value" );
    typedef typename dsp_cond_type< (N==32), float, int32_t >::type _t;
};


#include "dsp_convert.hpp"

//provide a class to describe a "register" depending on "N"
template< int32_t N >
struct dspreg : dsptype< N > {
    typedef typename dsptype< N >::_t dspreg_t;   //default type name for the corresponding dspreg<N>

    dspreg_t reg;                       //main register

    dspreg() { }
    dspreg(fp_t fp) { setfp(fp); }      //constructor initializing with a floating point value

    dspreg& operator= (dspreg& rhs) {   //simply copy rhs to our local reg. same characteristics
        if (&rhs != this) reg = rhs.reg;
        return *this; }

    dspreg& operator= (fp_t fp) { setfp(fp); return *this; }

    operator fp_t()  { return getfp(); }

    //provide the () operator to return the type of the register = mantissa for integers
    int32_t operator() ()   { return N; }

    fp_t getfp() const {
        if (N==ff32) return 0;                  //TODO
        if ((N==fp32)||(N==fp64)) return reg;   //compiler will convert eventually
        fp_t fp = reg;                          //compiler will convert 
        fp /= (1ULL << N);
        return fp;
    }

    void setfp(fp_t fp) {
        if (N==ff32) reg=0;                     //TODO
        else if ((N==fp32)||(N==fp64)) 
            reg = fp;                           //direct conversion in floating point
        else { 
            fp *= (1ULL << N); 
            reg = fp; }                         //conversion to integer by compiler
    }

    //convert a given register type to the current register type
    //compiler based conversion (expecting static value most of the time)
    template< int32_t G > void conv(dspreg< G >& r) {
        //conversion for all combination
        if ( N == G) reg = r.reg; //same types no conversion
        else if ((N==ff32)||(G==ff32)) { reg = 0; } //TODO special case for fast float
        else if ((N==fp32)||(N==fp64)) {    
            //expecting a floating point result
            if ((G==fp32)||(G==fp64)) reg = r.reg; // compiler will convert any float to any float
            else {
                //here we have an integer to convert to floatfing point
                dspreg_t f = 1ULL << G;
                reg = (dspreg_t)r.reg / f;      //using compiler default routines
            }
        }  else {                       
            //expecting an integer result
            if ((G==fp32)||(G==fp64)) { //from floating point
                reg = (r.reg * (1ULL << N));    //compiler will convert result to int32_t
            } else if (N == G) reg = r.reg;
            else if ( N < 32 ) {  //expecting q32
                 if ( G < 32) {      //same format
                    if (N < G) reg = (int32_t)r.reg >> (G-N);
                    else       reg = (int32_t)r.reg << (N-G); //TODO potential saturation 
                } else if ( G < 64 ) { //from q64
                    int32_t n_g = N-(G-32);
                    if (n_g == 0) { 
                        int32_t hi = (int64_t)r.reg >> 32;
                        reg = hi;
                    } else if (n_g < 0) {
                        int32_t hi = (int64_t)r.reg >> 32;   //extract msb only
                        reg = hi >> -n_g;
                    } else {
                        int64_t hi = (int64_t)r.reg << n_g;    //TODO potential saturation 
                        reg = hi >> 32;         //keep only msb
                    }
                }
            } else if (N < 64) {    //expecting q64
                if (N == G) reg = r.reg;
                else if ( G < 32 ) {     //from q32
                    int32_t n_g = (N-32)-G;
                    if (n_g == 0) reg = (int64_t)r.reg << 32;
                    else if (n_g < 0) {
                        int32_t hi = (int32_t)r.reg >> -n_g; 
                        uint32_t lo = (uint32_t)r.reg << (32+n_g);
                        reg = ((int64_t)hi << 32) | lo;
                    } else {
                        int32_t hi = (int32_t)r.reg << n_g; 
                        reg = ((int64_t)hi << 32);
                    }
                } else if (G < 64) { //from another q64
                    int32_t n_g = N-G;
                    if (n_g < 0) reg = (int64_t)r.reg >> -n_g;
                    else reg = (int64_t) r.reg << n_g; //TODO potential saturation 
                } 
            }
        }
    }
};


#if 0
    //convert a given integer with its mantissa to the current register type
    dspreg_t convintN_(int64_t val, int32_t G) {
        dspreg_t temp;
        if ((N==fp32)||(N==fp64)) {    
            //expecting a floating point result
            if (G < 32) { //given a q32
                if (N==fp32) qconv::float_q32(&temp, val, G);
                if (N==fp64) qconv::double_q32(&temp,val, G);
            } else {    //given a q64
                if (N==fp32) qconv::float_q64(&temp, val, G);
                if (N==fp64) qconv::double_q64(&temp,val, G);
            }
        }  else {                       
            //expecting an integer result
            if (N < 32) {         //expecting q32
                if      (G < 32) qconv::q32_q32(&temp, val, N-G);
                else if (G < 64) qconv::q32_q64(&temp, val, N-(G-32));
            } else if (N < 64) {  //expecting q64
                if      (G < 32) qconv::q64_q32(&temp, val, N-32-G);
                else if (G < 64) qconv::q64_q64(&temp, val, N-G);
            }
        }
        return temp;
    }

#endif

#endif //_DSP_QNM_H_
