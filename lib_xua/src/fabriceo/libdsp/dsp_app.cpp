
//#include "port_i2c.hpp"
//#include <xs1.h>

#include <stdint.h>

uint32_t chanend_1;

////////////////////////////////////////////////////////////
#include "debug_print.h"

#define DSP_MEM_SIZE 10000

volatile uint32_t dsp_FS = 0;   //all tasks running until fs != prev. leaving tasks when FS==0

#include "dsp_base.hpp"         //provides class to manage a multicore, configurable audio dsp solution

////////////////////////////////////////////////////////////
//user predefined configurations for 5 dsp engines
struct avdspfp32  : avdsp< fp32, fp32, fp32, 31 > { using AVDSP = avdspfp32;  };    //avdsp for float 32 bits
struct avdspfp64  : avdsp< fp64, fp32, fp32, 31 > { using AVDSP = avdspfp64;  };    //avdsp for double float 64 bits
struct avdspint64 : avdsp<   56,   24,   28, 31 > { using AVDSP = avdspint64; };    //avdsp for int32_t 64-56-24-28 bits
struct avdspint32 : avdsp<   28,   28,   28, 31 > { using AVDSP = avdspint32; };    //avdsp for int32_t 32-28 bits
struct avdspff64  : avdsp< ff32,   28,   28, 31 > { using AVDSP = avdspff64;  };    //fast float 32 bits experimental

//global definitions
dspIO  ADC(16), DAC(0), USBout(8), USBin(24);
dspMem core1mem(0);

dspGain<24> GGi(0);
dspGainSlew<24> GSi { 1.0f };

dspFilter f1 { FLPLR4, 400 };
dspFilter f2 { FHPLR4, 400 };
dspFilter f3 { FLPLR4, 2000 };
dspFilter f4 { FHPLR4, 2000 };

dspFilter fLow[]    = { f1, { FPEAK, 80, 0.7,   0.5 }, { F_ORDER_2 },{ F_ORDER_2 },{ F_ORDER_2 } };
dspFilter fMedium[] = { f2, { FPEAK, 1000, 2.0, 1.5 }, { F_ORDER_2 }, f3 };
dspFilter fHigh[]   = { { F_ORDER_4 }, { F_ORDER_2 }, { F_ORDER_2 }, { F_ORDER_2 }, { F_ORDER_4 }};

dspFilterState fLowLeft(  filtersArray(fLow) );
dspFilterState fLowRight( fLowLeft );

dspFilterState fMediumLeft(  filtersArray(fLow) );
dspFilterState fMediumRight( fMediumLeft );

struct Myprog1       : avdspfp32, dspMainCore< Myprog1 > {

    dspGainSlew GT { 1.0f };
    //objet filters
    //objects xx
    dspIO ADC_L { 8 }, DAC_L { 0 };

    void loop() {
        AVDSP ALU;   //define a local register for dsp actions
        ALU.clear();
        //ALU = ADC_L;
        ALU.loadio(ADC_L);
        ALU.filter(fLowLeft);
        ALU.storeio(DAC_L);


    }
    void setup() { 
    }
    void change_FS() { 
        //filterList.update();
        //filterStateList.update();
    }

    struct Core2           : avdspint64, dspOtherCore< Core2 > {

        //define multiple register for handling dsp functions
        AVDSP       X, Y, Z;
        dspParam    Gain = 1.0;
        dspReg      W;

        //user filters additional object declaration
        void loop() {
            X   .clear()
                .addfp(1.0);
            W = X;
            Y   .mulparam(Gain);
        }
        void setup() { 
            GSi.reset();
        }
        void change_FS() { }
    } core2;

} myprog1;


struct Prog2       : avdspfp32, dspMainCore< Prog2 > {
    dspIO  I1 { 9 }, O1 { 1 };
    AVDSP   DSP;
    void loop() {
        DSP.clear().loadio(I1).gainfp(0.5f).storeio(O1);
    }
    void setup() { }
    void change_FS() { }

} prog2;


volatile uint32_t currentProg = 1;

extern "C" void dsp_set_FS(uint32_t fs) { dsp_FS = fs; }
extern "C" void dsp_otherCore3() { };
extern "C" void dsp_otherCore2() { };
extern "C" void dsp_otherCore1() { 
while (1) { switch (currentProg) {
    case 1: { myprog1.core2.run(); }
} } };
extern "C" void dsp_mainCore()   { 
while (1) { switch (currentProg) {
    case 1: { myprog1.run(); }
    case 2: { prog2.run(); }
} } };

