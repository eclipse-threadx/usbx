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
/**************************************************************************/
/**                                                                       */ 
/** USBX Component                                                        */ 
/**                                                                       */
/**   Audio Class                                                         */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_audio.h"
#include "ux_host_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_audio_write                          PORTABLE C      */ 
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function writes to the audio streaming interface.              */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    audio                                 Pointer to audio class        */ 
/*    audio_transfer_request                Pointer to transfer request   */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_class_audio_transfer_request Start audio transfer request  */ 
/*    _ux_host_stack_class_instance_verify  Verify instance is valid      */ 
/*    _ux_host_semaphore_get                Get semaphore                 */ 
/*    _ux_host_semaphore_put                Put semaphore                 */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Application                                                         */ 
/*    Audio Class                                                         */ 
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
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_audio_write(UX_HOST_CLASS_AUDIO *audio, UX_HOST_CLASS_AUDIO_TRANSFER_REQUEST *audio_transfer_request)
{

UINT        status;

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_AUDIO_WRITE, audio, audio_transfer_request -> ux_host_class_audio_transfer_request_data_pointer, 
                            audio_transfer_request -> ux_host_class_audio_transfer_request_requested_length, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

    /* Ensure the instance is valid.  */
    if (_ux_host_stack_class_instance_verify(_ux_system_host_class_audio_name, (VOID *) audio) != UX_SUCCESS)
    {        

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_INSTANCE_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_HOST_CLASS_INSTANCE_UNKNOWN, audio, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_HOST_CLASS_INSTANCE_UNKNOWN);
    }

    /* Protect thread reentry to this instance.  */
    status =  _ux_host_semaphore_get(&audio -> ux_host_class_audio_semaphore, UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
        return(status);

    /* By default the transmission will be successful.  */
    status =  UX_SUCCESS;
    
    /* Ensure we have a selected interface that allows isoch transmission.  */
    if (audio -> ux_host_class_audio_isochronous_endpoint -> ux_endpoint_descriptor.wMaxPacketSize == 0)
    {

        /* Unprotect thread reentry to this instance.  */
        status =  _ux_host_semaphore_put(&audio -> ux_host_class_audio_semaphore);

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_AUDIO_WRONG_INTERFACE);

        /* Return error status.  */
        return(UX_HOST_CLASS_AUDIO_WRONG_INTERFACE);
    }
    
    /* Hook the audio transfer request to the chain of transfer requests in the audio instance.
       We hook the transfer request to the tail transfer request.  */
    audio_transfer_request -> ux_host_class_audio_transfer_request_next_audio_transfer_request =  audio -> ux_host_class_audio_tail_transfer_request;
    audio -> ux_host_class_audio_tail_transfer_request =  audio_transfer_request;

    /* Check if this is the first time we have a transfer request, if so update the head as well.  */
    if (audio -> ux_host_class_audio_head_transfer_request == UX_NULL)
        audio -> ux_host_class_audio_head_transfer_request =  audio_transfer_request;

    /* Ask the stack to hook this transfer request to the iso ED.  */
    status =  _ux_host_class_audio_transfer_request(audio, audio_transfer_request);

    /* Unprotect thread reentry to this instance.  */
    status =  _ux_host_semaphore_put(&audio -> ux_host_class_audio_semaphore);

    /* Return completion status.  */
    return(status);
}

