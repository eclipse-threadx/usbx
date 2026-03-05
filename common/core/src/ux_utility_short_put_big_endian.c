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
/*    _ux_utility_short_put_big_endian                    PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function writes a 16-bit value in big endian format.           */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    address                               Destination address           */
/*    value                                 16-bit value                  */
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
/*    USBX Components                                                     */
/*                                                                        */
/**************************************************************************/
VOID  _ux_utility_short_put_big_endian(UCHAR * address, USHORT value)
{

USHORT  low_byte_value;
USHORT  high_byte_value;


    /* First we swap the value bytes. */
    low_byte_value =  value >> 8;
    high_byte_value =  (USHORT)(value<< 8);
    value =  high_byte_value | low_byte_value;

    /* In order to make this function endian agnostic and memory alignment
       independent, we write a byte at a time from the address.  */
    *address++ =  (UCHAR) (value & 0xff);
    *address=     (UCHAR) ((value >> 8) & 0xff);

    /* Return to caller. */
    return;
}

