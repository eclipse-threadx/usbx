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
/**   Video Class                                                         */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_video.h"
#include "ux_host_stack.h"

UCHAR _ux_system_class_video_interface_descriptor_structure[] =             {1,1,1,1,1,1,1,1};
UCHAR _ux_system_class_video_input_terminal_descriptor_structure[] =        {1,1,1,1,2,1,1};
UCHAR _ux_system_class_video_input_header_descriptor_structure[] =          {1,1,1,1,2,1,1,1,1,1,1,1};
UCHAR _ux_system_class_video_processing_unit_descriptor_structure[] =       {1,1,1,1,1,2,1,1};
UCHAR _ux_system_class_video_streaming_interface_descriptor_structure[] =   {1,1,1,1,1,1};
UCHAR _ux_system_class_video_streaming_endpoint_descriptor_structure[] =    {1,1,1,1,1,1};
UCHAR _ux_system_class_video_frame_descriptor_structure[] =                 {1,1,1,1,1,2,2,4,4,4,4,1};

UCHAR _ux_system_host_class_video_name[] =                                  "ux_host_class_video";

/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_video_activate                       PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*     This function activates the video class. It may be called twice by */
/*     the same device if there is a video control interface to this      */ 
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
/*    _ux_host_class_video_configure        Configure the video class     */ 
/*    _ux_host_class_video_descriptor_get   Get video descriptor          */ 
/*    _ux_host_class_video_input_terminal_get                             */
/*                                          Get input terminal            */
/*    _ux_host_class_video_input_format_get Get input format              */
/*    _ux_host_class_video_control_list_get Get controls                  */
/*    _ux_host_stack_class_instance_create  Create class instance         */ 
/*    _ux_host_stack_class_instance_destroy Destroy class instance        */ 
/*    _ux_utility_memory_allocate           Allocate a memory block       */ 
/*    _ux_utility_memory_free               Free a memory block           */ 
/*    _ux_utility_semaphore_create          Create protection semaphore   */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Video Class                                                         */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_video_activate(UX_HOST_CLASS_COMMAND *command)
{

UX_INTERFACE            *interface;
UX_HOST_CLASS_VIDEO     *video;
UINT                    status;


    /* The video is always activated by the interface descriptor and not the
       device descriptor.  */
    interface =  (UX_INTERFACE *) command -> ux_host_class_command_container;

    /* Check the subclass of the new device. If it is a Video Control Interface,
       we don't need to create an instance of this function. When we get the streaming interface,
       we will search the video control interface for the device.  */
    if (interface -> ux_interface_descriptor.bInterfaceSubClass == UX_HOST_CLASS_VIDEO_SUBCLASS_CONTROL)
        return(UX_SUCCESS);
    
    /* Obtain memory for this class instance.  */
    video =  (UX_HOST_CLASS_VIDEO *) _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HOST_CLASS_VIDEO));
    if (video == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Store the class container into this instance.  */
    video -> ux_host_class_video_class =  command -> ux_host_class_command_class_ptr;

    /* Store the interface container into the video class instance.  */
    video -> ux_host_class_video_streaming_interface =  interface;

    /* Store the device container into the video class instance.  */
    video -> ux_host_class_video_device =  interface -> ux_interface_configuration -> ux_configuration_device;

    /* This instance of the device must also be stored in the interface container.  */
    interface -> ux_interface_class_instance =  (VOID *) video;

    /* Create this class instance.  */
    status =  _ux_host_stack_class_instance_create(video -> ux_host_class_video_class, (VOID *) video);
        
    /* Configure the video.  */
    status =  _ux_host_class_video_configure(video);     

    /* Get the video descriptor (all the class specific stuff) and memorize them
       as we will need these descriptors to change settings.  */
    if (status == UX_SUCCESS)
        status =  _ux_host_class_video_descriptor_get(video);

    /* Locate the video device streaming terminal.  */
    if (status == UX_SUCCESS)
        status =  _ux_host_class_video_input_terminal_get(video);

    /* In the input terminal streaming interface get the number of formats supported.  */
    if (status == UX_SUCCESS)
        status =  _ux_host_class_video_input_format_get(video);

    /* Get video controls.  */
    if (status == UX_SUCCESS)
        status =  _ux_host_class_video_control_list_get(video);

    /* Create the semaphore to protect multiple threads from accessing the same
       video instance.  */
    if (status == UX_SUCCESS)
    {
        status =  _ux_utility_semaphore_create(&video -> ux_host_class_video_semaphore, "ux_video_semaphore", 1);
        if (status != UX_SUCCESS)
            status = UX_SEMAPHORE_ERROR;
    }

    if (status == UX_SUCCESS)
    {

        /* Mark the video as live now.  */
        video -> ux_host_class_video_state =  UX_HOST_CLASS_INSTANCE_LIVE;

        /* If all is fine and the device is mounted, we may need to inform the application
        if a function has been programmed in the system structure.  */
        if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
        {
            
            /* Call system change function.  */
            _ux_system_host ->  ux_system_host_change_function(UX_DEVICE_INSERTION, video -> ux_host_class_video_class, (VOID *) video);
        }

        /* If trace is enabled, insert this event into the trace buffer.  */
        //UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_VIDEO_ACTIVATE, video, 0, 0, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

        /* If trace is enabled, register this object.  */
        UX_TRACE_OBJECT_REGISTER(UX_TRACE_HOST_OBJECT_TYPE_INTERFACE, video, 0, 0, 0)

        /* Return success.  */
        return(UX_SUCCESS);
    }

    /* There was error, free resources.  */

    /* The last resource, video -> ux_host_class_video_semaphore is not created or created error,
       no need to free.  */

    /* Destroy the class instance.  */
    _ux_host_stack_class_instance_destroy(video -> ux_host_class_video_class, (VOID *) video);

    /* This instance of the device must also be cleared in the interface container.  */
    interface -> ux_interface_class_instance = UX_NULL;

    /* Free instance memory.  */
    _ux_utility_memory_free(video);

    /* Return completion status.  */
    return(status);    
}

