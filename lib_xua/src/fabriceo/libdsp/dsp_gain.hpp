/*
 * dsp_gain.hpp
 *
 *  Created on: 2 nov. 2025
 *      Author: fabriceo
 */

#ifndef DSP_GAIN_HPP_
#define DSP_GAIN_HPP_

#include <stdint.h>
#include "dsp_reg.hpp"
#include "dsp_slew.hpp"

template< int32_t M > struct dspGain_{
    dspreg<M> val;
    dspGain_(fp_t fp) { val = fp; }
};
//gives global visibility on dspGain< >
template< int32_t M > struct dspGain : dspGain_<M> {
    dspGain(fp_t fp) : dspGain_<M>(fp) {}
};

template< int32_t M > struct dspGainSlew_ : dspGain_< M >{
    using dspGain_< M >::val;
    dspslew slew;
    dspreg<M> factor;
    dspGainSlew_() { reset(); }
    dspGainSlew_(fp_t fp) : dspGain_<M>(fp) { slew.reset(); }
    //operator= (dspGainSlew_<M>& rhs) { }
    void reset() { val=0; slew.reset(); }
    void set(float newval, float rate, int32_t time) { 
        dspreg<fp32> oldval;
        oldval.conv<M>(val);
        float f = slew.calcSlewFactor(oldval,newval,rate,time);
        factor.conv<fp32>(f);
    }
    void execslew() {
        //time slice approach
        if (slew.donow()) { 
            //val *= factor
        } else {
            //apply gain on dsp reg
        }
    }
};

template< int32_t M > struct dspGainSlew : dspGainSlew_<M>{
    dspGainSlew(fp_t fp) : dspGainSlew_<M>(fp) { }
    //operator= (dspGainSlew_<M>& rhs) { }
};

//this class is only used as an extension on the dspbase class
template< typename CRTP, int32_t M > struct dspgain {
    dspgain() {}
    friend CRTP;

    //same as global dspGain< > but with simplified syntax removing need for < >
    //visibility on this class is prioritized for objects inheriting dspbase
    class dspGain : dspGain_<M>{ public:
        using dspGain_<M>::val;
        dspGain& operator= (dspGain& g)   { 
            if (&g != this) val = g.val;
            return *this;
        }
    };

    //same as global dspGainSlew< > but with simplified syntax removing need for < >
    //visibility on this class is prioritized for objects inheriting dspbase
    class dspGainSlew : dspGainSlew_<M>{ public:
        using dspGainSlew_<M>::val;
        using dspGainSlew_<M>::slew;
        dspGainSlew(fp_t fp) : dspGainSlew_<M>(fp) { }
        //this operator will accept members declared globally
        dspGainSlew& operator= (dspGainSlew& gs)   { 
            if (&gs != this) { val = gs.val; slew = gs.slew; }
            return *this;
        }
    };

    CRTP& gainfp(fp_t g) {
        CRTP& child = static_cast<CRTP&>(*this);
        child.mulfp( g); 
        return child; };

    CRTP& gain(dspGain& g) {
        CRTP& child = static_cast<CRTP&>(*this);
        child.mulparam(g);
        return child; };

    CRTP& gainSlew(dspGainSlew& gslew) {
        CRTP& child = static_cast<CRTP&>(*this);
        if (child.donow()) {

        } else {
            child.mulparam(gslew);
        } 
        return child; };
};


#endif /* DSP_GAIN_HPP_ */
