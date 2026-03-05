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
/**   HUB Class                                                           */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_hub.h"
#include "ux_host_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_host_class_hub_port_change_reset_process        PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will process a reset condition change.                */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hub                                   Pointer to HUB class          */
/*    port                                  Port number                   */
/*    port_status                           Status of port                */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_host_class_hub_feature            Set HUB class feature         */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    HUB Class                                                           */
/*                                                                        */
/**************************************************************************/
VOID  _ux_host_class_hub_port_change_reset_process(UX_HOST_CLASS_HUB *hub, UINT port, UINT port_status)
{

    UX_PARAMETER_NOT_USED(port_status);

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_HUB_PORT_CHANGE_RESET_PROCESS, hub, port, port_status, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

    /* Here we simply clear the condition so that we don't get awaken again.  */
    _ux_host_class_hub_feature(hub, port, UX_CLEAR_FEATURE, UX_HOST_CLASS_HUB_C_PORT_RESET);

    /* Return to caller.  */
    return;
}

