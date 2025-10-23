//collection of routines for lib_xua
//to minimize code impact inside original lib_xua

#include <xs1.h>
#include "xua_fabriceo.h"

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

//routines to help transfering commands from USB tile to Audiohub

#include "interrupt.h"

static volatile unsigned dec_cmd_pending = 0;
static volatile unsigned dec_param1;
static volatile unsigned dec_param2;
static volatile unsigned dec_param3;

void decouple_send_command(unsigned cmd, unsigned params, unsigned param1, unsigned param2, unsigned param3) {
    do {
        while (dec_cmd_pending) { }    // wait free slot
        dec_cmd_pending = cmd;         //aquire
        asm volatile("nop;nop;nop;nop;nop;nop;nop;");   //wait a full xmos scheduler cycle
    } while (dec_cmd_pending!=cmd);    //retry if another task has been prioritized
    dec_param1 = param1; dec_param2 = param2; dec_param3 = param3;
}

//send a potential pending command to the audio hub
void decouple_treat_command(unsigned ch) {
    unsigned cmd = dec_cmd_pending;
    if (cmd) {
        DISABLE_INTERRUPTS();
        inuint(ch);     //get word from audiohub
        outct(ch, cmd & 0x7F);
        switch (cmd ) { // managing parameters here
        case 1: { //dummy example
            outuint(ch, dec_param1); outuint(ch, dec_param2); outuint(ch, dec_param3);
            break; }
        }
        chkct(ch, 1);
        ENABLE_INTERRUPTS();
        dec_cmd_pending = 0;
    }
}


//basic routines placeholder to handle DSP tasks in lib_xua
//with synchronization by AudiohubMainLoop

//called just before launching Audiomainloop and all the dsptaks
void xua_dsp_reset(unsigned n) __attribute__ ((weak));
void xua_dsp_reset(unsigned n) { }
//called just before launching Audiomainloop and all the dsptaks
void xua_dsp_init(unsigned n)  __attribute__ ((weak));
void xua_dsp_init(unsigned n) { }
//placeholder for dsptasks. should be replaced by user program
#define xua_dsp_task(x) asm volatile("nop #xua_dsp_task %0"::"r"(n))
void xua_dsp_task_1(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_1(const unsigned n) { xua_dsp_task(1); }
void xua_dsp_task_2(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_2(const unsigned n) { xua_dsp_task(2); }
void xua_dsp_task_3(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_3(const unsigned n) { xua_dsp_task(3); }
void xua_dsp_task_4(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_4(const unsigned n) { xua_dsp_task(4); }
void xua_dsp_task_5(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_5(const unsigned n) { xua_dsp_task(5); }
void xua_dsp_task_6(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_6(const unsigned n) { xua_dsp_task(6); }
void xua_dsp_task_7(const unsigned n)  __attribute__ ((weak));
void xua_dsp_task_7(const unsigned n) { xua_dsp_task(7); }
