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


#if defined(UX_DEVICE_STANDALONE)
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_hid_tasks_run                      PORTABLE C      */
/*                                                           6.3.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is the background task of the hid.                    */
/*                                                                        */
/*    It's for standalone mode.                                           */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hid_class                                 Address of hid class      */
/*                                                container               */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    State machine status                                                */
/*    UX_STATE_EXIT                         Device not configured         */
/*    UX_STATE_IDLE                         No interrupt transfer running */
/*    UX_STATE_WAIT                         Interrupt IN transfer running */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_class_hid_event_get        Get HID event                 */
/*    _ux_device_stack_transfer_run         Run transfer state machine    */
/*    _ux_utility_memory_copy               Copy memory                   */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Device Stack                                                   */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  01-31-2022     Chaoqiong Xiao           Initial Version 6.1.10        */
/*  10-31-2023     Chaoqiong Xiao           Modified comment(s),          */
/*                                            added zero copy support,    */
/*                                            resulting in version 6.3.0  */
/*                                                                        */
/**************************************************************************/
UINT _ux_device_class_hid_tasks_run(VOID *instance)
{

UX_SLAVE_CLASS_HID          *hid;
UX_SLAVE_DEVICE             *device;
UX_DEVICE_CLASS_HID_EVENT   *hid_event;
UX_SLAVE_TRANSFER           *trans;
ULONG                       tick, elapsed;
UINT                        status;


    /* Get HID instance.  */
    hid = (UX_SLAVE_CLASS_HID *) instance;

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* Check if the device is configured.  */
    if (device -> ux_slave_device_state != UX_DEVICE_CONFIGURED)
    {
        hid -> ux_device_class_hid_event_state = UX_STATE_EXIT;
        return(UX_STATE_EXIT);
    }

#if defined(UX_DEVICE_CLASS_HID_INTERRUPT_OUT_SUPPORT)
    if (hid -> ux_device_class_hid_receiver)
        hid -> ux_device_class_hid_receiver -> ux_device_class_hid_receiver_tasks_run(hid);
#endif

    /* Get access to current event instance.  */
    hid_event = &hid -> ux_device_class_hid_event;

    /* Run HID state machine.  */
    switch(hid -> ux_device_class_hid_event_state)
    {
    case UX_STATE_EXIT:

        /* There is nothing to do in this state.  */
        return (UX_STATE_EXIT);

    case UX_STATE_RESET:

        /* Start timeout waiting.  */
        hid -> ux_device_class_hid_event_wait_start = _ux_utility_time_get();
        hid -> ux_device_class_hid_event_state = UX_STATE_IDLE;

        /* Fall through.  */
    case UX_STATE_IDLE:

        /* Check if there is event ready.  */
        status = _ux_device_class_hid_event_check(hid, &hid_event);

        /* If there is no event, check idle rate.  */
        if (status != UX_SUCCESS)
        {

            /* Check idle rate setting.  */
            if (hid -> ux_device_class_hid_event_wait_timeout == UX_WAIT_FOREVER)
            {

                /* There is no background idle report, keep waiting.  */
                return(UX_STATE_IDLE);
            }

            /* Check wait timeout.  */
            tick = _ux_utility_time_get();
            elapsed = _ux_utility_time_elapsed(hid -> ux_device_class_hid_event_wait_start, tick);
            if (elapsed < hid -> ux_device_class_hid_event_wait_timeout)
            {

                /* Keep waiting.  */
                return(UX_STATE_IDLE);
            }

            /* Send the last event in buffer.  */
        }

        /* Prepare the request to send event.  */
        trans = &hid -> ux_device_class_hid_interrupt_endpoint ->
                                            ux_slave_endpoint_transfer_request;

#if (UX_DEVICE_ENDPOINT_BUFFER_OWNER == 1) && defined(UX_DEVICE_CLASS_HID_ZERO_COPY)

        /* Directly use event buffer for transfer.  */
        trans -> ux_slave_transfer_request_data_pointer =
                                hid_event -> ux_device_class_hid_event_buffer;
#else

        /* Copy event data to endpoint buffer.  */
        _ux_utility_memory_copy(trans -> ux_slave_transfer_request_data_pointer,
                                UX_DEVICE_CLASS_HID_EVENT_BUFFER(hid_event),
                                hid_event -> ux_device_class_hid_event_length); /* Use case of memcpy is verified. */
#endif

        trans -> ux_slave_transfer_request_requested_length =
                                hid_event -> ux_device_class_hid_event_length;
        UX_SLAVE_TRANSFER_STATE_RESET(trans);
        hid -> ux_device_class_hid_event_state = UX_STATE_WAIT;

        /* Fall through.  */
    case UX_STATE_WAIT:

        /* Run transfer state machine.  */
        trans = &hid -> ux_device_class_hid_interrupt_endpoint ->
                                            ux_slave_endpoint_transfer_request;
        status = _ux_device_stack_transfer_run(trans,
                                hid_event -> ux_device_class_hid_event_length,
                                hid_event -> ux_device_class_hid_event_length);

        /* Any error or success case.  */
        if (status <= UX_STATE_NEXT)
        {

            /* Event handled and the tail should be freed.  */
            _ux_device_class_hid_event_free(hid);

            /* Next round.  */
            hid -> ux_device_class_hid_event_state = UX_STATE_RESET;
            return(UX_STATE_IDLE);
        }

        /* Wait.  */
        return(UX_STATE_WAIT);

    default:

        /* Just go back to normal state.  */
        hid -> ux_device_class_hid_event_state = UX_STATE_RESET;
        return(UX_STATE_IDLE);
    }
}
#endif
