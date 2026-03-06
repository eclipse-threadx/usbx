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
/**   Slave Simulator Controller Driver                                   */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_dcd_sim_slave.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_dcd_sim_slave_endpoint_reset                    PORTABLE C      */
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will reset a physical endpoint.                       */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    dcd_sim_slave                         Pointer to device controller  */
/*    endpoint                              Pointer to endpoint container */
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
/*    Slave Simulator Controller Driver                                   */
/*                                                                        */
/**************************************************************************/
UINT  _ux_dcd_sim_slave_endpoint_reset(UX_DCD_SIM_SLAVE *dcd_sim_slave, UX_SLAVE_ENDPOINT *endpoint)
{

UX_DCD_SIM_SLAVE_ED     *ed;
ULONG                   transfer_waiting;
#if !defined(UX_DEVICE_STANDALONE)
UX_SLAVE_TRANSFER       *transfer;
#endif

    UX_PARAMETER_NOT_USED(dcd_sim_slave);

    /* Get the physical endpoint address in the endpoint container.  */
    ed =  (UX_DCD_SIM_SLAVE_ED *) endpoint -> ux_slave_endpoint_ed;

#if defined(UX_DEVICE_STANDALONE)

    /* Transfer pending always considered.  */
    transfer_waiting = UX_DCD_SIM_SLAVE_ED_STATUS_TRANSFER;
#else

    /* Save waiting status for non-zero endpoints.  */
    if (ed -> ux_sim_slave_ed_index)
    {
        transfer_waiting = ed -> ux_sim_slave_ed_status;
        transfer_waiting &= UX_DCD_SIM_SLAVE_ED_STATUS_TRANSFER;
    }
    else
        transfer_waiting = 0;
#endif

    /* Clear pending transfer and stall status.  */
    ed -> ux_sim_slave_ed_status &=  ~(ULONG)(transfer_waiting |
                                            UX_DCD_SIM_SLAVE_ED_STATUS_STALLED |
                                            UX_DCD_SIM_SLAVE_ED_STATUS_DONE);

#if !defined(UX_DEVICE_STANDALONE)

    /* If some thread is pending, signal wakeup.  */
    if (transfer_waiting)
    {
        transfer = &endpoint -> ux_slave_endpoint_transfer_request;
        transfer -> ux_slave_transfer_request_completion_code = UX_TRANSFER_BUS_RESET;
        _ux_device_semaphore_put(&transfer -> ux_slave_transfer_request_semaphore);
    }
#endif

    /* This function never fails.  */
    return(UX_SUCCESS);
}

