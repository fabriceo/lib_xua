#ifndef _DSP_BASE_H_
#define _DSP_BASE_H_

#include <stdint.h>
#include <math.h>

#if defined(__XS2__) || defined(__XS3__)
#include <xs1.h>
#include <platform.h>
#endif


enum {  fp32 = 32, fp64 = 64, ff32 = 0, //do not change 


};

//eventually change this type to double, to augment precision for 
//parameters given in the user dsp program (gains, filter parameters or constant values)
//generic floating point type across all the library
typedef float fp_t; 


//default type value for any parameters or gain
#ifndef param_q
#define param_q (fp32)
#endif

//default type value for biquads coefficients
#ifndef fcoef_q
#define fcoef_q (fp32)
#endif

#include "dsp_convert.hpp"
#include "dsp_reg.hpp"
#include "dsp_io_mem.hpp"
#include "dsp_list.hpp"
#include "dsp_mem.hpp"
#include "dsp_slew.hpp"
#include "dsp_gain.hpp"
#include "dsp_mixer.hpp"
#include "dsp_delay.hpp"
#include "dsp_filter.hpp"
//#include "dsp_biquad.hpp"

/*
 * 
 * 
 *
*/
template<   
    //N = confugration of the accumulator 0..64
    int32_t N,             
    //configuration of any parameters like gains, 1..32
    int32_t M = param_q ,   
    //configuration for the filter coefficients 1..32 (fir can be differents)
    int32_t L = fcoef_q ,   
    //configuration for the IO (q32 or float only)
    int32_t K = 31 >  //most of the time, samples are coded -1..+1 in signed q31 format

