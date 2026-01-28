/*
 * fo_commands.h
 *
 *  Created on: 25 oct. 2025
 *      Author: fabriceo
 *
 *      routines to exchange commands between application and Audiohub
 *
 */

#ifndef FO_COMMANDS_H_
#define FO_COMMANDS_H_

#include "fo_helpers.h"

EXTERNC_ON
void decouple_send_command(unsigned cmd, unsigned param1, unsigned param2, unsigned param3) ;
void decouple_treat_command(XCCHAN ch);
EXTERNC_OFF

#endif /* FO_COMMANDS_H_ */
