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
/*    _ux_utility_memory_compare                          PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function compares two memory blocks.                           */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    memory_source                         Pointer to source             */
/*    memory_destination                    Pointer to destination        */
/*    length                                Number of bytes to compare    */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    UX_SUCCESS                            If blocks are equal           */
/*    UX_ERROR                              If blocks are not equal       */
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
UINT  _ux_utility_memory_compare(VOID *memory_source, VOID *memory_destination, ULONG length)
{

UCHAR *   source;
UCHAR *   destination;


    /* Setup source and destination byte oriented pointers.  */
    source =  (UCHAR *) memory_source;
    destination =  (UCHAR *) memory_destination;

    /* Loop to compare blocks.  */
    while(length--)
    {

        /* Compare a single byte.  */
        if(*destination++ != *source++)
        {

            /* Not equal, return an error.  */
            return(UX_ERROR);
        }
    }

    /* Blocks are equal, return success.  */
    return(UX_SUCCESS);
}

