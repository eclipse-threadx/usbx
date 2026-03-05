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
/*    _ux_hcd_ohci_frame_number_set                       PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*     This function will set the current frame number to the one         */
/*     specified. This function is mostly used for isochronous purposes.  */
/*     Here we need to write to the host controller which in turn will    */
/*     update the HCCA at the end of the frame.                           */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hcd_ohci                              Pointer to OHCI controller    */
/*    frame_number                          Frame number to set           */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
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
VOID  _ux_hcd_ohci_frame_number_set(UX_HCD_OHCI *hcd_ohci, ULONG frame_number)
{

    /* Write to OHCI register. */
    _ux_hcd_ohci_register_write(hcd_ohci, OHCI_HC_FM_NUMBER, frame_number & 0xffff);
    return;
}

