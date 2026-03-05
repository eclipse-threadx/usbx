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
/*    _ux_hcd_ehci_poll_rate_entry_get                    PORTABLE C      */
/*                                                           6.1.2        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function return a pointer to the first ED in the periodic tree */
/*    that start specific poll rate.                                      */
/*    Note that when poll rate is longer, poll depth is smaller and       */
/*    endpoint period interval is larger.                                 */
/*      PollInterval   Depth                                              */
/*         1             M                                                */
/*         2            M-1                                               */
/*         4            M-2                                               */
/*         8            M-3                                               */
/*        ...           ...                                               */
/*         N             0                                                */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hcd_ehci                              Pointer to EHCI controller    */
/*    ed_list                               Pointer to ED list to scan    */
/*    poll_depth                            Poll depth expected           */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    UX_EHCI_ED *                          Pointer to ED                 */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    EHCI Controller Driver                                              */
/*                                                                        */
/**************************************************************************/
UX_EHCI_ED *_ux_hcd_ehci_poll_rate_entry_get(UX_HCD_EHCI *hcd_ehci,
    UX_EHCI_ED *ed_list, ULONG poll_depth)
{


    UX_PARAMETER_NOT_USED(hcd_ehci);

    /* Scan the list of ED/iTD/siTDs from the poll rate lowest/interval longest
       entry until appropriate poll rate node.
       The depth index is the poll rate EHCI value and the first entry (anchor)
       is pointed.  */

    /* Obtain next link pointer including Typ and T.  */
    while(poll_depth --)
    {
        if (ed_list -> REF_AS.ANCHOR.ux_ehci_ed_next_anchor == UX_NULL)
            break;
        ed_list = ed_list -> REF_AS.ANCHOR.ux_ehci_ed_next_anchor;
    }

    /* Return the list entry.  */
    return(ed_list);
}
