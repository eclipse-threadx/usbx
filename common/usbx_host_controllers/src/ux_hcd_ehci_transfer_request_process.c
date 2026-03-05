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
/**   EHCI Controller Driver                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_hcd_ehci.h"
#include "ux_host_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_hcd_ehci_transfer_request_process               PORTABLE C      */
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*     This function process the transfer that was completed either       */
/*     successfully because of a partial transmission or because of an    */
/*     error. The transfer descriptor tells us what to do with it, either */
/*     put a semaphore to the caller or invoke a completion routine. If a */
/*     completion routine is specified, the routine is called and no      */
/*     semaphore is put.                                                  */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    transfer_request                      Pointer to transfer request   */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    (ux_transfer_request_completion_function) Transfer complete function*/
/*    _ux_host_semaphore_put                  Put producer semaphore      */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    EHCI Controller Driver                                              */
/*                                                                        */
/**************************************************************************/
VOID  _ux_hcd_ehci_transfer_request_process(UX_TRANSFER *transfer_request)
{

    /* Check if there is a function for the transfer completion.  */
    if (transfer_request -> ux_transfer_request_completion_function != UX_NULL)

        /* Yes, so we call it.  */
        transfer_request -> ux_transfer_request_completion_function(transfer_request);
    else

        /* There is a semaphore so send the signal to the class.  */
        _ux_host_semaphore_put(&transfer_request -> ux_transfer_request_semaphore);

    /* Return to caller.  */
    return;
}

