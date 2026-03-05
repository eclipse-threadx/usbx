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
/**   HID Class                                                           */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_hid.h"
#include "ux_host_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_host_class_hid_item_data_get                    PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function returns a data from the descriptor based on its size, */
/*    format and whether it is signed or not.                             */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    descriptor                            Pointer to descriptor         */
/*    item                                  Pointer to item               */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    ULONG data value                                                    */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_utility_short_get                 Get 16-bit value              */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    HID Class                                                           */
/*                                                                        */
/**************************************************************************/
ULONG  _ux_host_class_hid_item_data_get(UCHAR *descriptor, UX_HOST_CLASS_HID_ITEM *item)
{

ULONG       value;


    /* Get the length of the item.  */
    switch (item -> ux_host_class_hid_item_report_length)
    {

    case 1:

        value =  (ULONG) *descriptor;
        break;


    case 2:

        value =  (ULONG) _ux_utility_short_get(descriptor);
        break;


    case 4:

        value =  (ULONG) _ux_utility_long_get(descriptor);
        break;


    default:

        return(0);
    }

    /* Return the value.  */
    return(value);
}

