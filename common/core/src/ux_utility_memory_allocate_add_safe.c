/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation 
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
/*    _ux_utility_memory_allocate_add_safe                PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function allocates a block of memory for the specified added   */
/*    size and alignment.                                                 */
/*                                                                        */
/*    Note if adding result overflow, no memory is allocated.             */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    align                                 Memory alignment required     */
/*    cache                                 Memory pool source            */
/*    size_add_a                            Number of bytes a to add      */
/*    size_add_b                            Number of bytes b to add      */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Pointer to block of memory                                          */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_utility_memory_allocate           Allocate block of memory      */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Components                                                     */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*                                                                        */
/**************************************************************************/
VOID* _ux_utility_memory_allocate_add_safe(ULONG align,ULONG cache,ULONG size_add_a,ULONG size_add_b)
{
    return UX_UTILITY_MEMORY_ALLOCATE_ADD_SAFE(align, cache, size_add_a, size_add_b);
}
