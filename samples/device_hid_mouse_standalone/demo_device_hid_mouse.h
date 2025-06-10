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

#ifndef UX_DEMO_DEVICE_HID_MOUSE_H
#define UX_DEMO_DEVICE_HID_MOUSE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ux_api.h"
#include "ux_device_class_hid.h"


#include "demo_device_hid_descriptors.h"

#include "usb_otg.h"


#define UX_DEVICE_MEMORY_STACK_SIZE             2*1024


#ifdef UX_DEMO_MOUSE_ABSOLUTE
#define UX_DEMO_HID_MOUSE_CURSOR_MOVE           500
#else /* UX_DEMO_MOUSE_ABSOLUTE */
#define UX_DEMO_HID_MOUSE_CURSOR_MOVE           3
#define UX_DEMO_HID_MOUSE_CURSOR_MOVE_N         100
#endif /* UX_DEMO_MOUSE_ABSOLUTE */

#define UX_DEMO_HID_MOUSE_WHEEL_MOVE            15
#define UX_DEMO_HID_MOUSE_WHEEL_MOVE_N          100


/******************************/
/**  Mouse cursor direction   */
/******************************/
#define UX_MOUSE_CURSOR_MOVE_RIGHT              0x00
#define UX_MOUSE_CURSOR_MOVE_DOWN               0x01
#define UX_MOUSE_CURSOR_MOVE_LEFT               0x02
#define UX_MOUSE_CURSOR_MOVE_UP                 0x03
#define UX_MOUSE_CURSOR_MOVE_DONE               0x10

/*****************************/
/**  Mouse Buttons           */
/*****************************/
#define UX_MOUSE_BUTTON_PRESS_LEFT              0x00
#define UX_MOUSE_BUTTON_PRESS_RIGHT             0x01
#define UX_MOUSE_BUTTON_PRESS_MIDDLE            0x02
#define UX_MOUSE_BUTTON_PRESS_DONE              0x10

/*****************************/
/**  Mouse wheel direction   */
/*****************************/
#define UX_MOUSE_WHEEL_MOVE_DOWN                0x00
#define UX_MOUSE_WHEEL_MOVE_UP                  0x01
#define UX_MOUSE_WHEEL_MOVE_DONE                0x10

/*******************************/
/**  Mouse demo state machine  */
/*******************************/
#define UX_DEMO_MOUSE_CURSOR                    0x00
#define UX_DEMO_MOUSE_WHEEL                     0x01
#define UX_DEMO_MOUSE_BUTTON                    0x02
#define UX_DEMO_MOUSE_DONE                      0x10


VOID ux_application_define(VOID);
VOID ux_demo_device_hid_task(VOID);

#ifdef __cplusplus
}
#endif
#endif /* UX_DEMO_DEVICE_HID_MOUSE_H */
