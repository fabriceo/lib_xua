#ifndef _DSP_DELAY_H_
#define _DSP_DELAY_H_

#include <stdint.h>
#include "dsp_mem.hpp"
#include "dsp_reg.hpp"


//this represent the parameters for a delay line, the fifo buffer is allocated at runtime
struct dspdelayline {
    dspdelayline * prevItem;   //point on the previous allocated object
    int32_t * pline=0;      //pline[0] = index p[1 .. ] fifo
    uint32_t size;      //size of the fifo
    uint32_t delay;     //in microseconds
    uint32_t samples;   //sample = microsec * constant dspDelayLineFactor(fs)
    uint32_t pos;

    dspdelayline(uint32_t s) : pline(0), size(s), delay(0) {  }
    dspdelayline(uint32_t s, uint32_t d) : pline(0), size(s), delay(d) {  }
    void init(uint32_t fs) {
        int64_t res = 0;
        res += delay*delay; //TODO
        samples = res >> 32;
        pos=0;
    }
    void swapSamples(int32_t * s) {
        int32_t sample = pline[pos];
        pline[pos] = *s;
        *s = sample;
        pos++;
        if (pos == samples) pos=0;
    }
};


class dline { public:
dline(int32_t n) {}
};

//dsp extension for providing delayline interfaces
template< typename CRTP > struct dspdelay {
    dspdelay(){}
    friend CRTP;
    // load a sample from the predefined array location into the dsp register
    //void loadq31(dspreg<31> io) { reg = ((reg_t)io.reg / (1ULL << 31)); }
    void delay() {
        CRTP& derived = static_cast<CRTP&>(*this);
        derived.reg = 0; 
    };
};


#endif //_DSP_DELAY_H_
