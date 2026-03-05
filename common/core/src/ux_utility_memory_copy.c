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
/*    _ux_utility_memory_copy                             PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function copies a block of memory from a source to a           */
/*    destination.                                                        */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    memory_destination                    Pointer to destination        */
/*    memory_source                         Pointer to source             */
/*    length                                Number of bytes to copy       */
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
VOID  _ux_utility_memory_copy(VOID *memory_destination, VOID *memory_source, ULONG length)
{

UCHAR *   source;
UCHAR *   destination;

    /* Setup byte oriented source and destination pointers.  */
    source =  (UCHAR *) memory_source;
    destination =  (UCHAR *) memory_destination;

    /* Loop to perform the copy.  */
    while(length--)
    {

        /* Copy one byte.  */
        *destination++ =  *source++;
    }

    /* Return to caller.  */
    return;
}

