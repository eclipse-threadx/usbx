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
#include "ux_system.h"
#include "ux_utility.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_system_uninitialize                             PORTABLE C      */
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function uninitializes the various control data structures for */
/*    the USBX system.                                                    */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_utility_mutex_delete                ThreadX delete mutex        */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/**************************************************************************/
UINT  _ux_system_uninitialize(VOID)
{

#ifdef UX_ENABLE_DEBUG_LOG

    /* Free memory for debug log buffer.  */
    if ( _ux_system -> ux_system_debug_log_buffer != UX_NULL)
        _ux_utility_memory_free( _ux_system -> ux_system_debug_log_buffer);
#endif /* UX_ENABLE_DEBUG_LOG */

    /* Delete the Mutex object used by USBX to control critical sections.  */
    _ux_system_mutex_delete(&_ux_system -> ux_system_mutex);

    return(UX_SUCCESS);
}
