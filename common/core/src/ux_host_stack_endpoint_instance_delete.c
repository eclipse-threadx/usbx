/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 * Copyright (c) 2026-present Eclipse ThreadX contributors
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
/**   Host Stack                                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_host_stack_endpoint_instance_delete             PORTABLE C      */
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will delete an endpoint instance. It does not delete  */
/*    the endpoint container but it removes the HCD endpoint and reclaims */
/*    the bandwidth.                                                      */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    endpoint                              Endpoint to delete            */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_host_stack_bandwidth_release      Release bandwidth             */
/*    _ux_utility_semaphore_delete          Semaphore delete              */
/*    (ux_hcd_entry_function)               HCD entry function            */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Components                                                     */
/*                                                                        */
/**************************************************************************/
VOID  _ux_host_stack_endpoint_instance_delete(UX_ENDPOINT *endpoint)
{

UX_HCD          *hcd;


    /* Obtain the HCD for this endpoint.  */
    hcd = UX_DEVICE_HCD_GET(endpoint -> ux_endpoint_device);

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_STACK_ENDPOINT_INSTANCE_DELETE, endpoint -> ux_endpoint_device, endpoint, 0, 0, UX_TRACE_HOST_STACK_EVENTS, 0, 0)

    /* Ensure the endpoint had its physical ED allocated.  */
    if (endpoint -> ux_endpoint_ed != UX_NULL)
    {

        /* Destroy this endpoint.  */
        hcd -> ux_hcd_entry_function(hcd, UX_HCD_DESTROY_ENDPOINT, (VOID *) endpoint);

        /* Free the semaphore previously attached to the transfer_request of this endpoint.  */
        _ux_host_semaphore_delete(&endpoint -> ux_endpoint_transfer_request.ux_transfer_request_semaphore);
    }

    /* If the endpoint requested guaranteed bandwidth, free it now.  */
    switch ((endpoint -> ux_endpoint_descriptor.bmAttributes) & UX_MASK_ENDPOINT_TYPE)
    {

    case UX_CONTROL_ENDPOINT:
    case UX_BULK_ENDPOINT:

        break;

    default:

        /* Reclaim its bandwidth.  */
        _ux_host_stack_bandwidth_release(hcd, endpoint);
    }

    /* If trace is enabled, register this object.  */
    UX_TRACE_OBJECT_UNREGISTER(endpoint);

    /* Return to caller.  */
    return;
}

