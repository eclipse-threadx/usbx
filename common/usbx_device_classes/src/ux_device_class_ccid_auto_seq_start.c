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
/**   Device CCID Class                                                   */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_ccid.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_ccid_auto_seq_start                PORTABLE C      */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function indicates auto sequence start of the USB CCID device. */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    ccid                                  Pointer to ccid instance      */
/*    slot                                  Slot inserted                 */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Source Code                                                    */
/*                                                                        */
/**************************************************************************/
UINT _ux_device_class_ccid_auto_seq_start(UX_DEVICE_CLASS_CCID *ccid, ULONG slot)
{

UX_DEVICE_CLASS_CCID_SLOT       *ccid_slot;

    /* Sanity check.  */
    if (slot >= ccid -> ux_device_class_ccid_parameter.ux_device_class_ccid_max_n_slots)
        return(UX_INVALID_PARAMETER);

    /* Get slot instance.  */
    ccid_slot  = ccid -> ux_device_class_ccid_slots;
    ccid_slot += slot;

    /* Lock states.  */
    _ux_device_class_ccid_lock(ccid);

    /* Set state of auto sequencing.  */
    ccid_slot -> ux_device_class_ccid_slot_flags |= UX_DEVICE_CLASS_CCID_FLAG_AUTO_SEQUENCING;

    /* Unlock states.  */
    _ux_device_class_ccid_unlock(ccid);

    /* Return transfer status.  */
    return(UX_SUCCESS);
}
