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
/*    _ux_host_class_audio_activate                       PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*     This function activates the audio class. It may be called twice by */
/*     the same device if there is a audio control interface to this      */ 
/*     device.                                                            */ 
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
/*    _ux_host_class_audio_configure        Configure the audio class     */ 
/*    _ux_host_class_audio_descriptor_get   Get audio descriptor          */ 
/*    _ux_host_class_audio_device_controls_list_get Get controls list     */ 
/*    _ux_host_class_audio_device_type_get  Get device type               */ 
/*    _ux_host_class_audio_streaming_terminal_get Get streaming terminal  */ 
/*    _ux_host_stack_class_instance_create  Create class instance         */ 
/*    _ux_host_stack_class_instance_destroy Destroy class instance        */ 
/*    _ux_utility_memory_allocate           Allocate a memory block       */ 
/*    _ux_utility_semaphore_create          Create protection semaphore   */ 
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
UINT  _ux_host_class_audio_activate(UX_HOST_CLASS_COMMAND *command)
{

UX_INTERFACE            *interface;
UX_HOST_CLASS_AUDIO     *audio;
UINT                    status;


    /* The audio is always activated by the interface descriptor and not the
       device descriptor.  */
    interface =  (UX_INTERFACE *) command -> ux_host_class_command_container;

    /* Check the subclass of the new device. If it is a Audio Control Interface,
       we don't need to create an instance of this function. When we get the streaming interface,
       we will search the audio control interface for the device.  */
    if (interface -> ux_interface_descriptor.bInterfaceSubClass == UX_HOST_CLASS_AUDIO_SUBCLASS_CONTROL)
        return(UX_SUCCESS);
    
    /* Obtain memory for this class instance.  */
    audio =  (UX_HOST_CLASS_AUDIO *) _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HOST_CLASS_AUDIO));
    if (audio == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Store the class container into this instance.  */
    audio -> ux_host_class_audio_class =  command -> ux_host_class_command_class_ptr;

    /* Store the interface container into the audio class instance.  */
    audio -> ux_host_class_audio_streaming_interface =  interface;

    /* Store the device container into the audio class instance.  */
    audio -> ux_host_class_audio_device =  interface -> ux_interface_configuration -> ux_configuration_device;

    /* This instance of the device must also be stored in the interface container.  */
    interface -> ux_interface_class_instance =  (VOID *) audio;

    /* This instance of the device must also be stored in the interface container.  */
    interface -> ux_interface_class_instance =  (VOID *) audio;

    /* Create this class instance.  */
    _ux_host_stack_class_instance_create(audio -> ux_host_class_audio_class, (VOID *) audio);
        
    /* Configure the audio.  */
    status =  _ux_host_class_audio_configure(audio);     
    if (status != UX_SUCCESS)
    {

        /* Error, destroy the class instance.  */
        _ux_host_stack_class_instance_destroy(audio -> ux_host_class_audio_class, (VOID *) audio);

        /* Return the error.  */
        return(status);
    }

    /* Get the audio descriptor (all the class specific stuff) and memorize them
       as we will need these descriptors to change settings.  */
    status =  _ux_host_class_audio_descriptor_get(audio);        
    if (status != UX_SUCCESS)
    {

        /* Error, destroy the class instance.  */
        _ux_host_stack_class_instance_destroy(audio -> ux_host_class_audio_class, (VOID *) audio);

        /* Return the error.  */
        return(status);
    }

    /* Locate the audio device streaming terminal.  */
    status =  _ux_host_class_audio_streaming_terminal_get(audio);        
    if (status != UX_SUCCESS)
    {

        /* Error, destroy the class instance.  */
        _ux_host_stack_class_instance_destroy(audio -> ux_host_class_audio_class, (VOID *) audio);

        /* Return the error.  */
        return(status);
    }

    /* Get the audio device type. Here we only support input and output devices.  */
    status =  _ux_host_class_audio_device_type_get(audio);       
    if (status != UX_SUCCESS)
    {

        /* Error, destroy the class instance.  */
        _ux_host_stack_class_instance_destroy(audio -> ux_host_class_audio_class, (VOID *) audio);

        /* Return the error.  */
        return(status);
    }

    /* Get the audio device controls.  */
    status =  _ux_host_class_audio_device_controls_list_get(audio);      
    if (status != UX_SUCCESS)
    {

        /* Error, destroy the class instance.  */
        _ux_host_stack_class_instance_destroy(audio -> ux_host_class_audio_class, (VOID *) audio);

        /* Return the error.  */
        return(status);
    }

    /* Create the semaphore to protect multiple threads from accessing the same
       audio instance.  */
    status =  _ux_utility_semaphore_create(&audio -> ux_host_class_audio_semaphore, "ux_hot_class_audio_semaphore", 1);
    if (status != UX_SUCCESS)
        return(UX_SEMAPHORE_ERROR);

    /* Mark the audio as live now.  */
    audio -> ux_host_class_audio_state =  UX_HOST_CLASS_INSTANCE_LIVE;

    /* If all is fine and the device is mounted, we may need to inform the application
       if a function has been programmed in the system structure.  */
    if ((status == UX_SUCCESS) && (_ux_system_host -> ux_system_host_change_function != UX_NULL))
    {
        
        /* Call system change function.  */
        _ux_system_host ->  ux_system_host_change_function(UX_DEVICE_INSERTION, audio -> ux_host_class_audio_class, (VOID *) audio);
    }

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_AUDIO_ACTIVATE, audio, 0, 0, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

    /* If trace is enabled, register this object.  */
    UX_TRACE_OBJECT_REGISTER(UX_TRACE_HOST_OBJECT_TYPE_INTERFACE, audio, 0, 0, 0)

    /* Return completion status.  */
    return(status);    
}

