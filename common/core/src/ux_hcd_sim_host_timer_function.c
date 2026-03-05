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
/**   Host Simulator Controller Driver                                    */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_hcd_sim_host.h"

#if !defined(UX_HOST_STANDALONE)
#include "tx_timer.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_hcd_sim_host_timer_function                     PORTABLE C      */
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*     This function is the timer function of the simulator. It is        */
/*     invoked on a timer every tick.                                     */
/*                                                                        */
/*     It's for RTOS mode.                                                */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hcd_sim_host_addr                     Address of host controller    */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_utility_semaphore_put             Put semaphore                 */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    ThreadX                                                             */
/*                                                                        */
/**************************************************************************/
VOID  _ux_hcd_sim_host_timer_function(ULONG hcd_sim_host_addr)
{

UX_HCD_SIM_HOST     *hcd_sim_host;
UX_HCD              *hcd;


    /* Setup pointer to simulator host structure.  */
    UX_TIMER_EXTENSION_PTR_GET(hcd_sim_host, UX_HCD_SIM_HOST, hcd_sim_host_addr)

    /* Get the pointers to the generic HCD areas.  */
    hcd =  hcd_sim_host -> ux_hcd_sim_host_hcd_owner;

    /* Increase the interrupt count. This indicates the controller is still alive.  */
    hcd_sim_host -> ux_hcd_sim_host_interrupt_count++;

    /* Check if the controller is operational, if not, skip it.  */
    if (hcd -> ux_hcd_status == UX_HCD_STATUS_OPERATIONAL)
    {

        /* Wake up the thread for the controller transaction processing.  */
        hcd -> ux_hcd_thread_signal++;
        _ux_host_semaphore_put(&_ux_system_host -> ux_system_host_hcd_semaphore);
    }
}
#endif
