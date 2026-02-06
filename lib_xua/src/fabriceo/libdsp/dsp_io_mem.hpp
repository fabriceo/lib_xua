
#ifndef _DSP_IO_H_
#define _DSP_IO_H_

#include <stdint.h>
#include "dsp_reg.hpp"
#include "dsp_convert.hpp"
#include "string.h"

//check the default value for the inputs & outputs. either q31 or simple float.
#ifndef io_q
#if defined(__XS2__) || defined(__XS3__)
#define io_q (31)
#else
#define io_q (fp32)
#endif
#endif

typedef typename dsptype< io_q >::_t  io_t;

//used to define a location inside the sample array.
//this is used to force labelling all input outputs with some type checking
#if 0
//original hook from lib_xua
void __attribute__ ((weak)) UserBufferManagement(uint32_t sampsFromUsbToAudio[], uint32_t sampsFromAudioToUsb[])
/* Default implementation for UserBufferManagementInit() */
void __attribute__ ((weak)) UserBufferManagementInit(uint32_t sampFreq)
#endif

/*
principles:
possibility to use the sample buffer to store temporary values across cores.
using 2 bin locations for supporting fp64 or q64

possibility to have buffer for 2x or 4x the original sample buffer

possibility to load sample from buffer(n-1,n-2,n-3,n-4) to synchronize timing when multiple cores are chained
output buffer is always singlesize

intercept buffman,
    get USB value and ADC value and store them in delayed buffer.
    write output value to DAC and USB
    increment buffer with rollover checking

local buffer example :

0..7    to DAC outputs 
8..15   to USB host

16..23  ADC inputs
24..31  from USB host
32..40  MEM_SPARE (64bits spaced)

16..40 repeated according to DSP_BUFFERS_NUM
*/

//TODO chnge all defines by proper memeber of a config structure....

#define XUA_DACS_SIZE       (4)     //number of DAC channels declared in lib_xua
#define XUA_USB_IN_SIZE     (2)     //number of channels comming from USB host
#define XUA_ADC_SIZE        (6)     //number of ADC channels + spdifrx+adattx
#define XUA_USB_OUT_SIZE    (2)     //number of channels sent to USB host

//definition four our local 32+8 bins buffer
//first buffer is for DAC outputs and spdif tx
#define DSP_POS_DAC         (0)     //pos of the DAC output in local sample array (always 0)
#define DSP_OUT_DACS        (4)
#define DSP_OUT_DACS_PAD    (4)     // 8 bins in total suggested here
//second buffer is for samples goint to usb host
#define DSP_POS_TO_USB      (DSP_OUT_DACS+DSP_OUT_DACS_PAD)
#define DSP_OUT_TO_USB      (2)     //pos of the sample going to the USB host in local sample array
#define DSP_OUT_TO_USB_PAD  (6)
#define DSP_OUT_TOTAL       (DSP_OUT_DACS+DSP_OUT_DACS_PAD+DSP_OUT_TO_USB+DSP_OUT_TO_USB_PAD)

//next buffer is containing ADC value (and spdifrx)
#define DSP_POS_ADC         (DSP_OUT_TOTAL) //pos of the ADC inputs in local sample array
#define DSP_IN_ADC          (6)
#define DSP_IN_ADC_PAD      (2)     // 8 bins
//4th buffer is for data sent by USB host
#define DSP_POS_FROM_USB    (DSP_POS_ADC+DSP_IN_ADC+DSP_IN_ADC_PAD)
#define DSP_IN_FROM_USB     (2)
#define DSP_IN_FROM_USB_PAD (6)     // 8 bins
#define DSP_IN_TOTAL        (DSP_IN_ADC+DSP_IN_ADC_PAD+DSP_IN_FROM_USB+DSP_IN_FROM_USB_PAD)

#define DSP_POS_MEM_SPARE   (DSP_POS_ADC + DSP_IN_TOTAL)
#define DSP_MEM_SPARE       (8) // 64bits storage to cover either float/double or int32_t/longlong
#define DSP_END_SPARE       (DSP_POS_MEM_SPARE+2*DSP_MEM_SPARE)

