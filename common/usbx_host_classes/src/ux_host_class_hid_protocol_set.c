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
/*    _ux_host_class_hid_protocol_set                     PORTABLE C      */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function performs a SET_PROTOCOL to the HID device to switch   */
/*    between BOOT (0) and REPORT (1) protocols.                          */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hid                                   Pointer to HID class          */
/*    protocol                              Protocol (BOOT/REPORT)        */
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
/**************************************************************************/
UINT  _ux_host_class_hid_protocol_set(UX_HOST_CLASS_HID *hid, USHORT protocol)
{
#if defined(UX_HOST_STANDALONE)
UX_INTERRUPT_SAVE_AREA
#endif
UX_ENDPOINT     *control_endpoint;
UX_TRANSFER     *transfer_request;
UINT             status;

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

    /* Protect thread reentry to this instance.  */
#if defined(UX_HOST_STANDALONE)
    UX_DISABLE
    if (hid -> ux_host_class_hid_flags & UX_HOST_CLASS_HID_FLAG_LOCK)
    {
        UX_RESTORE
        return(UX_BUSY);
    }
    hid -> ux_host_class_hid_flags |= UX_HOST_CLASS_HID_FLAG_LOCK;
    UX_RESTORE
#else
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

#if defined(UX_HOST_STANDALONE)
    /* Protect the control endpoint.  It will be unprotected in the transfer request function.  */
    UX_DISABLE
    if (hid -> ux_host_class_hid_device -> ux_device_flags & UX_DEVICE_FLAG_LOCK)
    {
        _ux_host_class_hid_unlock(hid);
        UX_RESTORE
        return(UX_BUSY);
    }
    hid -> ux_host_class_hid_device -> ux_device_flags |=  UX_DEVICE_FLAG_LOCK;
    transfer_request -> ux_transfer_request_flags |=  UX_TRANSFER_FLAG_AUTO_DEVICE_UNLOCK;
    UX_TRANSFER_STATE_RESET(transfer_request);
    UX_RESTORE
#endif

    /* Create a transfer request for the SET_PROTOCOL request.  */
    transfer_request -> ux_transfer_request_data_pointer     =  UX_NULL;
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function         =  UX_HOST_CLASS_HID_SET_PROTOCOL;
    transfer_request -> ux_transfer_request_type             =  UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value            =  (UINT)protocol;
    transfer_request -> ux_transfer_request_index            =  hid -> ux_host_class_hid_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Send request to HCD layer.  */
    status =  _ux_host_stack_transfer_request(transfer_request);

#if defined(UX_HOST_STANDALONE)
    if (!(transfer_request -> ux_transfer_request_flags & UX_TRANSFER_FLAG_AUTO_WAIT))
        return(status);
    _ux_host_class_hid_unlock(hid);
#else
    /* Unprotect thread reentry to this instance.  */
    _ux_host_semaphore_put(&hid -> ux_host_class_hid_semaphore);
#endif

    /* Return the function status.  */
    return(status);
}

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _uxe_host_class_hid_protocol_set                    PORTABLE C      */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks errors in HID protocol set function call.      */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hid                                   Pointer to HID class          */
/*    protocol                              Protocol (BOOT/REPORT)        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Status                                                              */
/*                                                                        */
/**************************************************************************/
UINT  _uxe_host_class_hid_protocol_set(UX_HOST_CLASS_HID *hid, USHORT protocol)
{
    /* Sanity check.  */
    if (hid == UX_NULL)
        return(UX_INVALID_PARAMETER);

    /* Validate protocol value: must be BOOT(0) or REPORT(1). */
    if (protocol != UX_HOST_CLASS_HID_PROTOCOL_BOOT &&
        protocol != UX_HOST_CLASS_HID_PROTOCOL_REPORT)
        return(UX_INVALID_PARAMETER);

    /* Invoke protocol set function.  */
    return(_ux_host_class_hid_protocol_set(hid, protocol));
}
