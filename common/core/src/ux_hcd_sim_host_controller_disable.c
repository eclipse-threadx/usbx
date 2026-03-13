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


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_hcd_sim_host_controller_disable                 PORTABLE C      */
/*                                                           6.1.2        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will disable the simulated host controller.           */
/*    The controller will release all its resources (memory, IO ...).     */
/*    After this, the controller will not send SOF any longer.            */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hcd_sim_host                          Pointer to host controller    */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Host Simulator Controller Driver                                    */
/*                                                                        */
/**************************************************************************/
UINT  _ux_hcd_sim_host_controller_disable(UX_HCD_SIM_HOST *hcd_sim_host)
{

UX_HCD  *hcd = hcd_sim_host -> ux_hcd_sim_host_hcd_owner;


    /* Set the state of the controller to HALTED.  */
    hcd -> ux_hcd_status =  UX_HCD_STATUS_HALTED;

    /* Return successful completion.  */
    return(UX_SUCCESS);
}
