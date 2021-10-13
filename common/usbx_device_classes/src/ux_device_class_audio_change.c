/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
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
/*    _ux_device_class_audio_change                       PORTABLE C      */
/*                                                           6.1.9        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function changes the interface of the Audio device             */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    command                           Pointer to audio command          */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_system_error_handler          System error trap                 */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Device Audio Class                                                  */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  10-15-2021     Chaoqiong Xiao           Modified comment(s),          */
/*                                            replaced wMaxPacketSize by  */
/*                                            calculated payload size,    */
/*                                            resulting in version 6.1.9  */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_audio_change(UX_SLAVE_CLASS_COMMAND *command)
{

UX_DEVICE_CLASS_AUDIO                   *audio;
UX_DEVICE_CLASS_AUDIO_STREAM            *stream;
UX_SLAVE_CLASS                          *class;
UX_SLAVE_INTERFACE                      *interface;
UX_SLAVE_ENDPOINT                       *endpoint;
UCHAR                                   *frame_buffer;
ULONG                                    stream_index;


    /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    audio = (UX_DEVICE_CLASS_AUDIO *) class -> ux_slave_class_instance;

    /* Get the interface that owns this instance.  */
    interface =  (UX_SLAVE_INTERFACE  *) command -> ux_slave_class_command_interface;

    /* Get the interface number (base 0).  */
    if (audio -> ux_device_class_audio_interface)
    {

        /* If IAD used, calculate stream index based on interface number.  */
        stream_index  = interface -> ux_slave_interface_descriptor.bInterfaceNumber;
        stream_index -= audio -> ux_device_class_audio_interface -> ux_slave_interface_descriptor.bInterfaceNumber;
        stream_index --;
    }
    else

        /* One stream for one driver!  */
        stream_index  = 0;

    /* Get the stream instance.  */
    stream = &audio -> ux_device_class_audio_streams[stream_index];

    /* Update the interface.  */
    stream -> ux_device_class_audio_stream_interface = interface;

    /* If the interface to mount has a non zero alternate setting, the class is really active with
       the endpoints active.  If the interface reverts to alternate setting 0, it needs to have
       the pending transactions terminated.  */
    if (interface -> ux_slave_interface_descriptor.bAlternateSetting != 0)
    {

        /* Locate the endpoints.  ISO IN/OUT for Streaming Interface.  */
        endpoint = interface -> ux_slave_interface_first_endpoint;

        /* Parse all endpoints.  */
        stream -> ux_device_class_audio_stream_endpoint = UX_NULL;
        while (endpoint != UX_NULL)
        {

            /* Check the endpoint.  */
            if ((endpoint -> ux_slave_endpoint_descriptor.bmAttributes &
                    (UX_DEVICE_CLASS_AUDIO_EP_TRANSFER_TYPE_MASK | UX_DEVICE_CLASS_AUDIO_EP_USAGE_TYPE_MASK)) == UX_ISOCHRONOUS_ENDPOINT)
            {

                /* We found the endpoint, check its size.  */
                if (endpoint -> ux_slave_endpoint_transfer_request.ux_slave_transfer_request_transfer_length > stream -> ux_device_class_audio_stream_frame_buffer_size - 8)
                {

                    /* Error trap!  */
                    _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_MEMORY_INSUFFICIENT);

                    /* Frame buffer too small for endpoints.  */
                    return(UX_MEMORY_INSUFFICIENT);
                }

                /* Save it.  */
                stream -> ux_device_class_audio_stream_endpoint = endpoint;
                break;
            }

            /* Next endpoint.  */
            endpoint =  endpoint -> ux_slave_endpoint_next_endpoint;
        }

        /* Now check if all endpoints have been found.  */
        if (stream -> ux_device_class_audio_stream_endpoint == UX_NULL)
        {

            /* Error trap!  */
            _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_DESCRIPTOR_CORRUPTED);

            /* Not all endpoints have been found. Major error, do not proceed.  */
            return(UX_DESCRIPTOR_CORRUPTED);
        }

        /* Now reset frame buffers.  */
        frame_buffer = stream -> ux_device_class_audio_stream_buffer;
        while(frame_buffer < stream -> ux_device_class_audio_stream_buffer + stream -> ux_device_class_audio_stream_buffer_size)
        {

            /* Reset header information.  */
            *((ULONG *) frame_buffer     ) = 0;
            *((ULONG *)(frame_buffer + 4)) = 0;

            /* Next.  */
            frame_buffer += stream -> ux_device_class_audio_stream_frame_buffer_size;
        }
        stream -> ux_device_class_audio_stream_transfer_pos = stream -> ux_device_class_audio_stream_access_pos;
    }
    else
    {

        /* There is no data endpoint.  */
        stream -> ux_device_class_audio_stream_endpoint = UX_NULL;

        /* In this case, we are reverting to the Alternate Setting 0.  We need to terminate the pending transactions.  */
        /* Endpoints actually aborted and destroyed before change command.  */
        /*
        _ux_device_stack_transfer_all_request_abort(stream -> ux_device_class_audio_stream_endpoint, UX_TRANSFER_APPLICATION_RESET);
         */
    }

    /* Invoke stream change callback.  */
    if (stream -> ux_device_class_audio_stream_callbacks.ux_device_class_audio_stream_change)
        stream -> ux_device_class_audio_stream_callbacks.ux_device_class_audio_stream_change(stream, interface -> ux_slave_interface_descriptor.bAlternateSetting);

    /* Return completion status.  */
    return(UX_SUCCESS);
}
