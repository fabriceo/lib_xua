/*
 * xc_timeout.h
 *
 *  Created on: 22 oct. 2025
 *      Author: fabriceo
 */


#ifndef XUA_FABRICEO_H_
#define XUA_FABRICEO_H_

//some routines used inside LIB_XUA,
//goal being to centralize changes and extra feature here.
//all changes in libXUA are flagged with XUAFO macro

#define XUAFO_JOIN0(x,y) x ## y
#define XUAFO_JOIN(x,y) XUAFO_JOIN0(x,y)


//usefull defines
#ifdef __XC__
#define XCUNSAFE unsafe
#define XCTIMER  timer
#define XCPORT   port
#define XCCHAN   chanend
#else
#define XCUNSAFE
#define XCTIMER  unsigned
#define XCPORT   unsigned
#define XCCHAN   unsigned
#endif

#ifndef __XC__
/* Support for xCORE  channels in C */
#define null 0
#define outuint(c, x)   asm ("out res[%0], %1" :: "r" (c), "r" (x))
#define outct(c, x)     asm ("outct res[%0], %1" :: "r" (c), "r" (x))
#define chkct(c, x)     asm ("chkct res[%0], %1" :: "r" (c), "r" (x))
static inline unsigned inuint(unsigned c) { int res; asm ("in %0, res[%1]" :"=r"(res): "r" (c)); return res; }
#endif


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


// list of messages exchanged between lib_xua USB tile and the user Application (eventually in c++)
typedef enum  {
    MSG_NOT_READY = 0,
    DEC_RATE_CHANGE,        //when decouple send a XUA_AUDCTL_SET_SAMPLE_FREQ command to audio hub
    EP0_VOLUME_OUT,
    EP0_VOLUME_IN,
    EP0_VENDOR_REQ,         // when the usb host send a vendor request
    EP0_DFU_MODE,           // when the device reboot with single DFU interface
    BUF_SOF_MCLK_STOPED,    //indicate that mclk on USB tile input port is gone
    BUF_SOF_MCLK_STARTED,   //indicate that mclk on USB tile input port is now running well
    BUF_SOF_MCLK_CHANGED,   //indicate that mclk on USB tile input port has changed

} messages_e;


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

#if defined( __cplusplus )
extern "C" {
#endif

// return a pointer on the next available message from the queue, otherwise null.
// move index forward to free the previous msg.
messages_t * XCUNSAFE messages_get();
// return the next available message from the queue without discarding it, otherwise 0.
unsigned messages_peek();
//create a message in the queue with value 0 (disabled).
//move index forward to reserve the space.
messages_t * XCUNSAFE messages_create();
//clear all messages and indexes
messages_t * XCUNSAFE messages_clear();
//return the number of messages created
unsigned messages_number() ;

//send command to audiohub via decoupe task, as part of samples transfer
void decouple_send_command(unsigned cmd, unsigned params, unsigned param1, unsigned param2, unsigned param3);
//send command and potential params to audiohub
void decouple_treat_command(XCCHAN ch);

void xua_dsp_reset(unsigned n);
void xua_dsp_init(unsigned n);
void xua_dsp_task_1(const unsigned n);
void xua_dsp_task_2(const unsigned n);
void xua_dsp_task_3(const unsigned n);
void xua_dsp_task_4(const unsigned n);
void xua_dsp_task_5(const unsigned n);
void xua_dsp_task_6(const unsigned n);
void xua_dsp_task_7(const unsigned n);


#if defined( __cplusplus )
}
#endif



#endif /* XUA_FABRICEO_H_ */
