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
/*    _ux_device_class_audio_feedback_set                 PORTABLE C      */
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function set encoded feedback of the Audio Stream.             */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    stream                                Address of audio stream       */
/*                                            instance                    */
/*    encoded_feedback                      Feedback data (3 or 4 bytes)  */
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
/*  01-31-2022     Chaoqiong Xiao           Initial Version 6.1.10        */
/*                                                                        */
/**************************************************************************/
UINT _ux_device_class_audio_feedback_set(UX_DEVICE_CLASS_AUDIO_STREAM *stream,
                                           UCHAR *encoded_feedback)
{
#if !defined(UX_DEVICE_CLASS_AUDIO_FEEDBACK_SUPPORT)
    UX_PARAMETER_NOT_USED(stream);
    UX_PARAMETER_NOT_USED(encoded_feedback);
    return(UX_FUNCTION_NOT_SUPPORTED);
#else

UX_SLAVE_DEVICE             *device;
UX_SLAVE_ENDPOINT           *endpoint;
UX_SLAVE_TRANSFER           *transfer;
UCHAR                       *buffer;


    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* As long as the device is in the CONFIGURED state.  */
    if (device -> ux_slave_device_state != UX_DEVICE_CONFIGURED)
    {

        /* Cannot proceed with command, the interface is down.  */
        return(UX_CONFIGURATION_HANDLE_UNKNOWN);
    }

    /* Check if endpoint is available.  */
    endpoint = stream -> ux_device_class_audio_stream_feedback;
    if (endpoint == UX_NULL)
        return(UX_ERROR);

    /* Check if endpoint direction is OK.  */
    if ((endpoint -> ux_slave_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) != UX_ENDPOINT_IN)
        return(UX_ERROR);

    /* Transfer buffer.  */
    transfer = &endpoint -> ux_slave_endpoint_transfer_request;
    buffer = transfer -> ux_slave_transfer_request_data_pointer;

    /* Save data.  */
    *buffer++ = *encoded_feedback ++;
    *buffer++ = *encoded_feedback ++;
    *buffer++ = *encoded_feedback ++;
    if (_ux_system_slave -> ux_system_slave_speed == UX_HIGH_SPEED_DEVICE)
        *buffer = *encoded_feedback;
    return(UX_SUCCESS);
#endif
}

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _uxe_device_class_audio_feedback_set                PORTABLE C      */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks errors in encoded feedback setting function    */
/*    call.                                                               */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    stream                                Address of audio stream       */
/*                                            instance                    */
/*    encoded_feedback                      Feedback data (3 or 4 bytes)  */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_class_audio_feedback_set  Set encoded feedback           */
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
UINT _uxe_device_class_audio_feedback_set(UX_DEVICE_CLASS_AUDIO_STREAM *stream,
                                           UCHAR *encoded_feedback)
{

    /* Sanity check on input parameters.  */
    if (stream == UX_NULL || encoded_feedback == UX_NULL)
        return(UX_INVALID_PARAMETER);

    /* Get feedback.  */
    return(_ux_device_class_audio_feedback_set(stream, encoded_feedback));
}
