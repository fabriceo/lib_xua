/*
 * xua_timeout.h
 *
 *  Created on: 25 oct. 2025
 *      Author: fabriceo
 *
 *      inline routines to implement basic timeout control on blocking ressources
 *
 *
 */

#ifndef XUA_TIMEOUT_H_
#define XUA_TIMEOUT_H_

#include "fo_helpers.h"

static inline void timeout_setEvent(XCTIMER tmr, int delay) {
    asm volatile (
        "\n\t   gettime r11"                   // get time
        "\n\t   add r11,r11,%1"                // add given delay
        "\n\t   setd res[%0], r11"             // set timer target value
        "\n\t   setc res[%0], 9"               // set timer event condition after
        : : "r"(tmr),"r"(delay):"r11" );       //return result
}

static inline unsigned timeout_running(XCTIMER tmr) {
    unsigned result;
    asm volatile (
        "\n\t   ldap r11, .Levent%="           // get address of temporary label below
        "\n\t   setv res[%1], r11 "            // set resource vector address
        "\n\t   ldaw r11,sp[0]"                // get stack pointer value
        "\n\t   setev res[%1], r11"            // set environement vector to SP
        "\n\t   ldc %0, 1"                     // result will be 1
        "\n\t   eeu  res[%1]"                  // enable timer resource event
        "\n\t   setsr 1"                       // enable any events in our thread
        "\n\t   bu .Lexit%="                   // end ( go back to "if" statement )

        "\n .Levent%=:"                        // event entry point
        "\n\t   get r11, ed"                   // get exception data register which is the SP value set with setev
        "\n\t   set sp, r11"                   // restore stack pointer in case it was changed
        "\n\t   ldc %0, 0"                     // result forced to 0 to jump in the optional "else" section

        "\n .Lexit%=:"                       // exit point
        : "=r"(result) : "r"(tmr) : "r11" );   //return result
    return result;
}

static inline void timeout_clearAllEvents() {
    asm volatile("clre");
}


#if 0
//demo
static inline int timeout_demoExmple(in buffered port:32 pp) {
    timer tmr;
    timeout_setEvent(tmr,500000000);
    int val = 0;
    if ( timeout_running(tmr) ) {
        //code for accessing the blocking resource
        pp :> val;
        //clear events imediatelly after getting port value
        timeout_clearAllEvents();
        //some basic treatment can continue here but preferably outside of the "if"
    } else {
        // event is raised
    }

    return 0;
}
#endif


#endif /* XUA_TIMEOUT_H_ */
