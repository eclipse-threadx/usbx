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
/*    _ux_host_class_video_stop                           PORTABLE C      */
/*                                                           6.1.12       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function stops the video channel.                              */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    video                                 Pointer to video class        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_host_stack_endpoint_transfer_abort                              */
/*                                          Abort outstanding transfer    */
/*    _ux_host_stack_interface_setting_select                             */
/*                                          Select interface              */
/*    _ux_host_semaphore_get                Get semaphore                 */
/*    _ux_host_semaphore_put                Release semaphore             */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Video Class                                                         */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_video_stop(UX_HOST_CLASS_VIDEO *video)
{

UINT                    status;
UX_CONFIGURATION        *configuration;
UX_INTERFACE            *interface_ptr;
UINT                    streaming_interface;


    /* Protect thread reentry to this instance.  */
    status =  _ux_host_semaphore_get(&video -> ux_host_class_video_semaphore, UX_WAIT_FOREVER);

    /* Get the interface number of the video streaming interface.  */
    streaming_interface =  video -> ux_host_class_video_streaming_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* We need to abort transactions on the iso pipe.  */
    if (video -> ux_host_class_video_isochronous_endpoint != UX_NULL)
    {

        /* Abort the iso transfer.  */
        _ux_host_stack_endpoint_transfer_abort(video -> ux_host_class_video_isochronous_endpoint);
    }

    /* We found the alternate setting for the sampling values demanded, now we need
        to search its container.  */
    configuration =        video -> ux_host_class_video_streaming_interface -> ux_interface_configuration;
    interface_ptr =        configuration -> ux_configuration_first_interface;

    /* Scan all interfaces.  */
    while (interface_ptr != UX_NULL)
    {

        /* We search for both the right interface and alternate setting.  */
        if ((interface_ptr -> ux_interface_descriptor.bInterfaceNumber == streaming_interface) &&
            (interface_ptr -> ux_interface_descriptor.bAlternateSetting == 0))
        {

            /* We have found the right interface/alternate setting combination
               The stack will select it for us.  */
            status =  _ux_host_stack_interface_setting_select(interface_ptr);

            /* If the alternate setting for the streaming interface could be selected, we memorize it.  */
            if (status == UX_SUCCESS)
            {

                /* Memorize the interface.  */
                video -> ux_host_class_video_streaming_interface =  interface_ptr;

                /* There is no endpoint for the alternate setting 0.  */
                video -> ux_host_class_video_isochronous_endpoint = UX_NULL;

                /* Unprotect thread reentry to this instance.  */
                _ux_host_semaphore_put(&video -> ux_host_class_video_semaphore);

                /* Return successful completion.  */
                return(UX_SUCCESS);
            }
        }

        /* Move to next interface.  */
        interface_ptr =  interface_ptr -> ux_interface_next_interface;
    }

    /* Unprotect thread reentry to this instance.  */
    _ux_host_semaphore_put(&video -> ux_host_class_video_semaphore);

    /* Return completion status.  */
    return(status);
}

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _uxe_host_class_video_stop                          PORTABLE C      */
/*                                                           6.3.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Yajun Xia, Microsoft Corporation                                    */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks errors in video stop function call.            */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    video                                 Pointer to video class        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_host_class_video_stop             Video stop                    */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Video Class                                                         */
/*                                                                        */
/**************************************************************************/
UINT  _uxe_host_class_video_stop(UX_HOST_CLASS_VIDEO *video)
{

    /* Sanity checks.  */
    if (video == UX_NULL)
        return(UX_INVALID_PARAMETER);

    /* Call the actual video stop function.  */
    return(_ux_host_class_video_stop(video));
}