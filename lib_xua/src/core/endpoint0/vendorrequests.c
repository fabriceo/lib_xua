// Copyright 2016-2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include "xua.h"
#if XUA_USB_EN

#include "xud.h"
#include "vendorrequests.h"

int VendorAudioRequests(XUD_ep ep0_out, XUD_ep ep0_in, unsigned char bRequest, unsigned char cs, unsigned char cn,
    unsigned short unitId, unsigned char direction, NULLABLE_RESOURCE(chanend, c_aud_ctl),
    NULLABLE_RESOURCE(chanend, c_mix_ctl),
    NULLABLE_RESOURCE(chanend, c_clk_ctL)) __attribute__ ((weak));

int VendorAudioRequests(XUD_ep ep0_out, XUD_ep ep0_in, unsigned char bRequest, unsigned char cs, unsigned char cn,
    unsigned short unitId, unsigned char direction, NULLABLE_RESOURCE(chanend, c_aud_ctl),
    NULLABLE_RESOURCE(chanend, c_mix_ctl),
    NULLABLE_RESOURCE(chanend, c_clk_ctL))
{
    return XUD_RES_ERR;
}

int VendorRequests(XUD_ep ep0_out, XUD_ep ep0_in,  REFERENCE_PARAM(USB_SetupPacket_t, sp) VENDOR_REQUESTS_PARAMS_DEC_) __attribute__ ((weak));


#define VENDOR_TEST1 (0xB1)
#define VENDOR_TEST2 (0xB2)

unsigned char vendorBuf[64];
unsigned vendorLength = 0;

int VendorRequests(XUD_ep ep0_out, XUD_ep ep0_in,  REFERENCE_PARAM(USB_SetupPacket_t, sp) VENDOR_REQUESTS_PARAMS_DEC_)
{
    XUD_Result_t result = XUD_RES_ERR;
    if (sp->bRequest >= 0xB0)
        printf("USB VENDOR REQUEST 0x%x\n", sp->bRequest);

    if(sp->bmRequestType.Direction == USB_BM_REQTYPE_DIRECTION_H2D) {
        int xudres;
        // Host to device
        // get buffer associated with request
        if (sp->wLength) {
            if ((xudres = XUD_GetBuffer(ep0_out, vendorBuf, &vendorLength)) != XUD_RES_OKAY) {
                printf("VendorRequest XUD_GetBuffer error %d\n",xudres);
                return XUD_RES_ERR;
            }
            printf("XUD_GetBuffer %d %d done\n",sp->wLength,vendorLength);
        }
    } else vendorLength = 0;

    switch( sp->bRequest ) {

        case VENDOR_TEST1: {
            printf("H2D TEST1\n");
            result = XUD_RES_OKAY;
        } break;

        case VENDOR_TEST2: {
            printf("D2H TEST2\n");
            vendorBuf[0] = 0x78; vendorBuf[1] = 0x56; vendorBuf[2] = 0x34; vendorBuf[3] = 0x12;
            vendorLength = 4;
            result = XUD_RES_OKAY;
        } break;
    }

    if (result == XUD_RES_OKAY) {
        if ((sp->bmRequestType.Direction == USB_BM_REQTYPE_DIRECTION_D2H) && ( sp->wLength != 0 )) {
                // send back expected data to the host
                result = XUD_DoGetRequest(ep0_out, ep0_in, vendorBuf, vendorLength, sp->wLength);
            } else {
                // acknoledge command treatment
                result = XUD_DoSetRequestStatus(ep0_in);
            }
    }

    return result;
}

void VendorRequests_Init(VENDOR_REQUESTS_PARAMS_DEC) __attribute__ ((weak));

void VendorRequests_Init(VENDOR_REQUESTS_PARAMS_DEC)
{

}

#endif /* XUA_USB_EN */
