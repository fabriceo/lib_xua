/*
 * fo_decouple.h
 *
 *  Created on: 22 oct. 2025
 *      Author: fabrice
 */

#ifndef FO_DECOUPLE_H_
#define FO_DECOUPLE_H_

#ifdef __xua_conf_h_exists__
    #include "xua_conf.h"
#endif

/************ DECOUPLE EXTENSIONS ***************/

//test flag defined in xua_conf.h
#if defined( XUA_DECOUPLE_AUDCTL) && ( XUA_DECOUPLE_AUDCTL == 1 )

#include "fo_msg_basic.h"

static inline void decouple_AUDCTL_SET_SAMPLE_FREQ(unsigned freq)  { unsafe {
    messages_t * XCUNSAFE m = messages_create();
    if (m) { m->param1 = freq; m->msg = DEC_RATE_CHANGE; }
} }

#define XUA_DECOUPLE_AUDCTL_SET_SAMPLE_FREQ(x) decouple_AUDCTL_SET_SAMPLE_FREQ(x)

#else
#define XUA_DECOUPLE_AUDCTL_SET_SAMPLE_FREQ(x)  do { } while(0)
#endif //XUA_DECOUPLE_AUDCTL


#if defined( XUA_DECOUPLE_EXTRA_COMMANDS ) && ( XUA_DECOUPLE_EXTRA_COMMANDS == 1 )

#include "fo_commands.h"

#define XUA_DECOUPLE_CMD_TRANSFER(x) decouple_treat_command(x)
#else
#define XUA_DECOUPLE_CMD_TRANSFER(x) do { } while(0)
#endif

#endif /* FO_DECOUPLE_H_ */
