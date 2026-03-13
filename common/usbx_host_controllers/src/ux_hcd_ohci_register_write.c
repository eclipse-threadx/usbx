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
/*    _ux_hcd_ohci_register_write                         PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*     This function writes a register to the OHCI HCOR.                  */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hcd_ohci                              Pointer to OHCI controller    */
/*    ohci_register                         OHCI register to write to     */
/*    value                                 Value to write                */
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
VOID  _ux_hcd_ohci_register_write(UX_HCD_OHCI *hcd_ohci, ULONG ohci_register, ULONG value)
{

    /* Write to the register.  */
    *(hcd_ohci -> ux_hcd_ohci_hcor + ohci_register) =  value;

    /* Return to caller.  */
    return;
}

