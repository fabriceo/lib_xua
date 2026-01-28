#ifndef _DSP_MEM_H_
#define _DSP_MEM_H_

#include <stdint.h>
#include <stdlib.h> //malloc & free

//define the size (in bytes) for the preallocated area of memory (to avoid using malloc/free along life cycle)
#ifndef DSP_MEM_SIZE
#define DSP_MEM_SIZE 10000
#endif

//used to allocate and free memory required by user program within a specific prealocated area.

struct dspMalloc {
    uint32_t sizeallocated = 0;
    uint32_t sizeused = 0;
    void     * alloc = nullptr;
    uint32_t * mem   = nullptr;         //pointer on the memory allocated area alligned 8
    dspMalloc(uint32_t s) { init(s); }
    //allocate an array
    void init(uint32_t s) { 
        if (0 == alloc) {
            sizeused = 0;
            alloc = malloc(s+4);        //assuming word allgned by default. takes one more
            if (0 != alloc) {
                sizeallocated = s;
                mem = (uint32_t*)alloc;
                if ((uint32_t)mem & 4) { sizeused+=4; mem++; } //allign 8bytes
            }
        }
    }
    // free everythings, should not happen in a realtime enviroenemnt
    void exit() { 
        if (alloc) { 
            free(alloc); }
    }
    //provide a buffer of s bytes alligned 8 
    uint32_t * get(uint32_t s) {
        uint32_t * res = nullptr;
        uint32_t sround = s+7; sround &= ~7; //extend to multiple of 8
        if ((0 != mem) && ((sizeused + sround) < sizeallocated)) {
            res = mem;
            mem += sround / 4;
            sizeused += sround;
        }
        return res;
    }
} DSP_MALLOC(DSP_MEM_SIZE); //instantiate a predefined memory array for this program


#endif //_DSP_MEM_H_
