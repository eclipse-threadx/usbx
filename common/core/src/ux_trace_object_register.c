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
extern VOID _tx_trace_object_register(UCHAR , VOID *, CHAR *, ULONG , ULONG );
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_trace_object_register                           PORTABLE C      */
/*                                                           6.1.9        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function registers a USBX object in the trace registry.        */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    object_type                           Type of system object         */
/*    object_ptr                            Address of system object      */
/*    object_name                           Name of system object         */
/*    parameter_1                           Supplemental parameter 1      */
/*    parameter_2                           Supplemental parameter 2      */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _tx_trace_object_register             Actual register function      */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application Initialization                                          */
/*                                                                        */
/**************************************************************************/
VOID  _ux_trace_object_register(UCHAR object_type, VOID *object_ptr, CHAR *object_name, ULONG parameter_1, ULONG parameter_2)
{

UX_INTERRUPT_SAVE_AREA


    /* Disable interrupts.  */
    UX_DISABLE

    /* Call actual object register function.  */
    _tx_trace_object_register(object_type, object_ptr, object_name, parameter_1, parameter_2);

    /* Restore interrupts.  */
    UX_RESTORE
}
#endif

