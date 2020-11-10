/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/


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
/*    _ux_hcd_ehci_transfer_abort                         PORTABLE C      */
/*                                                           6.1.2        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*     This function will abort transactions attached to a transfer       */
/*     request.                                                           */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hcd_ehci                              Pointer to EHCI controller    */
/*    transfer_request                      Pointer to transfer request   */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_utility_virtual_address           Get virtual address           */
/*    _ux_utility_mutex_on                  Get mutex                     */
/*    _ux_utility_mutex_off                 Put mutex                     */
/*    _ux_utility_delay_ms                  Delay miliseconds             */
/*    _ux_hcd_ehci_ed_clean                 Clean TDs on ED               */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    EHCI Controller Driver                                              */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  11-09-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            fixed compile warnings,     */
/*                                            resulting in version 6.1.2  */
/*                                                                        */
/**************************************************************************/
UINT  _ux_hcd_ehci_transfer_abort(UX_HCD_EHCI *hcd_ehci,UX_TRANSFER *transfer_request)
{

UX_ENDPOINT                     *endpoint;
UX_EHCI_HSISO_ED                *ied;
UX_EHCI_PERIODIC_LINK_POINTER   lp;
UX_TRANSFER                     **list_head;
UX_TRANSFER                     *transfer;
ULONG                           max_load_count;
ULONG                           remain_count;

    UX_PARAMETER_NOT_USED(hcd_ehci);


    /* Get the pointer to the endpoint associated with the transfer request*/
    endpoint =  (UX_ENDPOINT *) transfer_request -> ux_transfer_request_endpoint;

    /* From the endpoint container, get the address of the physical endpoint.  */
    lp.void_ptr =  endpoint -> ux_endpoint_ed;

    /* Check if this physical endpoint has been initialized properly!  */
    if (lp.void_ptr == UX_NULL)
    {

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_ENDPOINT_HANDLE_UNKNOWN, endpoint, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_ENDPOINT_HANDLE_UNKNOWN);
    }

    /* Check endpoint type.  */
    if ((endpoint -> ux_endpoint_descriptor.bmAttributes & UX_MASK_ENDPOINT_TYPE) == UX_ISOCHRONOUS_ENDPOINT)
    {

        /* Lock the periodic table.  */
        _ux_utility_mutex_on(&hcd_ehci -> ux_hcd_ehci_periodic_mutex);

#if defined(UX_HCD_EHCI_SPLIT_TRANSFER_ENABLE)
        if (endpoint -> ux_endpoint_device -> ux_device_speed != UX_HIGH_SPEED_DEVICE)
        {
            list_head = &lp.sitd_ptr -> ux_ehci_fsiso_td_transfer_head;
        }
        else
#endif
        {
            /* Get ED for the iTD(s).  */
            ied = lp.itd_ptr -> ux_ehci_hsiso_td_ed;

            /* Get list head for further process.  */
            list_head = &ied -> ux_ehci_hsiso_ed_transfer_head;

            /* Max load count (in 1 ms): 8, 4, 2, 1 ... */
            max_load_count = 8u >> ied -> ux_ehci_hsiso_ed_frinterval_shift;
        }

        /* The whole list is aborted.  */
        if ((transfer_request == 0) ||
            (transfer_request == &endpoint -> ux_endpoint_transfer_request) ||
            (*list_head) == transfer_request)
        {
            *list_head = UX_NULL;
            remain_count = 0;
        }
        else
        {

            /* At least one request remains.  */
            remain_count = 1;

            /* Remove it and transfers after it.  */
            transfer = (*list_head) -> ux_transfer_request_next_transfer_request;
            while(transfer)
            {

                /* If next is transfer we expect, remove from it.  */
                if (transfer -> ux_transfer_request_next_transfer_request == transfer_request)
                {

                    /* Point to NULL to remove all things after it.  */
                    transfer -> ux_transfer_request_next_transfer_request = UX_NULL;
                    break;
                }

                /* Next transfer.  */
                transfer = transfer -> ux_transfer_request_next_transfer_request;

                /* Remain plus one.  */
                remain_count ++;
            }
        }

        /* Wait a while so background transfer completed.  */
        if (remain_count < max_load_count)
            _ux_utility_delay_ms(1);

        /* Release the periodic table.  */
        _ux_utility_mutex_off(&hcd_ehci -> ux_hcd_ehci_periodic_mutex);
    }
    else

        /* Clean the TDs attached to the ED.  */
        _ux_hcd_ehci_ed_clean(lp.ed_ptr);

    /* Return successful completion.  */
    return(UX_SUCCESS);
}
