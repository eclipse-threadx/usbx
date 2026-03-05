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
/**   Trace                                                               */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#ifndef UX_SOURCE_CODE
#define UX_SOURCE_CODE
#endif


/* Include necessary system files.  */

#include "ux_api.h"

#ifdef UX_ENABLE_EVENT_TRACE
extern VOID _tx_trace_object_unregister(VOID *);
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_trace_object_unregister                         PORTABLE C      */
/*                                                           6.1.9        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function unregisters a USBX object in the trace registry.      */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    object_pointer                        Address of system object      */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _tx_trace_object_unregister           Actual unregister function    */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX delete functions                                               */
/*                                                                        */
/**************************************************************************/
VOID  _ux_trace_object_unregister(VOID *object_ptr)
{

UX_INTERRUPT_SAVE_AREA


    /* Disable interrupts.  */
    UX_DISABLE

    /* Call actual object unregister function.  */
    _tx_trace_object_unregister(object_ptr);

    /* Restore interrupts.  */
    UX_RESTORE
}
#endif
