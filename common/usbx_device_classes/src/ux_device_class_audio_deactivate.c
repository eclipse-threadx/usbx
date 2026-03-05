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
/*    _ux_device_class_audio_deactivate                   PORTABLE C      */
/*                                                           6.1.12       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function deactivate an instance of the audio class.            */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    command                               Pointer to a class command    */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_stack_transfer_all_request_abort                         */
/*                                          Abort all transfers           */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Device Audio Class                                                  */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_audio_deactivate(UX_SLAVE_CLASS_COMMAND *command)
{

UX_DEVICE_CLASS_AUDIO           *audio;
UX_DEVICE_CLASS_AUDIO_STREAM    *stream;
UX_SLAVE_ENDPOINT               *endpoint;
UX_SLAVE_CLASS                  *class_ptr;
UINT                             i;


    /* Get the class container.  */
    class_ptr =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    audio = (UX_DEVICE_CLASS_AUDIO *) class_ptr -> ux_slave_class_instance;

    /* Stop pending streams.  */
    stream = audio -> ux_device_class_audio_streams;
    for (i = 0; i < audio -> ux_device_class_audio_streams_nb; i ++)
    {

        /* Locate the endpoint.  */
        endpoint = stream -> ux_device_class_audio_stream_endpoint;

        /* Terminate the transactions pending on the endpoint.  */
        if (endpoint)
            _ux_device_stack_transfer_all_request_abort(endpoint, UX_TRANSFER_BUS_RESET);

        /* Free the stream.  */
        stream -> ux_device_class_audio_stream_endpoint = UX_NULL;
        stream -> ux_device_class_audio_stream_interface = UX_NULL;

        stream ++;
    }

    /* Free the control.  */
    audio -> ux_device_class_audio_interface = UX_NULL;

    /* If there is a deactivate function call it.  */
    if (audio -> ux_device_class_audio_callbacks.ux_slave_class_audio_instance_deactivate != UX_NULL)

        /* Invoke the application.  */
        audio -> ux_device_class_audio_callbacks.ux_slave_class_audio_instance_deactivate(audio);

    /* Return completion status.  */
    return(UX_SUCCESS);
}
