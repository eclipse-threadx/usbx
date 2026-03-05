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
/*    _ux_host_stack_hcd_transfer_request                 PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will route the transfer_request to the appropriate    */
/*    HCD for transfer.                                                   */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    transfer_request                      Pointer to transfer request   */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    (ux_hcd_entry_function)               HCD's entry function          */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Components                                                     */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_stack_hcd_transfer_request(UX_TRANSFER *transfer_request)
{

UINT            status;
UX_HCD          *hcd;

    /* Obtain the HCD.  */
    hcd = UX_DEVICE_HCD_GET(transfer_request -> ux_transfer_request_endpoint -> ux_endpoint_device);

    /* Call the HCD entry function to process the transfer request.  */
    status =  hcd -> ux_hcd_entry_function(hcd, UX_HCD_TRANSFER_REQUEST, (VOID *) transfer_request);

    /* Return completion status.  */
    return(status);
}

