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


/* DEPRECATION NOTICE
 * _ux_utility_string_length_get() is deprecated. Do not use it in new code.
 *
 * WHY: this function scans the string until it finds a NUL byte with no
 * upper bound on the search. If the buffer is not properly NUL-terminated,
 * the scan reads past the end of the buffer, which is undefined behaviour.
 *
 * WHAT TO DO: replace calls with _ux_utility_string_length_check(), passing
 * the maximum buffer length as an additional argument.
 */
#pragma message("_ux_utility_string_length_get() is deprecated. " \
                "Use _ux_utility_string_length_check() and pass the buffer length.")

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_utility_string_length_get                       PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    DEPRECATED. Use _ux_utility_string_length_check() instead, passing  */
/*    the maximum buffer length. This function scans without an upper     */
/*    bound; a non-NUL-terminated buffer causes an overread.              */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    string                                Pointer to string             */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    length of string                                                    */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Components                                                     */
/*                                                                        */
/**************************************************************************/
ULONG  _ux_utility_string_length_get(UCHAR *string)
{

ULONG       length =  0;

    /* Loop to find the length of the string.  */
    length =  0;
    while (string[length])
    {

        /* Move to next position.  */
        length++;
    }

    /* Return length to caller.  */
    return(length);
}

