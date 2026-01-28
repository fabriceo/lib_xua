/*
 * xua_msg_basic.c
 *
 *  Created on: 25 oct. 2025
 *      Author: fabriceo
 */

#include "fo_msg_basic.h"
#include "swlock.h"
// swlock_acquire(&msglock);
// swlock_release(&msglock);

//basic queue and messaging system

static swlock_t msglock = SWLOCK_INITIAL_VALUE;

#define qsize 32
static messages_t qmsg[qsize];

static volatile messages_t * qin  = &qmsg[0];
static volatile messages_t * qout = &qmsg[0];
static volatile unsigned qnum     = 0;

messages_t * messages_get() {
    messages_t * res = 0;
    swlock_acquire(&msglock);
    messages_t * out = (messages_t *)qout;
    if (qin != out) {
        out++;
        if (out >= &qmsg[qsize]) out = qmsg;
        if (out->msg) {
            res  = out;
            qout = out;
            qnum --;
        }
    }
    swlock_release(&msglock);
    return res;
}

unsigned messages_peek() {
    unsigned res = 0;
    swlock_acquire(&msglock);
    messages_t * out = (messages_t *)qout;
    if (qin != out) {
        out++;
        if (out > &qmsg[qsize] ) out = qmsg;
        if (out->msg) res = out->msg;
    }
    swlock_release(&msglock);
    return res;
}


messages_t * messages_create() {
    swlock_acquire(&msglock);
    messages_t * res = (messages_t *)qin;
    messages_t * next = res + 1;
    if (next >= &qmsg[qsize]) next = qmsg;
    if (next != qout) {
        qin = next;
        res = next;
        qnum++;
    }
    res->msg = res->param1 = res->param2 = res->param3 = 0;
    swlock_release(&msglock);
    return res;
}

messages_t * messages_clear() {
    swlock_acquire(&msglock);
    qin = qout = qmsg;
    qmsg->msg = 0;
    qnum = 0;
    swlock_release(&msglock);
    return 0;
}

unsigned messages_number() {
    return qnum;
}



