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
/*    _ux_host_stack_interface_instance_create            PORTABLE C      */
/*                                                           6.1.12       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will create an interface instance. It creates each    */
/*    endpoint associated with the interface.                             */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    interface                             Pointer to interface          */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_host_stack_endpoint_instance_create Create instance endpoint    */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Components                                                     */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_stack_interface_instance_create(UX_INTERFACE *interface_ptr)
{

UX_ENDPOINT     *endpoint;
UINT            status;

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_STACK_INTERFACE_INSTANCE_CREATE, interface_ptr, 0, 0, 0, UX_TRACE_HOST_STACK_EVENTS, 0, 0)

    /* Obtain the first endpoint for this alternate setting.  */
    endpoint =  interface_ptr -> ux_interface_first_endpoint;

    /* Loop to create each endpoint.  */
    while (endpoint != UX_NULL)
    {

        /* Create an endpoint for the instance.  */
        status = _ux_host_stack_endpoint_instance_create(endpoint);

        /* Check status, the controller may have refused the endpoint creation.  */
        if (status != UX_SUCCESS)

            /* An error occurred at the controller level.  */
            return(status);

        /* Move to next endpoint.  */
        endpoint =  endpoint -> ux_endpoint_next_endpoint;
    }

    /* If trace is enabled, register this object.  */
    UX_TRACE_OBJECT_REGISTER(UX_TRACE_HOST_OBJECT_TYPE_INTERFACE, interface_ptr, 0, 0, 0);

    /* Return completion status.  */
    return(UX_SUCCESS);
}