//define the number of buffers for all inputs and spare. this provides an inherent fifo process
#define DSP_BUFFER_1_SIZE   (DSP_END_SPARE)
#define DSP_BUFFERS_NUM     (4)
#define DSP_BUFFERS_SIZE    (DSP_BUFFERS_NUM * DSP_BUFFER_1_SIZE)

//locally used to manage the buffer index
//index A is used for sending value to lib xua (dac & usb host input) 
//index A is used by DSP to read inputs.
static int32_t dspBufferA = 0;  
//index B is used for stroring value comming from lib xua (adc & usb host output) 
//indexB is used by DSP to write output
static int32_t dspBufferB = 0;

#define DSP_SAMPLES_ARRAY_SIZE (DSP_BUFFERS_SIZE)

int dspSamplesArray[DSP_SAMPLES_ARRAY_SIZE];

//defined in dsp_base
void dspbaseBufferInit(uint32_t sampFreq);  

#if 0

extern "C" void UserBufferManagement(unsigned sampsFromUsbToAudio[], unsigned sampsFromAudioToUsb[]) {
    //called in between transfer from/to usb
    //possibility to trigger DSP here but need to verify/avoid jitter from  audiohub_st.xc

    asm("#UserBufferManagement:");
    int32_t target = dspBufferB += DSP_BUFFER_1_SIZE;
    if (target == DSP_BUFFERS_SIZE) target -= DSP_BUFFERS_SIZE;
    dspBufferA = dspBufferB; //switch dsp buffers
    dspBufferB = target;
    //gettime stamp
    //can trigger here.
    memcpy( &dspSamplesArray[DSP_POS_ADC+dspBufferB], sampsFromAudioToUsb, XUA_ADC_SIZE);
    memcpy( &dspSamplesArray[DSP_POS_FROM_USB+dspBufferA], sampsFromUsbToAudio, XUA_USB_IN_SIZE);
    memcpy( sampsFromAudioToUsb, &dspSamplesArray[DSP_POS_TO_USB+dspBufferA], XUA_USB_OUT_SIZE);
    memcpy( sampsFromUsbToAudio, &dspSamplesArray[DSP_POS_DAC+dspBufferB], XUA_DACS_SIZE);

}

extern "C" void UserBufferManagementInit(unsigned sampFreq) {
    dspBufferA = 0; dspBufferB = 0;
    memset( &dspSamplesArray, 0, sizeof(dspSamplesArray));
    dspbaseBufferInit(sampFreq);
}
#endif

//gives the possibility to define an Input or Output located in our local samples array.
//input area is buffered so it is possible to retreive previous samples to allign core timing
struct dspIO { 

    uint32_t idx = 0; int32_t pos = 0;
    //constructor will be optimized at compilation if user definitions are static
    dspIO(uint32_t x)  { 
        if (x < DSP_POS_MEM_SPARE) idx = x; }

    // prepare a base offset depending on p (negative value given)
    dspIO(int32_t x, int32_t p) : dspIO(x)  {     
        if ((-p)<DSP_BUFFERS_NUM) pos = p * DSP_BUFFER_1_SIZE; }

    //only change output space and do not use "pos"
    void setio(io_t val) {
        if (idx < DSP_OUT_TOTAL) {
            dspSamplesArray[dspBufferB+idx] = val; 
        }
    }

    //getio always returns the asis value stored in the sample array considered as io_t
    io_t getio() const {
        int32_t buff = pos + dspBufferA;
        if (buff < 0) buff += DSP_BUFFERS_SIZE;
        return dspSamplesArray[buff+idx];
    }

    //this operator return the value of the input as a floating point value between -1.0 ... +1.0
    operator float () { float fp = getio();  return fp / (1ULL << io_q); }

    // this operator assign a floating point value to the output
    dspIO& operator = (float rhs) {
        setio(rhs * (1ULL << io_q)); 
        return *this; }

    //this operator copy an IO object (idx,pos) to the left side of the =
    dspIO& operator = (const dspIO& rhs)     { 
        if (&rhs != this) { setio(rhs.getio()); }
        return *this; }

}; 

