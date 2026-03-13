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
/**   Video Class                                                         */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_video.h"
#include "ux_host_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_host_class_video_max_payload_get                PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function returns the maximum transfer size in a single payload */
/*    transfer.                                                           */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    video                                 Pointer to video class        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Maximum payload transfer size                                       */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/**************************************************************************/
ULONG  _ux_host_class_video_max_payload_get(UX_HOST_CLASS_VIDEO *video)
{

    /* Return the maximum payload size.  */
    return(video ->ux_host_class_video_current_max_payload_size);
}

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _uxe_host_class_video_max_payload_get               PORTABLE C      */
/*                                                           6.3.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Yajun Xia, Microsoft Corporation                                    */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks errors in video max payload get function call. */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    video                                 Pointer to video class        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Maximum payload transfer size                                       */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_host_class_video_max_payload_get  Video max payload get         */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/**************************************************************************/
ULONG  _uxe_host_class_video_max_payload_get(UX_HOST_CLASS_VIDEO *video)
{

    /* Sanity checks.  */
    if (video == UX_NULL)
        return(0);

    /* Call the actual video max payload get function.  */
    return(_ux_host_class_video_max_payload_get(video));
}