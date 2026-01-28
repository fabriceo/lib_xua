/*
 * vchannel.c
 *
 *  Created on: 24 oct. 2025
 *      Author: fabriceo
 */

#include <stdlib.h>
#include <xs1.h>
#include "swlock.h"

swlock_t vchannel_lock = SWLOCK_INITIAL_VALUE;

#ifndef VCHANNEL_SIZE
#define VCHANNEL_SIZE 256
#endif

char vchannel_default_queue[VCHANNEL_SIZE];

typedef struct {
    int size, pin, pout, count;
    unsigned lock; // only for senders
    char * queue;
} vchannel_t;

typedef vchannel_t * vchannelPtr_t;

vchannel_t vchannel_default;

void vchannel_init( vchannelPtr_t vch, char * addr, int length ) {
    swlock_acquire(&vchannel_lock);
    if (vch == 0) vch = &vchannel_default;
    if ((vch == &vchannel_default) && (addr==0) && (length==0)) {
        vch->queue = &vchannel_default_queue[0];
        vch->size = VCHANNEL_SIZE;
    } else {
        if (length == 0) length = VCHANNEL_SIZE;
        if (addr == 0) {
            addr = (char *)malloc(length);
        }
        vch->size = length;
        vch->queue = addr;
    }
    vch->pin = vch->pout = vch->count = 0;
    swlock_release(&vchannel_lock);
}


unsigned vchannel_outct(vchannelPtr_t vch, unsigned ct, unsigned wait) {
    if (vch == 0) vch = &vchannel_default;
    if (vch->queue == 0) vchannel_init(vch,0,0);
    unsigned res = 0;
    do  {
        if ((((unsigned)get_local_tile_id()+1)==vch->lock) || swlock_try_acquire(&vch->lock)) {
            swlock_acquire(&vchannel_lock);
            int out  = vch->pout;
            int next = vch->pin;
            int left = out - next - 1;
            if (left < 0) left += vch->size; //count for rollover
            if (left != 0 ) {
                vch->queue[next++] = ct;
                if (next >= vch->size ) next = 0;
                vch->pin = next;
                res = 1;
                swlock_release(&vchannel_lock);
                if (ct==1) swlock_release(&vch->lock);
                break;
            }
            swlock_release(&vchannel_lock);
        }
        if (wait) { delay_ticks(100); wait--;  }
    } while (wait);
    return res;
}

unsigned vchannel_outuint(vchannelPtr_t vch, unsigned val, unsigned wait) {
    if (vch == 0) vch = &vchannel_default;
    if (vch->queue == 0) vchannel_init(vch,0,0);
    unsigned res = 0;
    while (1) {
        if (((get_local_tile_id()+1)==vch->lock) || swlock_try_acquire(&vch->lock)) {
            swlock_acquire(&vchannel_lock);
            int out  = vch->pout;
            int next = vch->pin;
            int left = out - next - 1;
            if (left < 0) left += vch->size; //count for rollover
            if (left >= 5 ) {
                vch->queue[next++] = 2; //value for an unint
                if (next == vch->size ) next = 0;
                vch->queue[next++] = val; val >>= 8;
                if (next == vch->size ) next = 0;
                vch->queue[next++] = val; val >>= 8;
                if (next == vch->size ) next = 0;
                vch->queue[next++] = val; val >>= 8;
                if (next == vch->size ) next = 0;
                vch->queue[next++] = val;
                if (next == vch->size ) next = 0;
                vch->pin = next;
                swlock_release(&vchannel_lock);
                res = 1;
                break;
            }
            swlock_release(&vchannel_lock);
        }
        if (wait) { wait--; delay_ticks(100); }
    } while (wait);
    return res;
}

unsigned vchannel_inct(vchannelPtr_t vch, unsigned wait) {
    do {
        if (vch == 0) vch = &vchannel_default;
        if (vch->queue == 0) vchannel_init(vch,0,0);
        swlock_acquire(&vchannel_lock);
        int out  = vch->pout;
        int next = vch->pin;
        if (next != out) {
            unsigned ct = vch->queue[out];
            if (ct==2) {
                swlock_release(&vchannel_lock);
                return ct;
            }
            next++;
            if (next == vch->size ) next = 0;
            vch->pout = next;
            swlock_release(&vchannel_lock);
            return ct;
        }
        swlock_release(&vchannel_lock);
        if (wait) { wait--; delay_ticks(100); }
    } while (wait);
    return 0;
}

unsigned vchannel_inuint(vchannelPtr_t vch, unsigned * val, unsigned wait) {
    do {
        if (vch == 0) vch = &vchannel_default;
        if (vch->queue == 0) vchannel_init(vch,0,0);
        swlock_acquire(&vchannel_lock);
        int next = vch->pout;
        int last = vch->pin;
        if (next != last) {
            unsigned ct = vch->queue[next];
            if (ct==2) {
                swlock_release(&vchannel_lock);
                return ct;
            }
            next++;
            if (next == vch->size ) next = 0;
            int x = vch->queue[next++];
            if (next == vch->size ) next = 0;
            x |= vch->queue[next++] << 8;
            if (next == vch->size ) next = 0;
            x |= vch->queue[next++] << 16;
            if (next == vch->size ) next = 0;
            x |= vch->queue[next++] << 24;
            *val = x;
            if (next == vch->size ) next = 0;
            vch->pout = next;
            swlock_release(&vchannel_lock);
            return ct;
        }
        swlock_release(&vchannel_lock);
        if (wait) { wait--; delay_ticks(100); }
    } while (wait);
    return 0;
}
