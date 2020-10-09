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
/*    _ux_host_class_audio_descriptor_get                 PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function obtains the entire audio configuration descriptors.   */ 
/*    This is needed because the audio class has many class specific      */ 
/*    descriptors which describe the alternate settings that can be used. */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    audio                                 Pointer to audio class        */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_stack_transfer_request       Process transfer request      */ 
/*    _ux_utility_descriptor_parse          Parse descriptor              */ 
/*    _ux_utility_memory_allocate           Allocate memory block         */ 
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
UINT  _ux_host_class_audio_descriptor_get(UX_HOST_CLASS_AUDIO *audio)
{

UCHAR *                 descriptor;
UX_ENDPOINT             *control_endpoint;
UX_TRANSFER             *transfer_request;
UX_CONFIGURATION        configuration;
UINT                    status;
ULONG                   total_configuration_length;


    /* We need to get the default control endpoint transfer request pointer.  */
    control_endpoint =  &audio -> ux_host_class_audio_device -> ux_device_control_endpoint;
    transfer_request =  &control_endpoint -> ux_endpoint_transfer_request;

    /* Need to allocate memory for the descriptor.  */
    descriptor =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_CONFIGURATION_DESCRIPTOR_LENGTH);
    if (descriptor == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Create a transfer request for the GET_DESCRIPTOR request.  */
    transfer_request -> ux_transfer_request_data_pointer =      descriptor;
    transfer_request -> ux_transfer_request_requested_length =  UX_CONFIGURATION_DESCRIPTOR_LENGTH;
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             UX_CONFIGURATION_DESCRIPTOR_ITEM << 8;
    transfer_request -> ux_transfer_request_index =             0;

    /* Send request to HCD layer.  */
    status =  _ux_host_stack_transfer_request(transfer_request);

    /* Check for correct transfer and entire descriptor returned.  */
    if ((status == UX_SUCCESS) && (transfer_request -> ux_transfer_request_actual_length == UX_CONFIGURATION_DESCRIPTOR_LENGTH))
    {

        /* The descriptor is in a packed format, parse it locally.  */      
        _ux_utility_descriptor_parse(descriptor, _ux_system_configuration_descriptor_structure,
                                    UX_CONFIGURATION_DESCRIPTOR_ENTRIES, (UCHAR *) &configuration.ux_configuration_descriptor);
           
        /* Now we have the configuration descriptor which will tell us how many 
           bytes there are in the entire descriptor.  */
        total_configuration_length =  configuration.ux_configuration_descriptor.wTotalLength;

        /* Free the previous descriptor.  */
        _ux_utility_memory_free(descriptor);

        /* Allocate enough memory to read all descriptors attached
          to this configuration.  */
        descriptor =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, total_configuration_length);
        if (descriptor == UX_NULL)
            return(UX_MEMORY_INSUFFICIENT);

        /* Set the length we need to retrieve.  */
        transfer_request -> ux_transfer_request_requested_length =  total_configuration_length;

        /* And reprogram the descriptor buffer address.  */
        transfer_request -> ux_transfer_request_data_pointer =  descriptor;

        /* Send request to HCD layer.  */
        status =  _ux_host_stack_transfer_request(transfer_request);

        /* Check for correct transfer and entire descriptor returned.  */
        if ((status == UX_SUCCESS) && (transfer_request -> ux_transfer_request_actual_length == total_configuration_length))
        {           

            /* Save the address of the entire configuration descriptor of the audio device.  */            
            audio -> ux_host_class_audio_configuration_descriptor =  descriptor;

            /* Save the length of the entire descriptor too.  */            
            audio -> ux_host_class_audio_configuration_descriptor_length =  total_configuration_length;

            /* We do not free the resource for the descriptor until the device is
               plugged out.  */
            return(UX_SUCCESS);
        }
    }

    /* Free all used resources.  */
    _ux_utility_memory_free(descriptor);

    /* Return completion status.  */
    return(status);
}

