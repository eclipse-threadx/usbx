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
/**   Pictbridge Application                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_pictbridge.h"
#include "ux_device_stack.h"
#include "ux_device_class_pima.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_pictbridge_dpsclient_register_event_callback_function           */
/*                                                        PORTABLE C      */
/*                                                           6.1          */
/*                                                                        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function register user callback function                       */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    pictbridge                            Pictbridge instance           */
/*    event_callback_function               Pointer to callback function  */
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
/*    user application                                                    */
/*                                                                        */
/**************************************************************************/
UINT  _ux_pictbridge_dpsclient_register_event_callback_function(UX_PICTBRIDGE *pictbridge, UINT (*event_callback_function)(struct UX_PICTBRIDGE_STRUCT *pictbridge, UINT event_flag))
{

    /* Store the user callback function.  */
    pictbridge -> ux_pictbridge_dps_event_callback_function = event_callback_function;

    /* Always return success.  */
    return(UX_SUCCESS);
}

