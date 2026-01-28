/*
 * xua_fabriceo.h
 *
 *  Created on: 22 oct. 2025
 *      Author: fabriceo
 *
 *      this file can be included in the user application
 *      to get access to extra features implemented in libxua
 *
 */


#ifndef XUA_FABRICEO_H_
#define XUA_FABRICEO_H_

#ifdef __xua_conf_h_exists__
    #include "xua_conf.h"
#endif

#include "../src/fabriceo/fo_helpers.h"
#include "../src/fabriceo/fo_timeout.h"
#include "../src/fabriceo/fo_msg_basic.h"
#include "../src/fabriceo/fo_dsp_basic.h"




//usefull defines
#ifdef __XC__
#  ifndef XCUNSAFE
#    define XCUNSAFE unsafe
#  endif
#  ifndef XCTIMER
#    define XCTIMER  timer
#  endif
#  ifndef XCPORT
#    define XCPORT   port
#  endif
#  ifndef XCCHAN
#    define XCCHAN   chanend
#  endif
#else
#  ifndef XCUNSAFE
#    define XCUNSAFE
#  endif
#  ifndef XCTIMER
#    define XCTIMER  unsigned
#  endif
#  ifndef XCPORT
#    define XCPORT   unsigned
#  endif
#  ifndef XCCHAN
#    define XCCHAN   unsigned
#  endif
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




#endif /* XUA_FABRICEO_H_ */
