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
/*    _ux_device_class_audio_uninitialize                 PORTABLE C      */
/*                                                           6.3.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function uninitialize the Audio class.                         */
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
/*    _ux_device_thread_delete              Delete thread used            */
/*    _ux_utility_memory_free               Free used local memory        */
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
/*  01-31-2022     Chaoqiong Xiao           Modified comment(s),          */
/*                                            refined macros names,       */
/*                                            added feedback support,     */
/*                                            fixed stream uninitialize,  */
/*                                            resulting in version 6.1.10 */
/*  04-25-2022     Chaoqiong Xiao           Modified comment(s),          */
/*                                            fixed standalone compile,   */
/*                                            resulting in version 6.1.11 */
/*  10-31-2023     Chaoqiong Xiao           Modified comment(s),          */
/*                                            added a new mode to manage  */
/*                                            endpoint buffer in classes  */
/*                                            with zero copy enabled,     */
/*                                            resulting in version 6.3.0  */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_audio_uninitialize(UX_SLAVE_CLASS_COMMAND *command)
{

UX_DEVICE_CLASS_AUDIO           *audio;
UX_DEVICE_CLASS_AUDIO_STREAM    *stream;
UX_SLAVE_CLASS                  *audio_class;
ULONG                            i;


    /* Get the class container.  */
    audio_class =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    audio = (UX_DEVICE_CLASS_AUDIO *) audio_class -> ux_slave_class_instance;

    /* Sanity check.  */
    if (audio != UX_NULL)
    {

        /* Free the stream resources.  */
        stream = (UX_DEVICE_CLASS_AUDIO_STREAM *)((UCHAR *)audio + sizeof(UX_DEVICE_CLASS_AUDIO));
        for (i = 0; i < audio -> ux_device_class_audio_streams_nb; i ++)
        {
#if !defined(UX_DEVICE_STANDALONE)
            _ux_device_thread_delete(&stream -> ux_device_class_audio_stream_thread);
#if defined(UX_DEVICE_CLASS_AUDIO_FEEDBACK_SUPPORT)
            if (stream -> ux_device_class_audio_stream_feedback_thread_stack)
            {
                _ux_device_thread_delete(&stream -> ux_device_class_audio_stream_feedback_thread);
                _ux_utility_memory_free(stream -> ux_device_class_audio_stream_feedback_thread_stack);
            }
#endif
            _ux_utility_memory_free(stream -> ux_device_class_audio_stream_thread_stack);
#endif
            _ux_utility_memory_free(stream -> ux_device_class_audio_stream_buffer);

#if UX_DEVICE_ENDPOINT_BUFFER_OWNER == 1
#if defined(UX_DEVICE_CLASS_AUDIO_FEEDBACK_SUPPORT)
            _ux_utility_memory_free(stream -> ux_device_class_audio_stream_feedback_buffer);
#endif
#endif

            /* Next stream instance.  */
            stream ++;
        }

#if defined(UX_DEVICE_CLASS_AUDIO_INTERRUPT_SUPPORT)
#if UX_DEVICE_ENDPOINT_BUFFER_OWNER == 1
        _ux_utility_memory_free(audio -> ux_device_class_audio_interrupt_buffer);
#endif
#if !defined(UX_DEVICE_STANDALONE)
        _ux_device_thread_delete(&audio_class -> ux_slave_class_thread);
        _ux_utility_memory_free(audio_class -> ux_slave_class_thread_stack);

        _ux_device_semaphore_delete(&audio -> ux_device_class_audio_status_semaphore);
        _ux_device_mutex_delete(&audio -> ux_device_class_audio_status_mutex);
#else
#endif
#endif

        /* Free the audio instance with controls and streams.  */
        _ux_utility_memory_free(audio);
    }

    /* Return completion status.  */
    return(UX_SUCCESS);
}
