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


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_utility_long_get                                PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function reads a 32-bit value.                                 */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    address                               Source address                */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    32-bit value                                                        */
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
ULONG  _ux_utility_long_get(UCHAR * address)
{

ULONG    value;


    /* In order to make this function endian agnostic and memory alignment
       independent, we read a byte at a time from the address.  */
    value =   (ULONG) *address++;
    value |=  (ULONG)*address++ << 8;
    value |=  (ULONG)*address++ << 16;
    value |=  (ULONG)*address << 24;

    /* Return 32-bit value.  */
    return(value);
}

