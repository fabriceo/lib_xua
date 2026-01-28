#ifndef _DSP_SLEW_H_
#define _DSP_SLEW_H_

//helper class to manage slewrate
//
struct dspslew {
    int32_t  counter;
    int32_t  nexttime;
    int32_t  timeslew;
    int32_t  dspgettime() { 
        int32_t res; asm("gettime %0":"=r"(res)); return res; }
    void reset() { counter=0; }
    void set(int32_t tslew, int32_t count) { 
         timeslew = tslew; 
         nexttime = dspgettime()+tslew; 
         counter = count; 
    }
    int32_t  donow() {
         if (counter) {
            int32_t time = dspgettime();
            if ((time-nexttime)>0) { //after
                nexttime+=timeslew;
                counter--;
                return 1;
            }  
         }
         return 0;
    }
    //compute the number of operation required according to the given rate
    //rate is considered as a reduction per time stamp and ALWAYS below 1.0
    //eg -6db / sec = -0.6db per sec/10th = 10^-0.6 = 0.933
    //the routine compute the invert ratio if an augmentation is needed
    float calcSlewFactor(float current, float target, float rate, int32_t time ) {
        float factor;
        float ratio = target/current;
        if (ratio < 0.0f) {} // changement de signe!
        if (ratio >= 1.0f) { //augment
            factor = 1.0f/rate;
        } else { //reduce
            factor = rate;
        }
        float error = target*(1.0f-rate)/2.0f;
        //could use logarithm instead
        if (error<0.0f) error = -error;
        int32_t N = 0; 
        while(1) {
            N++;
            float next = current * factor;
            float delta = target-next;
            if (delta<0.0f) delta = -delta;
            if (delta<error) break;
        } 
        set(time, N);
        return factor;
    }
};
 


#endif //_DSP_SLEW_H_