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
/*    _ux_hcd_ehci_door_bell_wait                         PORTABLE C      */
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will raise the doorbell and wait for its              */
/*    acknowledgement. This mechanism is used to safely remove a physical */
/*    endpoint from EHCI.                                                 */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hcd_ehci                              Pointer to EHCI controller    */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_hcd_ehci_register_read            Read EHCI register            */
/*    _ux_hcd_ehci_register_write           Write EHCI register           */
/*    _ux_host_semaphore_get                Get semaphore                 */
/*    _ux_host_semaphore_put                Release semaphore             */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    EHCI Controller Driver                                              */
/*                                                                        */
/**************************************************************************/
VOID  _ux_hcd_ehci_door_bell_wait(UX_HCD_EHCI *hcd_ehci)
{

ULONG       ehci_register;
UINT        status;


    /* Protect against multiple thread entry to this HCD.  */
    status =  _ux_host_semaphore_get(&hcd_ehci -> ux_hcd_ehci_protect_semaphore, UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
        return;

    /* Raise the doorbell to the HCD.  */
    ehci_register =  _ux_hcd_ehci_register_read(hcd_ehci, EHCI_HCOR_USB_COMMAND);
    ehci_register |=  EHCI_HC_IO_IAAD;
    _ux_hcd_ehci_register_write(hcd_ehci, EHCI_HCOR_USB_COMMAND, ehci_register);

    /* Wait for the doorbell to be awaken.  */
    _ux_host_semaphore_get_norc(&hcd_ehci -> ux_hcd_ehci_doorbell_semaphore, UX_WAIT_FOREVER);

    /* Free the protection semaphore.  */
    _ux_host_semaphore_put(&hcd_ehci -> ux_hcd_ehci_protect_semaphore);

    /* Return to caller.  */
    return;
}

