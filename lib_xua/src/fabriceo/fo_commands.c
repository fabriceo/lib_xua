/*
 * fo_commands.c
 *
 *  Created on: 25 oct. 2025
 *      Author: fabriceo
 */

#include <xs1.h>
#include "fo_commands.h"
#include "fo_helpers.h"

#include "interrupt.h"

static volatile unsigned dec_cmd_pending = 0;
static volatile unsigned dec_param1;
static volatile unsigned dec_param2;
static volatile unsigned dec_param3;

void decouple_send_command(unsigned cmd, unsigned param1, unsigned param2, unsigned param3) {
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
        xcinuint(ch);     //get word from audiohub
        xcoutct(ch, cmd & 0x7F);
        switch (cmd ) { // managing parameters here
        case 1: { //dummy example
            xcoutuint(ch, dec_param1); xcoutuint(ch, dec_param2); xcoutuint(ch, dec_param3);
            break; }
        }
        xcchkct(ch, 1);
        ENABLE_INTERRUPTS();
        dec_cmd_pending = 0;
    }
}