struct  avdsp :     dspreg< N >,
        //other inherited classes providing additional methods via crtp
        dspiomem  <  avdsp< N,M,L,K >, N, K >,
        dspgain   <  avdsp< N,M,L,K >, M  >,
        dspdelay  <  avdsp< N,M,L,K >     >,
        dspmixer  <  avdsp< N,M,L,K >, M  >,
        dspfilter <  avdsp< N,M,L,K >, L  >  {
 
    static_assert(  
        ((N>=0)&&(N<=64)) && 
        ((M>1)&&(M<=32))  &&
        ((L>1)&&(L<=32))  &&
        (M==32?(L==32):true) &&
        (L==32?(M==32):true) &&
        ((K==31)||(K==32)) , "bad avdsp<> parameters");

    using   dspreg< N >::reg;
    using   dspReg     = dspreg< N >;
    using   dspParam   = dspreg< M >;
    using   dspCoefbq  = dspreg< L >;
    using   dspIo      = dspreg< K >;
    typedef typename dsptype< N >::_t  reg_t;       //same as dsp_reg_t from inherited class
    typedef typename dsptype< M >::_t  param_t;     //will be use for any parameters like gains
    typedef typename dsptype< L >::_t  coefbq_t;    //will be used for fliter's coefficients
    
    avdsp& clear() { reg = 0; return *this; };

    avdsp() { }    //default constructor

    avdsp& operator = (dspreg< N >& rhs) {
        if (&rhs != this) reg = rhs;
        return *this;
    }

    void convfp(fp_t fp)          { this->setfp(fp); }
    //void convint(int64_t val, int32_t m) { this->convintN(val, m); }
    void convparam(dspreg< M >& r)  { reg.conv< M >(r); } //reg = this->template conv<M>(r);
    void convio(dspreg< K >& r)     { reg.conv< K >(r);} 

    dspreg< K > convtoio() {
        dspreg< K > temp;
        temp.template conv< N >(*this);
        return temp;
    }

    avdsp& operator = (dspreg< M + (M==N)*100 >& rhs) {   //avoid double method definition by potentially adding 100
        convparam(rhs);
        return *this;
    }

    //load imediate value, compiler should calculate everything at compile time where feasible
    avdsp&  loadfp(fp_t fp)         { this->setfp(fp); return *this; }
    
    //simply adding an immediate value to the accumulator, expecting compiler to optimize
    avdsp&  addfp(fp_t fp)          {
        if (N==ff32) {  } //TODO
        else if ((N==fp32)||(N==fp64))  reg += fp;  
        else { 
            fp *= (1ULL << N);   
            reg += fp; }
        return*this;
    }
    //add a register of same size than this dsp object
    avdsp&  add(dspreg< N >& r)         { 
        reg += r.reg;         
        return *this; }

    //add a register of size corresponding to a parameter or gain
    avdsp&  addparam(dspreg< M >& r)    { 
        dspreg< N > temp; temp.conv< M >(r);
        reg += temp; return*this; }
#if 0
    //add a register of size corresponding to an IO (or sample)
    avdsp&  addio(dspreg< K >& r)     { 
        dspreg< N > temp; temp.conv< K >(r);
        reg += temp; return*this; }
#endif

    //only supported when adding exact same type of register
    avdsp& operator+ (dspreg< N >& rhs) {  
        reg+= rhs; return *this; }

    avdsp&  shift(int32_t s)             { 
        if (s == 0) { }
        else if (N==ff32) { }    //TODO
        if ((N==fp32)||(N==fp64)) { //applying on a floating point
            if (s > 0) {
                float f = 1ULL << s;
                reg *= f;
            } else {
                float f = 1ULL << -s;
                reg /= f;
            }
        } else { //applying on a integer 32 or 64 bits
            if (s > 0) reg <<= s;   //compiler standard routines
            else       reg >>= -s;
        }
        return *this;
    }

    avdsp& operator<< (int32_t s) {  shift(s);  return *this; }
    avdsp& operator>> (int32_t s) {  shift(-s); return *this; }

    //multiplying by a floating point
    avdsp&  mulfp(fp_t fp)          {
        if (N==ff32) { } //TODO
        else if ((N==fp32)||(N==fp64))  reg *= fp;  //compiler will manage
        else { //target reg is integer
            //TODO could be optimized and refined
            fp_t temp = reg;    //using compiler conversion
            temp *= fp;         //multiply in the float domain
            reg = temp;         //re-using compiler conversion
        }
        return*this;
    }

    //multiplying by a register of same type
    avdsp&  mul(dspreg< N >& rhs)          {
        if (N==ff32) { } //TODO
        else if ((N==fp32)||(N==fp64))  
            reg *= rhs.reg;  //compiler will manage
        else { //reg is integer 32 or 64
            if (N < 32) {
                int64_t res = reg;
                res *= rhs.reg;     //result is 64 bits with integer part poubled
                res <<= (32-N);
                reg = res >> 32;    //keep msb part only
            } else if (N < 64) {
                //TODO 64 x 64 => 128 bits, keep highest only
            }
        }
        return*this;
    }

    avdsp& operator* (dspreg< N >& rhs) { reg.mul(rhs); return *this; }

    avdsp&  mulparam(const dspreg< M >& param)     { 
        if (N==ff32) { } //TODO
        else if ((N==fp32)||(N==fp64)) { //expecting floating point result
            if ((M==fp32)||(M==fp64)) 
                reg *= param.reg; // full floating point
            else { //origin is integer
                reg *= (reg_t)param.reg;
                reg /= (1ULL << M );
            }
        } else { //expecting integer results
            if ((M==fp32)||(M==fp64)) { //origin float
                reg_t temp = reg;   //compiler convert to float
                temp *= param.reg;
                reg = temp;         //compiler convert to int32_t
            } else { //origin is integer
                //TODO needs optimization for 32x32 and 64x32
                float temp = reg;   //compiler convert to float
                temp *= param.reg;  //compiler convert to float
                temp /= 1 << M;     //remove excess
                reg = temp;         //compiler convert to int32_t
            }
        }
        return *this;
    }

    avdsp& operator* (dspreg< M + (M==N)*100 >& rhs) {  reg.mul(rhs); return *this; }

};




//object to use to instanciate a formal dsp core 
//as a specific thread on the xmos processor
//provides routines to orchestrate actions
template< typename CRTP > struct dspcore : dspFilter { 

    uint32_t randomNumber;
    uint32_t tpdfNumber;
    dspcore() {  };
    friend CRTP;
    
    void setup() {}         //default
    void change_FS() {}     //default
    void run() {
        // provide a run() for all childs
        CRTP& derived = static_cast<CRTP&>(*this);
        while (dsp_FS) {
            derived.setup();
            int32_t prev_FS = dsp_FS;
            while (prev_FS == dsp_FS) derived.loop();   //mandatory in the child class
            if (dsp_FS) derived.change_FS();
        }
    }
    void initRandom() {
        #if 0
        setps(0x060b, 0xF); //switch on ring-oscillators
        delay_microseconds(50);
        setps(0x060b, 0); 
        delay_ticks(10);
        random = getps(0x070b);
        #endif
    }
    void calcRandom() {
        const uint32_t poly = 0;
        (void)poly;
        //crc32(random,-1,0xEB31D82E);
    }
};

template< typename CRTP > struct dspMainCore : dspcore< CRTP > {
    
    dspMainCore()  { 
        dspcore< CRTP >::initRandom(); }
    ~dspMainCore() { }
};
template< typename CRTP > struct dspOtherCore : dspcore< CRTP > {
    dspOtherCore()  { }
    ~dspOtherCore() { }
};

void dspbaseBufferInit(uint32_t sampFreq) {

}

#endif //_DSP_BASE_H_
