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
/*    _ux_host_stack_rh_device_extraction                 PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function handles a device extraction on a downstream port      */
/*    of the root hub pointed by HCD.                                     */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    HCD                                   Pointer to HCD structure      */
/*    port_index                            Port index of insertion       */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_host_stack_device_remove          Remove device                 */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Components                                                     */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_stack_rh_device_extraction(UX_HCD *hcd, UINT port_index)
{

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_STACK_RH_DEVICE_EXTRACTION, hcd, port_index, 0, 0, UX_TRACE_HOST_STACK_EVENTS, 0, 0)

    /* Ask the stack to remove the device, pass the value 0 as the parent root hub.  */
    _ux_host_stack_device_remove(hcd, 0, port_index);

    /* The device has been removed, so the port is free again.  */
    hcd -> ux_hcd_rh_device_connection &= (ULONG)~(1 << port_index);

    /* That command should never fail!  */
    return(UX_SUCCESS);
}

