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
/*    _ux_host_class_audio_streaming_sampling_set         PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function selects the right alternate setting for the audio     */
/*    streaming interface based on the sampling values specified by the   */ 
/*    user.                                                               */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    audio                                 Pointer to audio class        */ 
/*    audio_sampling                        Pointer to audio sampling     */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_class_audio_alternate_setting_locate                       */
/*                                          Locate alternate setting      */ 
/*    _ux_host_stack_class_instance_verify  Verify instance is valid      */ 
/*    _ux_host_stack_interface_endpoint_get Get interface endpoint        */ 
/*    _ux_host_stack_interface_setting_select Select interface            */ 
/*    _ux_utility_semaphore_get             Get semaphore                 */ 
/*    _ux_utility_semaphore_put             Put semaphore                 */ 
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
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_audio_streaming_sampling_set(UX_HOST_CLASS_AUDIO *audio, UX_HOST_CLASS_AUDIO_SAMPLING *audio_sampling)
{

UINT                    status;
UINT                    alternate_setting;
ULONG                   endpoint_index;
UX_CONFIGURATION        *configuration;
UX_INTERFACE            *interface;
UX_ENDPOINT             *endpoint;
UINT                    streaming_interface;

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_AUDIO_STREAMING_SAMPLING_SET, audio, audio_sampling, 0, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

    /* Ensure the instance is valid.  */
    if (_ux_host_stack_class_instance_verify(_ux_system_host_class_audio_name, (VOID *) audio) != UX_SUCCESS)
        return(UX_HOST_CLASS_INSTANCE_UNKNOWN);

    /* Protect thread reentry to this instance.  */
    status =  _ux_utility_semaphore_get(&audio -> ux_host_class_audio_semaphore, UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
        return(UX_HOST_CLASS_INSTANCE_UNKNOWN);

    /* Find the correct alternate setting for the sampling desired.  */
    status =  _ux_host_class_audio_alternate_setting_locate(audio, audio_sampling, &alternate_setting);

    /* Did we find the alternate setting?  */
    if (status != UX_SUCCESS)
    {
 
        /* Unprotect thread reentry to this instance.  */
        status =  _ux_utility_semaphore_put(&audio -> ux_host_class_audio_semaphore);
        return(status);
    }

    /* We found the alternate setting for the sampling values demanded, now we need 
        to search its container.  */
    configuration =        audio -> ux_host_class_audio_streaming_interface -> ux_interface_configuration;
    interface =            configuration -> ux_configuration_first_interface;  
    streaming_interface =  audio -> ux_host_class_audio_streaming_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Scan all interfaces.  */
    while (interface != UX_NULL)     
    {

        /* We search for both the right interface and alternate setting.  */
        if ((interface -> ux_interface_descriptor.bInterfaceNumber == streaming_interface) &&
            (interface -> ux_interface_descriptor.bAlternateSetting == alternate_setting))
        {
            
            /* We have found the right interface/alternate setting combination 
               The stack will select it for us.  */
            status =  _ux_host_stack_interface_setting_select(interface);
            
            /* If the alternate setting for the streaming interface could be selected, we memorize it.  */
            if (status == UX_SUCCESS)
            {

                /* Memorize the interface.  */
                audio -> ux_host_class_audio_streaming_interface =  interface;

                /* We need to research the isoch endpoint now.  */
                for (endpoint_index = 0; endpoint_index < interface -> ux_interface_descriptor.bNumEndpoints; endpoint_index++)
                {                        

                    /* Get the list of endpoints one by one.  */
                    status =  _ux_host_stack_interface_endpoint_get(audio -> ux_host_class_audio_streaming_interface,
                                                                                    endpoint_index, &endpoint);

                    /* Check completion status.  */
                    if (status == UX_SUCCESS)
                    {

                        /* Check if endpoint is ISOCH, regardless of the direction.  */
                        if ((endpoint -> ux_endpoint_descriptor.bmAttributes & UX_MASK_ENDPOINT_TYPE) == UX_ISOCHRONOUS_ENDPOINT)
                        {

                            /* We have found the isoch endpoint, save it.  */
                            audio -> ux_host_class_audio_isochronous_endpoint =  endpoint;

                            /* Unprotect thread reentry to this instance.  */
                            status =  _ux_utility_semaphore_put(&audio -> ux_host_class_audio_semaphore);

                            /* Return successful completion.  */
                            return(UX_SUCCESS);             
                        }
                    }                
                }            
            }
        }

        /* Move to next interface.  */
        interface =  interface -> ux_interface_next_interface;
    }
        
    /* Unprotect thread reentry to this instance.  */
    status =  _ux_utility_semaphore_put(&audio -> ux_host_class_audio_semaphore);

    /* Error trap. */
    _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_NO_ALTERNATE_SETTING);

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_NO_ALTERNATE_SETTING, audio, 0, 0, UX_TRACE_ERRORS, 0, 0)

    /* We get here if we could not get the right alternate setting.  */
    return(UX_NO_ALTERNATE_SETTING);
}

