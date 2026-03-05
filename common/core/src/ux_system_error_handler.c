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
/**   System                                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"


#ifndef UX_DISABLE_ERROR_HANDLER
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_system_error_handler                            PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function saves the last critical error from USBX functions.    */
/*    It is mainly used for debugging purpose to trap where error occurred*/
/*                                                                        */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*   error_code                                                           */
/*                                                                        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/**************************************************************************/
VOID   _ux_system_error_handler(UINT system_level, UINT system_context, UINT error_code)
{

    /* Save the last system error code.  */
    _ux_system -> ux_system_last_error =  error_code;

    /* Increment the total number of system errors.  */
    _ux_system -> ux_system_error_count++;

    /* Is there an application call back function to call ? */
    if (_ux_system -> ux_system_error_callback_function != UX_NULL)
    {

        /* The callback function is defined, call it.  */
        _ux_system -> ux_system_error_callback_function(system_level, system_context, error_code);
    }
}
#endif
