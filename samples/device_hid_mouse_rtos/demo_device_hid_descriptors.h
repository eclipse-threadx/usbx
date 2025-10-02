/***************************************************************************
 * Copyright (c) 2025-present Eclipse ThreadX Contributors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/**  AUTHOR                                                               */
/**                                                                       */
/**   Mohamed AYED                                                        */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#ifndef UX_DEMO_DEVICE_DESCRIPTORS_H
#define UX_DEMO_DEVICE_DESCRIPTORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ux_api.h"

//#define UX_DEMO_MOUSE_ABSOLUTE

#define DEMO_HID_BOOT_DEVICE
#define UX_DEMO_HID_DEVICE_VID                  0x070A
#define UX_DEMO_HID_DEVICE_PID                  0x4036

#define UX_DEMO_MAX_EP0_SIZE                    0x40U
#define UX_DEMO_HID_CONFIG_DESC_SIZE            0x22U

#define UX_DEMO_BCD_USB                         0x0200

#define UX_DEMO_BCD_HID                         0x0110

#define UX_DEMO_HID_ENDPOINT_SIZE               0x08
#define UX_DEMO_HID_ENDPOINT_ADDRESS            0x81
#define UX_DEMO_HID_ENDPOINT_BINTERVAL          0x08

#ifdef DEMO_HID_BOOT_DEVICE
#define UX_DEMO_HID_SUBCLASS                    0x01
#else /* DEMO_HID_BOOT_DEVICE */
#define UX_DEMO_HID_SUBCLASS                    0x00
#endif


UCHAR*  ux_demo_get_high_speed_framework(VOID);
ULONG   ux_demo_get_high_speed_framework_length(VOID);
UCHAR*  ux_demo_get_full_speed_framework(VOID);
ULONG   ux_demo_get_full_speed_framework_length(VOID);
UCHAR*  ux_demo_get_string_framework(VOID);
ULONG   ux_demo_get_string_framework_length(VOID);
UCHAR*  ux_demo_get_language_framework(VOID);
ULONG   ux_demo_get_language_framework_length(VOID);

UCHAR*  ux_demo_device_hid_get_report(VOID);
ULONG   ux_demo_device_hid_get_report_length(VOID);

#ifdef __cplusplus
}
#endif
#endif  /* UX_DEMO_DEVICE_DESCRIPTORS_H */
