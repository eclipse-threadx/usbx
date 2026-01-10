/***************************************************************************
 * Copyright (c) 2025-present Eclipse ThreadX Contributors
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
/**   Host HID Class                                                      */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_hid.h"
#include "ux_host_stack.h"

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_host_class_hid_protocol_get                     PORTABLE C      */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function performs a GET_PROTOCOL to the HID device to read     */
/*    current protocol (BOOT=0 or REPORT=1).                              */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hid                                   Pointer to HID class          */
/*    protocol                              Destination for protocol      */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  01-10-2026     Mohamed AYED             Initial Version 6.x           */
/**************************************************************************/
UINT  _ux_host_class_hid_protocol_get(UX_HOST_CLASS_HID *hid, USHORT *protocol)
{

UX_ENDPOINT     *control_endpoint;
UX_TRANSFER     *transfer_request;
UINT             status;
UCHAR            proto_byte = 0xFFu;

    /* Ensure the instance is valid.  */
    if (_ux_host_stack_class_instance_verify(_ux_system_host_class_hid_name, (VOID *) hid) != UX_SUCCESS)
    {

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_HOST_CLASS_INSTANCE_UNKNOWN, hid, 0, 0, UX_TRACE_ERRORS, 0, 0)

#if defined(UX_HOST_STANDALONE)
        hid -> ux_host_class_hid_status = UX_HOST_CLASS_INSTANCE_UNKNOWN;
#endif

        return(UX_HOST_CLASS_INSTANCE_UNKNOWN);
    }

    /* Get the default control endpoint transfer request pointer.  */
    control_endpoint =  &hid -> ux_host_class_hid_device -> ux_device_control_endpoint;
    transfer_request =  &control_endpoint -> ux_endpoint_transfer_request;


#if !defined(UX_HOST_STANDALONE)

    /* Protect thread reentry to this instance.  */
    status =  _ux_host_semaphore_get(&hid -> ux_host_class_hid_semaphore, UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
        return(status);

    /* Protect the control endpoint semaphore here.  It will be unprotected in the
       transfer request function.  */
    status =  _ux_host_semaphore_get(&hid -> ux_host_class_hid_device -> ux_device_protection_semaphore, UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
    {
        _ux_host_semaphore_put(&hid -> ux_host_class_hid_semaphore);
        return(status);
    }

#endif

    /* Create a transfer request for the GET_PROTOCOL request.  */
    transfer_request -> ux_transfer_request_data_pointer     =  &proto_byte;
    transfer_request -> ux_transfer_request_requested_length =  1;
    transfer_request -> ux_transfer_request_function         =  UX_HOST_CLASS_HID_GET_PROTOCOL;
    transfer_request -> ux_transfer_request_type             =  UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value            =  0;
    transfer_request -> ux_transfer_request_index            =  hid -> ux_host_class_hid_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Send request to HCD layer.  */
    status =  _ux_host_stack_transfer_request(transfer_request);

#if !defined(UX_HOST_STANDALONE)
    /* Unprotect thread reentry to this instance.  */
    _ux_host_semaphore_put(&hid -> ux_host_class_hid_semaphore);
#endif

    if (status == UX_SUCCESS && protocol)
        *protocol = (USHORT)proto_byte;

    return(status);
}

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _uxe_host_class_hid_protocol_get                    PORTABLE C      */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks errors in HID protocol get function call.      */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hid                                   Pointer to HID class          */
/*    protocol                              Destination for protocol      */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Status                                                              */
/*                                                                        */
/**************************************************************************/
UINT  _uxe_host_class_hid_protocol_get(UX_HOST_CLASS_HID *hid, USHORT *protocol)
{
    /* Sanity check.  */
    if (hid == UX_NULL || protocol == UX_NULL)
        return(UX_INVALID_PARAMETER);

    /* Invoke protocol get function.  */
    return(_ux_host_class_hid_protocol_get(hid, protocol));
}
