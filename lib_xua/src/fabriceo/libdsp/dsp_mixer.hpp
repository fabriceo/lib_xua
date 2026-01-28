#ifndef _DSP_MIXER_H_
#define _DSP_MIXER_H_

#include <stdint.h>
#include "dsp_reg.hpp"


//define an object containing a referenc to an input sample (q31) and to a gain for this input
template< int32_t M = param_q > class dspmixerbase { public:
    using param_t = typename dsptype<M>::_t;
    int32_t input;
    param_t gain;
    dspmixerbase(int32_t i, double x) : input(i),gain(x) { }

};

//geenrate additiona method for dspbase
template< typename CRTP, int32_t M = param_q > struct dspmixer {
    dspmixer() {}
    friend CRTP;
    using param_t = typename dsptype<M>::_t;
    int32_t input;
    param_t gain;
    dspmixer(int32_t i, double x) : input(i),gain(x) { }

    void mix() {
        CRTP& derived = static_cast<CRTP&>(*this);
        derived.reg = 0; 
    };


};

static __attribute__ ((unused)) void  MIXER_TEST() { //syntax and compilation checks

    dspreg<24> gains[2] = { 3.0, 4.0 };
    (void)gains;
    dspmixerbase<28> table_mixer[2] = {
        { 1, 0.7f },
        { 2, 0.3f },
    };
    (void)table_mixer;
}


#if 0
//10 registers
	{ add filters,filters,r11 ; add states,states,c0 }	//add 24 or 16 to point on next state area
	ldd ah,al,states[0]					// get "remainder" from last calculation
	ldd c1, c0, filters[0] 				// c0=b0 and c1=b1
	maccs ah, al, r0, c0 				// x[n] * b0

	maccs ah, al, s1, c1				// s1 = x[n-1], compute x[n-1] * b1
	ldd c0, c1, filters[1] 				// c1=b2 and c0=a1
	maccs ah, al, s2, c1 				// s2 = x[n-2](=yn-2 from previous loop), compute x[n-2] * b2
	ldd s2, s1, states[1]	 			// s1 = y[n-1]  and s2 =y[n-2]

	maccs ah, al, s1, c0				// y[n-1] * a1	this coef is reduced by 1.0 due to remainder integration
	{ ldw c1, filters[4] ; sub sections,sections,1 } // c1 = a2
	maccs ah, al, s2, c1				// y[n-2] * a2
	lsats ah, al, r10					// saturate. mandatory to avoid oscillation

	std ah,al,states[0]					// save accumulator for reminder re-integration at next cycle
	lextract r0, ah, al, r10, 32		// compute Yn : remove BQ coefs precision get back to original sample precision 4.28
	std s1, r0, states[1]				// store into y[n-1] and y[n-2]
	{ ldc c0,16 ; bt sections, dsp_BIQUADS_loop }	// leave if finished, c0 used for incrementing states
#endif


#endif //_DSP_MIXER_H_
