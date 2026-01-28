#ifndef _DSP_BIQUAD_H_
#define _DSP_BIQUAD_H_

#include "dsp_mem.hpp"
#include "dsp_filter.hpp"

//a biquad object is defines as an array of filters, ad contains biquad coeffcients
class dsp_biquad {
    dsp_mem_item mem;           //point on the previous allocated object
    dsp_filter * fptr;          //point on associated filter bank definition
    uint32_t fnum;              //number of filters objects in the bank
    uint32_t * coefs;           //point on resulting biquads coefficients 
    uint32_t sections;          //total number of 2nd order biquad section for the filter bank
public:
    dsp_biquad() {  }
    //operator dsp_filter () { return get(); }
    dsp_biquad(dsp_filter ff[],uint32_t num) : fptr(ff),fnum(num) {  }
    void init() {  }
    void resetState() { }
    void calcCoefs(uint32_t fs) { resetState(); }
};


//define a biquad as above associated with a memory buffer to compute filter state at each samples.
class dsp_biquad_state {
    dsp_mem_item mem;            //point on the previous allocated object
    dsp_biquad * biquad;        //pointer of the biquad object attached to this instance      
    uint32_t * state;           //pointer on the states memory for all biquad sections
public:
    dsp_biquad_state(dsp_biquad& bq) : biquad(&bq), state(0) {}
};

template<int32_t m, int32_t n>
class dsp_biquad_calc { public:

};

void DSP_BIQUAD_TEST() {
//define the resulting biquad for the filter bank above
dsp_biquad  bq1( filter_bank(fbank1) );  //provide a table of biquad section based on filter bank

//create full biquad instance, with biquad coefficient and a state buffer for running it
dsp_biquad_state bq1_L(bq1), bq1_R(bq1);
}

#endif //_DSP_BIQUAD_H_
