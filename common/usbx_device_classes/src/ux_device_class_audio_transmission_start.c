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
/**   Device Audio Class                                                  */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_audio.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_audio_transmission_start           PORTABLE C      */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function start sending valid frames in the Audio class.        */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    stream                                Address of audio stream       */
/*                                            instance                    */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_thread_resume              Resume thread used            */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  01-31-2022     Chaoqiong Xiao           Modified comment(s),          */
/*                                            refined macros names,       */
/*                                            resulting in version 6.1.10 */
/*  10-31-2022     Yajun Xia                Modified comment(s),          */
/*                                            added standalone support,   */
/*                                            resulting in version 6.2.0  */
/*  03-08-2023     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.2.1  */
/*                                                                        */
/**************************************************************************/
UINT _ux_device_class_audio_transmission_start(UX_DEVICE_CLASS_AUDIO_STREAM *stream)
{

UX_SLAVE_ENDPOINT           *endpoint;
UX_SLAVE_DEVICE             *device;


    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* As long as the device is in the CONFIGURED state.  */
    if (device -> ux_slave_device_state != UX_DEVICE_CONFIGURED)
    {

        /* Cannot proceed with command, the interface is down.  */
        return(UX_CONFIGURATION_HANDLE_UNKNOWN);
    }

    /* Check if endpoint is available.  */
    endpoint = stream -> ux_device_class_audio_stream_endpoint;
    if (endpoint == UX_NULL)
        return(UX_ERROR);

    /* Check if endpoint direction is OK.  */
    if ((endpoint -> ux_slave_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) == UX_ENDPOINT_OUT)
        return(UX_ERROR);

    /* Check if there is frame to send (underflow).  */
    if (stream -> ux_device_class_audio_stream_transfer_pos -> ux_device_class_audio_frame_length == 0)
        return(UX_BUFFER_OVERFLOW);

#if defined(UX_DEVICE_STANDALONE)

    /* Start write task.  */
    if (stream -> ux_device_class_audio_stream_task_state == UX_DEVICE_CLASS_AUDIO_STREAM_RW_STOP)
        stream -> ux_device_class_audio_stream_task_state = UX_DEVICE_CLASS_AUDIO_STREAM_RW_START;
#else

    /* Start write thread.  */
    _ux_device_thread_resume(&stream -> ux_device_class_audio_stream_thread);
#endif
    return(UX_SUCCESS);
}


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _uxe_device_class_audio_transmission_start          PORTABLE C      */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks errors in starting sending function call.      */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    stream                                Address of audio stream       */
/*                                            instance                    */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_class_audio_transmission_start                           */
/*                                          Start transmission            */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  03-08-2023     Chaoqiong Xiao           Initial Version 6.2.1         */
/*                                                                        */
/**************************************************************************/
UINT _uxe_device_class_audio_transmission_start(UX_DEVICE_CLASS_AUDIO_STREAM *stream)
{

    /* Sanity check.  */
    if (stream == UX_NULL)
        return(UX_INVALID_PARAMETER);

    /* Start transmission.  */
    return(_ux_device_class_audio_transmission_start(stream));
}
