/*
 * xua_msg_basic.h
 *
 *  Created on: 25 oct. 2025
 *      Author: fabriceo
 */

#ifndef XUA_MSG_BASIC_H_
#define XUA_MSG_BASIC_H_

#include "fo_helpers.h"


//very basic message structure, fixed size.KISS
//this will most probably change in future for a more flexible, robust and powerfull solution
//writing a value in msg will "enable" the message.
typedef struct messages_s {
    int msg;
    unsigned param1;
    unsigned param2;
    unsigned param3;
} messages_t;


/*
 * usage : sending a message in the queue:
 * messages_t * m = messages_create();
 * if (m) { m->param1 = 0; m->msg = EP0_DFU_MODE; }
 *
 * receiving: testing and acting
 * while (1) {
 * messages_t * m = messages_get();
 * if ( m && (m->msg == EP0_DFU_MODE) ) printf("enterring dfu\n");
 * }
 */

EXTERNC_ON

messages_t * XCUNSAFE messages_get();
unsigned messages_peek();
messages_t * XCUNSAFE messages_create();
messages_t * XCUNSAFE messages_clear();
unsigned messages_number();

EXTERNC_OFF
#endif /* XUA_MSG_BASIC_H_ */
