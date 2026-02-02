/*
 * vchannel.h
 *
 *  Created on: 24 oct. 2025
 *      Author: fabriceo
 *
 *      basic implementation of a virtual channel communication model
 *
 */

#ifndef VCHANNEL_H_
#define VCHANNEL_H_

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

#if defined( __cplusplus )
extern "C" {
#endif

extern void vchannel_init( vchannelPtr_t vch, char * addr, int length );
extern unsigned vchannel_outct(vchannelPtr_t vch, unsigned ct, unsigned wait);
extern unsigned vchannel_outuint(vchannelPtr_t vch, unsigned val, unsigned wait) ;
extern unsigned vchannel_inct(vchannelPtr_t vch, unsigned wait);
extern unsigned vchannel_inuint(vchannelPtr_t vch, unsigned * val, unsigned wait);

#if defined( __cplusplus )
}
#endif

#endif /* VCHANNEL_H_ */
