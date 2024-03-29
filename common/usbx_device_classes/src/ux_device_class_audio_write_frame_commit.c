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
/*    _ux_device_class_audio_write_frame_commit           PORTABLE C      */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function set frame buffer valid to send in the Audio class.    */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    stream                                Address of audio stream       */
/*                                            instance                    */
/*    buffer                                Pointer to buffer to save     */
/*                                            frame data                  */
/*    length                                Frame length in bytes         */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
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
/*                                            fixed frame length check,   */
/*                                            resulting in version 6.1.10 */
/*  03-08-2023     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.2.1  */
/*                                                                        */
/**************************************************************************/
UINT _ux_device_class_audio_write_frame_commit(UX_DEVICE_CLASS_AUDIO_STREAM *stream, ULONG length)
{

UX_SLAVE_ENDPOINT           *endpoint;
UX_SLAVE_DEVICE             *device;
UCHAR                       *next_pos;


    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* As long as the device is in the CONFIGURED state.  */
    if (device -> ux_slave_device_state != UX_DEVICE_CONFIGURED)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_CONFIGURATION_HANDLE_UNKNOWN);

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

    /* Check overflow!!  */
    if (stream -> ux_device_class_audio_stream_access_pos == stream -> ux_device_class_audio_stream_transfer_pos &&
        stream -> ux_device_class_audio_stream_access_pos -> ux_device_class_audio_frame_length != 0)
        return(UX_BUFFER_OVERFLOW);

    /* Check frame length.  */
    if ((stream -> ux_device_class_audio_stream_frame_buffer_size - 8) < length)
        return(UX_ERROR);

    /* Calculate next frame buffer.  */
    next_pos = (UCHAR *)stream -> ux_device_class_audio_stream_access_pos;
    next_pos += stream -> ux_device_class_audio_stream_frame_buffer_size;
    if (next_pos >= stream -> ux_device_class_audio_stream_buffer + stream -> ux_device_class_audio_stream_buffer_size)
        next_pos = stream -> ux_device_class_audio_stream_buffer;

    /* Commit frame length.  */
    stream -> ux_device_class_audio_stream_access_pos -> ux_device_class_audio_frame_length = length;

    /* Move frame position.  */
    stream -> ux_device_class_audio_stream_access_pos = (UX_DEVICE_CLASS_AUDIO_FRAME *)next_pos;

    return(UX_SUCCESS);
}


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _uxe_device_class_audio_write_frame_commit          PORTABLE C      */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks errors in setting frame buffer function call.  */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    stream                                Address of audio stream       */
/*                                            instance                    */
/*    buffer                                Pointer to buffer to save     */
/*                                            frame data                  */
/*    length                                Frame length in bytes         */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_class_audio_write_frame_commit                           */
/*                                          Append a frame to FIFO        */
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
UINT _uxe_device_class_audio_write_frame_commit(UX_DEVICE_CLASS_AUDIO_STREAM *stream, ULONG length)
{

    /* Sanity checks.  */
    if (stream == UX_NULL || length < 1)
        return(UX_INVALID_PARAMETER);

    return(_ux_device_class_audio_write_frame_commit(stream, length));
}
