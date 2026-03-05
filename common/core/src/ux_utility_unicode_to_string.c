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
/*    _ux_utility_unicode_to_string                       PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function converts a unicode string to a zero terminated        */
/*    ascii string.                                                       */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    source                                Unicode String                */
/*    destination                           Ascii String                  */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    none                                                                */
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
VOID  _ux_utility_unicode_to_string(UCHAR *source, UCHAR *destination)
{
ULONG   string_length;

    /* Obtain the length of the unicode string.  This is the first byte*/
    string_length = (ULONG) *source++;

    /* Parse the unicode string. First byte is always 0, second byte is the
       ASCII character.  */
    while(string_length--)
    {
        /* First character is from the source.  */
        *destination++ = *source++;

        /* Second character of unicode word is 0.  */
        source++;
    }

    /* We are done with the ascii string. Insert a zero at the end. */
    *destination = 0;

    /* Finished.  */
    return;
}

