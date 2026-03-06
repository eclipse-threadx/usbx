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
/**                                                                       */
/** USBX Component                                                        */
/**                                                                       */
/**   Device DFU Class                                                    */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_dfu.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_dfu_state_get                      PORTABLE C      */
/*                                                           6.1.6        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function gets the USB DFU device state.                        */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    dfu                                   Pointer to DFU instance       */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    DFU state                                                           */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application Code                                                    */
/*    USBX Source Code                                                    */
/*                                                                        */
/**************************************************************************/
UCHAR  _ux_device_class_dfu_state_get(UX_SLAVE_CLASS_DFU *dfu)
{
    UX_PARAMETER_NOT_USED(dfu);
    return (UCHAR)_ux_system_slave -> ux_system_slave_device_dfu_state_machine;
}
