/***************************************************************************
 * Copyright (c) 2025-present Eclipse ThreadX Contributors
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
/*    _ux_dcd_sim_slave_uninitialize                      PORTABLE C      */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function uninitializes the USB simulation slave controller.    */
/*    The controller releases its resources (memory ...).                 */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_utility_memory_free               Free memory block             */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/**************************************************************************/
UINT  _ux_dcd_sim_slave_uninitialize(VOID)
{

UX_SLAVE_DCD            *dcd;
UX_DCD_SIM_SLAVE        *dcd_sim_slave;

    /* Get the pointer to the DCD.  */
    dcd = &_ux_system_slave -> ux_system_slave_dcd;

    /* If already unused, treat as success.  */
    if (dcd -> ux_slave_dcd_status == UX_UNUSED)
        return(UX_SUCCESS);

    /* Set the state of the controller to HALTED first.  */
    dcd -> ux_slave_dcd_status = UX_DCD_STATUS_HALTED;

    /* Free the simulated slave controller.  */
    dcd_sim_slave = (UX_DCD_SIM_SLAVE *) dcd -> ux_slave_dcd_controller_hardware;
    if (dcd_sim_slave != UX_NULL)
        _ux_utility_memory_free(dcd_sim_slave);

    /* Clear DCD bindings.  */
    dcd -> ux_slave_dcd_controller_hardware = UX_NULL;
    dcd -> ux_slave_dcd_function = UX_NULL;

    /* Set the state of the controller to UNUSED.  */
    dcd -> ux_slave_dcd_status = UX_UNUSED;

    return(UX_SUCCESS);
}
