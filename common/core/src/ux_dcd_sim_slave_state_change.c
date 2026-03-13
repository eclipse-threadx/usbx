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
#include "ux_hcd_sim_host.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_dcd_sim_slave_state_change                      PORTABLE C      */
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will set the state of the controller to the desired   */
/*    value.                                                              */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    dcd_sim_slave                         Pointer to device controller  */
/*    state                                 Desired state                 */
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
UINT  _ux_dcd_sim_slave_state_change(UX_DCD_SIM_SLAVE *dcd_sim_slave, ULONG state)
{

UX_HCD              *hcd;


    UX_PARAMETER_NOT_USED(state);

    if (state == UX_DEVICE_FORCE_DISCONNECT)
    {

        /* Simulate port detach & attach on host side.  */

        /* Get HCD.  */
        hcd = (UX_HCD *)dcd_sim_slave -> ux_dcd_sim_slave_hcd;

        /* Something happened on this port. Signal it to the root hub thread.  */
        if (hcd)
        {
            hcd -> ux_hcd_root_hub_signal[0] = 2;
            _ux_host_semaphore_put(&_ux_system_host -> ux_system_host_enum_semaphore);
        }
    }

    /* Nothing to do in simulation mode.  */
    return(UX_SUCCESS);
}

