/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation 
 * 
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 * 
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** USBX Component                                                        */
/**                                                                       */
/**   Device HID Class                                                    */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_hid.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_hid_receiver_event_get             PORTABLE C      */
/*                                                           6.3.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks if there was an event from the interrupt OUT,  */
/*    if so return first received event length and it's buffer entry      */
/*    pointer.                                                            */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hid                                      Address of hid class       */
/*    event                                    Pointer of the event       */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    status                                   UX_SUCCESS with event      */
/*                                             UX_ERROR without event     */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_utility_memory_copy                  Copy memory                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    ThreadX                                                             */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  01-31-2022     Chaoqiong Xiao           Initial Version 6.1.10        */
/*  07-29-2022     Chaoqiong Xiao           Modified comment(s),          */
/*                                            cleaned compile with TRACE, */
/*                                            resulting in version 6.1.12 */
/*  10-31-2023     Chaoqiong Xiao           Modified comment(s),          */
/*                                            added zero copy support,    */
/*                                            resulting in version 6.3.0  */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_hid_receiver_event_get(UX_SLAVE_CLASS_HID *hid,
                                UX_DEVICE_CLASS_HID_RECEIVED_EVENT *event)
{
#if !defined(UX_DEVICE_CLASS_HID_INTERRUPT_OUT_SUPPORT)
    UX_PARAMETER_NOT_USED(hid);
    UX_PARAMETER_NOT_USED(event);
    return(UX_FUNCTION_NOT_SUPPORTED);
#else


UX_DEVICE_CLASS_HID_RECEIVER            *receiver;
UX_DEVICE_CLASS_HID_RECEIVED_EVENT      *pos;


    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_CLASS_HID_RECEIVER_EVENT_GET, hid, event, 0, 0, UX_TRACE_DEVICE_CLASS_EVENTS, 0, 0)

    /* Get receiver.  */
    receiver = hid -> ux_device_class_hid_receiver;

    /* Get current reading position.  */
    pos = receiver -> ux_device_class_hid_receiver_event_read_pos;

    /* Check if it's available.  */
    if (pos -> ux_device_class_hid_received_event_length != 0)
    {

        /* Fill event structure to return.  */
        event -> ux_device_class_hid_received_event_length = pos -> ux_device_class_hid_received_event_length;

        /* Fill data buffer address to return.  */
        event -> ux_device_class_hid_received_event_data = UX_DEVICE_CLASS_HID_RECEIVED_QUEUE_ITEM_BUFFER(pos);

        return(UX_SUCCESS);
    }

    return(UX_ERROR);
#endif
}


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _uxe_device_class_hid_receiver_event_get            PORTABLE C      */
/*                                                           6.3.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks errors in HID event get function call.         */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hid                                   Pointer to hid instance       */
/*    event                                 Pointer of the event          */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_class_hid_receiver_event_get                             */
/*                                          Get a receiver event          */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  10-31-2023     Chaoqiong Xiao           Initial Version 6.3.0         */
/*                                                                        */
/**************************************************************************/
UINT  _uxe_device_class_hid_receiver_event_get(UX_SLAVE_CLASS_HID *hid,
                                UX_DEVICE_CLASS_HID_RECEIVED_EVENT *event)
{

    /* Sanity check.  */
    if ((hid == UX_NULL) || (event == UX_NULL))
        return(UX_INVALID_PARAMETER);

    /* Invoke function to free HID event.  */
    return(_ux_device_class_hid_receiver_event_get(hid, event));
}