struct dspMem { 
    int32_t idx, pos = 0;
    //constructor will be optimized at compilation if user definitions are static

    dspMem(int32_t x) { 
        if (x < DSP_MEM_SPARE) idx = x; }

    dspMem(int32_t x, int32_t p) : dspMem(x)  { 
        if ((-p)<DSP_BUFFERS_NUM) pos = p * DSP_BUFFER_1_SIZE; }

    //provide the raw address of the memory location to be read depending on pos
    void * getReadAddr() const { 
        int32_t buff = dspBufferB + pos;
        if (buff < 0) buff += DSP_BUFFERS_SIZE;
        void * ptr = (void *)&dspSamplesArray[ buff + 2 * idx + DSP_POS_MEM_SPARE];  
        return ptr;
    }
    //provide the raw address of the memory location to be written (independant of pos)
    void * getWriteAddr() const { 
        return (void *)&dspSamplesArray[ dspBufferB + 2 * idx + DSP_POS_MEM_SPARE];  
    }

}; 


//dspIO SPDIF_RX_L(4),SPDIF_RX_R(4);
//dspMem mem0(0,0), mem0_1(0,-1);

//extensions for avdsp object
template< typename CRTP, int32_t N, int32_t K >  struct dspiomem {

    dspiomem(){}
    friend CRTP;

    // load a sample from the predefined array location into the dsp register
    CRTP& loadio(int32_t x) {
        CRTP& child = static_cast<CRTP&>(*this);
        dspIO io(x);
        child.template conv< K >(io.getio()); 
        return child;
    };
    CRTP& loadio(dspIO& io) {
        CRTP& child = static_cast<CRTP&>(*this);
        dspreg< K > regio;
        regio = io.getio();
        child.template conv< K >(regio); 
        return child;
    };
    CRTP& storeio(int32_t x) {
        CRTP& child = static_cast<CRTP&>(*this);
        dspIO io(x);
        dspreg< K > regio;
        regio.template conv< N >(child);
        io.setio(regio);
        return child;
    };
    CRTP& storeio(dspIO& io) {
        CRTP& child = static_cast<CRTP&>(*this);
        dspreg< K > regio;
        regio.template conv< N >(child);
        io.setio(regio);
        return child;
    };

    CRTP& addio(int32_t x) {
        CRTP& child = static_cast<CRTP&>(*this);
        dspIO io(x);
        child.addio(io.getio());
        return child;
    }
    CRTP& addio(const dspIO& rhs) {
        CRTP& child = static_cast<CRTP&>(*this);
        dspreg<K> ioval = rhs.getio();
        child.addio(ioval);
        return child;
    }

    CRTP& operator = (const dspIO& rhs) {
        CRTP& child = static_cast<CRTP&>(*this);
        return loadio(rhs);
    }

    // load the content of a memory location expected to have the dspformat
    CRTP& loadmem(void * ptr) {
        CRTP& child = static_cast<CRTP&>(*this);
        dspreg< N > * temp;
        temp = (dspreg< N > *)ptr;
        child.reg = *temp; 
        return child;
    };
    // load the content of a memory pointed by object reference
    CRTP& loadmem(dspMem& mem) {
        CRTP& child = static_cast<CRTP&>(*this);
        void * ptr = mem.getReadAddr();
        dspreg< N > * temp;
        temp = (dspreg< N > *)ptr;
        child.reg = *temp; 
        return child;
    };
    // store the dspregister to a memory location
    CRTP& storemem(void * ptr) {
        CRTP& child = static_cast<CRTP&>(*this);
        dspreg< N > * temp;
        temp = (dspreg< N > *)ptr;
        *temp = child.reg;
        return child;
    };
    // store the dspregister to a memory location
    CRTP& storemem(dspMem& mem) {
        CRTP& child = static_cast<CRTP&>(*this);
        void * ptr = mem.getWriteAddr();
        dspreg< N > * temp;
        temp = (dspreg< N > *)ptr;
        *temp = child.reg;
        return child;
    };
};

#endif //_DSP_IO_H_
