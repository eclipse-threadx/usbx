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
/*    _ux_hcd_ohci_controller_disable                     PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*     This function will disable the OHCI controller. The controller     */
/*     will release all its resources (memory, IO ...). After this, the   */
/*     controller will not send SOF any longer. All transactions should   */
/*     have been completed, all classes should have been closed.          */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hcd_ohci                              Pointer to OHCI HCD           */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_hcd_ohci_register_write           Write OHCI register           */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    OHCI Controller Driver                                              */
/*                                                                        */
/**************************************************************************/
UINT  _ux_hcd_ohci_controller_disable(UX_HCD_OHCI *hcd_ohci)
{

UX_HCD      *hcd;
ULONG       ohci_register;


    /* Point to the generic portion of the host controller structure instance.  */
    hcd =  hcd_ohci -> ux_hcd_ohci_hcd_owner;

    /* Set the controller to disabled state.  */
    ohci_register =  OHCI_HC_CR_RESET;
    _ux_hcd_ohci_register_write(hcd_ohci, OHCI_HC_CONTROL, ohci_register);

    /* Reflect the state of the controller in the main structure.  */
    hcd -> ux_hcd_status =  UX_HCD_STATUS_HALTED;

    /* Return successful completion.  */
    return(UX_SUCCESS);
}

