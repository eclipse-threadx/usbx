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
/*    _ux_hcd_ohci_endpoint_reset                         PORTABLE C      */
/*                                                           6.1.2        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*     This function will reset an endpoint.                              */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hcd_ohci                              Pointer to OHCI HCD           */
/*    endpoint                              Pointer to endpoint           */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
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
UINT  _ux_hcd_ohci_endpoint_reset(UX_HCD_OHCI *hcd_ohci, UX_ENDPOINT *endpoint)
{

UX_OHCI_ED      *ed;
ULONG           td_value;


    UX_PARAMETER_NOT_USED(hcd_ohci);

    /* From the endpoint container fetch the OHCI ED descriptor.  */
    ed =  (UX_OHCI_ED *) endpoint -> ux_endpoint_ed;

    /* Reset the data0/data1 toggle bit in the Head TD.  */
    td_value =  (ULONG) ed -> ux_ohci_ed_head_td;
    td_value &=  UX_OHCI_ED_MASK_TD;
    ed -> ux_ohci_ed_head_td =  (UX_OHCI_TD *) td_value;

    /* This operation never fails!  */
    return(UX_SUCCESS);
}

