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
/**   Utility                                                             */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"


#if !defined(UX_STANDALONE)
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_utility_timer_create                            PORTABLE C      */
/*                                                           6.1.11       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function creates a timer.                                      */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    timer                                 Pointer to timer              */
/*    timer_name                            Name of timer                 */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    tx_timer_create                       ThreadX timer create          */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Components                                                     */
/*                                                                        */
/**************************************************************************/
UINT  _ux_utility_timer_create(UX_TIMER *timer, CHAR *timer_name, VOID (*expiration_function) (ULONG),
                                ULONG expiration_input, ULONG initial_ticks, ULONG reschedule_ticks,
                                UINT activation_flag)
{

UINT    status;


    /* Call ThreadX to create the timer object.  */
    status =  tx_timer_create(timer, (CHAR *) timer_name, expiration_function, expiration_input,
                                initial_ticks, reschedule_ticks, activation_flag);

    /* Check status.  */
    if (status != UX_SUCCESS)

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_UTILITY, status);

    /* Return completion status.  */
    return(status);
}
#endif
