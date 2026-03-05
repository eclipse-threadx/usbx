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
/**   OHCI Controller Driver                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_hcd_ohci.h"
#include "ux_host_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_hcd_ohci_port_suspend                           PORTABLE C      */
/*                                                           6.1.2        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*     This function will suspend a specific port attached to the root    */
/*     HUB.                                                               */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hcd_ohci                              Pointer to OHCI controller    */
/*    port_index                            Port index                    */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    OHCI Controller Driver                                              */
/*                                                                        */
/**************************************************************************/
UINT  _ux_hcd_ohci_port_suspend(UX_HCD_OHCI *hcd_ohci, ULONG port_index)
{

    UX_PARAMETER_NOT_USED(hcd_ohci);
    UX_PARAMETER_NOT_USED(port_index);
    return(UX_FUNCTION_NOT_SUPPORTED);
}

