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
/*    _ux_host_class_audio_deactivate                     PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function is called when this instance of the audio has been    */
/*    removed from the bus either directly or indirectly. The iso pipes   */ 
/*    will be destroyed and the instanced removed.                        */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    command                               Pointer to command            */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_stack_class_instance_destroy Destroy class instance        */ 
/*    _ux_host_stack_endpoint_transfer_abort Abort outstanding transfer   */ 
/*    _ux_utility_semaphore_get             Get semaphore                 */ 
/*    _ux_utility_semaphore_delete          Delete semaphore              */ 
/*    _ux_utility_memory_free               Release memory block          */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Audio Class                                                         */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_audio_deactivate(UX_HOST_CLASS_COMMAND *command)
{

UX_HOST_CLASS_AUDIO     *audio;
UINT                    status;

    /* Get the instance for this class.  */
    audio= (UX_HOST_CLASS_AUDIO *) command -> ux_host_class_command_instance;

    /* The audio is being shut down.  */
    audio -> ux_host_class_audio_state =  UX_HOST_CLASS_INSTANCE_SHUTDOWN;

    /* Protect thread reentry to this instance.  */
    status =  _ux_utility_semaphore_get(&audio -> ux_host_class_audio_semaphore, UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)

        /* Return error.  */
        return(status);
    
    /* We need to abort transactions on the iso out pipe.  */
    _ux_host_stack_endpoint_transfer_abort(audio -> ux_host_class_audio_isochronous_endpoint);

    /* The enumeration thread needs to sleep a while to allow the application or the class that may be using
       endpoints to exit properly.  */
    _ux_utility_thread_schedule_other(UX_THREAD_PRIORITY_ENUM); 

    /* Destroy the instance.  */
    _ux_host_stack_class_instance_destroy(audio -> ux_host_class_audio_class, (VOID *) audio);

    /* Destroy the semaphore.  */
    _ux_utility_semaphore_delete(&audio -> ux_host_class_audio_semaphore);

    /* Before we free the device resources, we need to inform the application
        that the device is removed.  */
    if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
    {
        
        /* Inform the application the device is removed.  */
        _ux_system_host -> ux_system_host_change_function(UX_DEVICE_REMOVAL, audio -> ux_host_class_audio_class, (VOID *) audio);
    }
    
    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_AUDIO_DEACTIVATE, audio, 0, 0, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

    /* If trace is enabled, register this object.  */
    UX_TRACE_OBJECT_UNREGISTER(audio);

    /* Free the audio instance memory.  */
    _ux_utility_memory_free(audio);
    
    /* Return successful completion.  */
    return(UX_SUCCESS);         
}

