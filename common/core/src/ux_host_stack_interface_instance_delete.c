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
/*    _ux_host_stack_interface_instance_delete            PORTABLE C      */
/*                                                           6.1.12       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will delete an interface instance. It does not delete */
/*    the interface container, but it removes all the endpoints           */
/*    associated with alternate setting.                                  */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    interface                             Pointer to interface          */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_host_stack_endpoint_instance_delete  Delete endpoint            */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Components                                                     */
/*                                                                        */
/**************************************************************************/
VOID  _ux_host_stack_interface_instance_delete(UX_INTERFACE *interface_ptr)
{

UX_ENDPOINT     *endpoint;

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_STACK_INTERFACE_INSTANCE_DELETE, interface_ptr, 0, 0, 0, UX_TRACE_HOST_STACK_EVENTS, 0, 0)

    /* If trace is enabled, register this object.  */
    UX_TRACE_OBJECT_UNREGISTER(interface_ptr);

    /* Obtain the first endpoint for this alternate setting.  */
    endpoint =  interface_ptr -> ux_interface_first_endpoint;

    /* Loop to delete each endpoint.  */
    while (endpoint != UX_NULL)
    {

        /* Delete endpoint.  */
        _ux_host_stack_endpoint_instance_delete(endpoint);

        /* Move to next endpoint. */
        endpoint =  endpoint -> ux_endpoint_next_endpoint;
    }

    /* Return to caller.  */
    return;
}

