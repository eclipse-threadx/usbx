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
/*    _ux_host_class_hid_report_item_analyse              PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function gets the report descriptor and analyzes it.           */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    descriptor                            Pointer to descriptor         */
/*    length                                Length of descriptor          */
/*    item                                  Pointer to item               */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    HID Class                                                           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_hid_report_item_analyse(UCHAR *descriptor, ULONG length, UX_HOST_CLASS_HID_ITEM *item)
{

UCHAR       item_byte;
UINT        result = UX_SUCCESS;

    /* Make sure descriptor has minimal length.*/
    if (length == 0)
    {
        return(UX_DESCRIPTOR_CORRUPTED);
    }

    /* Get the first byte from the descriptor.  */
    item_byte =  *descriptor;

    /*  We need to determine if this is a short or long item.
        For long items, the tag is always 1111.  */
    if ((item_byte & UX_HOST_CLASS_HID_ITEM_TAG_MASK) == UX_HOST_CLASS_HID_ITEM_TAG_LONG)
    {

        /* We have a long item, mark its format.  */
        item -> ux_host_class_hid_item_report_format =  UX_HOST_CLASS_HID_ITEM_TAG_LONG;

        /* Set the type.  */
        item -> ux_host_class_hid_item_report_type =  (item_byte >> 2) & 3;

        /* Make sure descriptor has minimal length.*/
        if (length >= 3)
        {
            /* Get its length (byte 1).  */
            item -> ux_host_class_hid_item_report_length =  (USHORT) *(descriptor + 1);

            /* Then the tag (byte 2).  */
            item -> ux_host_class_hid_item_report_tag =  *(descriptor + 2);
        }
        else
        {
            result = UX_DESCRIPTOR_CORRUPTED;
        }
    }
    else
    {

        /* We have a short item. Mark its format */
        item -> ux_host_class_hid_item_report_format =  UX_HOST_CLASS_HID_ITEM_TAG_SHORT;

        /* Get the length of the item.  */
        switch (item_byte & UX_HOST_CLASS_HID_ITEM_LENGTH_MASK)
        {

        case 3:

            item -> ux_host_class_hid_item_report_length =  4;
            break;

        default:

            item -> ux_host_class_hid_item_report_length =  item_byte & UX_HOST_CLASS_HID_ITEM_LENGTH_MASK;
            break;
        }

        /* Set the type.  */
        item -> ux_host_class_hid_item_report_type =  (item_byte >> 2) & 3;

        /* Then the tag.  */
        item -> ux_host_class_hid_item_report_tag =  (item_byte >> 4) & 0xf;

        /* Mark its format. For short items, this is always 1. */
        item -> ux_host_class_hid_item_report_format = 1;

    }

    /* Return result.  */
    return(result);
}
