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
/*    _ux_host_class_audio_device_controls_list_get       PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function obtains the controls for the audio device.            */ 
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
/*    _ux_utility_descriptor_parse          Parse descriptor              */ 
/*    _ux_utility_long_get                  Get 32-bit word               */ 
/*    _ux_utility_short_get                 Get 16-bit word               */ 
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
UINT  _ux_host_class_audio_device_controls_list_get(UX_HOST_CLASS_AUDIO *audio)
{

UCHAR *                                         descriptor;
UX_INTERFACE_DESCRIPTOR                         interface_descriptor;
UX_HOST_CLASS_AUDIO_INPUT_TERMINAL_DESCRIPTOR   input_interface_descriptor;
UX_HOST_CLASS_AUDIO_FEATURE_UNIT_DESCRIPTOR     feature_unit_interface_descriptor;
ULONG                                           total_descriptor_length;
ULONG                                           descriptor_length;
ULONG                                           descriptor_type;
ULONG                                           descriptor_subtype;
ULONG                                           descriptor_found;
ULONG                                           control_size_bytes;
UCHAR *                                         control_bit_map_address;
ULONG                                           channel_number;
    

    /* Get the descriptor to the entire configuration */
    descriptor =               audio -> ux_host_class_audio_configuration_descriptor;
    total_descriptor_length =  audio -> ux_host_class_audio_configuration_descriptor_length;
    
    /* Default is Interface descriptor not yet found.  */    
    descriptor_found =  UX_FALSE;
    
    /* Scan the descriptor for the Audio Streaming interface.  */
    while (total_descriptor_length)
    {

        /* Gather the length, type and subtype of the descriptor.  */
        descriptor_length =   *descriptor;
        descriptor_type =     *(descriptor + 1);
        descriptor_subtype =  *(descriptor + 2);

        /* Make sure this descriptor has at least the minimum length.  */
        if (descriptor_length < 3)
        {

            /* Error trap. */
            _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_DESCRIPTOR_CORRUPTED);

            /* If trace is enabled, insert this event into the trace buffer.  */
            UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_DESCRIPTOR_CORRUPTED, descriptor, 0, 0, UX_TRACE_ERRORS, 0, 0)

            return(UX_DESCRIPTOR_CORRUPTED);
        }

        /* Process relative to descriptor type.  */
        switch(descriptor_type)
        {


        case UX_INTERFACE_DESCRIPTOR_ITEM:

            /* Parse the interface descriptor and make it machine independent.  */
            _ux_utility_descriptor_parse(descriptor, _ux_system_interface_descriptor_structure,
                                            UX_INTERFACE_DESCRIPTOR_ENTRIES, (UCHAR *) &interface_descriptor);

            /* Ensure we have the correct interface for Audio streaming.  */
            if ((interface_descriptor.bInterfaceClass == UX_HOST_CLASS_AUDIO_CLASS) &&
                (interface_descriptor.bInterfaceSubClass == UX_HOST_CLASS_AUDIO_SUBCLASS_CONTROL))
             
                /* Mark we have found it.  */
                descriptor_found =  UX_TRUE;
            else

                descriptor_found =  UX_FALSE;
            break;
                

        case UX_HOST_CLASS_AUDIO_CS_INTERFACE:

            /* First make sure we have found the correct generic interface descriptor.  */
            if (descriptor_found == UX_TRUE)
            {

                /* Check the sub type.  */
                switch (descriptor_subtype)
                {

                case UX_HOST_CLASS_AUDIO_CS_INPUT_TERMINAL:
                                        
                    /* Make the descriptor machine independent.  */
                    _ux_utility_descriptor_parse(descriptor, _ux_system_class_audio_input_terminal_descriptor_structure,
                                            UX_HOST_CLASS_AUDIO_INPUT_TERMINAL_DESCRIPTOR_ENTRIES, (UCHAR *) &input_interface_descriptor);
                                                        
                    /* Get the number of channels and check for maximum allowed.  */
                    if (input_interface_descriptor.bNrChannels > UX_HOST_CLASS_AUDIO_MAX_CHANNEL)
                        audio -> ux_host_class_audio_channels =  UX_HOST_CLASS_AUDIO_MAX_CHANNEL;
                    else                                    
                        audio -> ux_host_class_audio_channels =  input_interface_descriptor.bNrChannels;
                    break;

                
                case UX_HOST_CLASS_AUDIO_CS_FEATURE_UNIT:

                    /* Make the descriptor machine independent.  */
                    _ux_utility_descriptor_parse(descriptor, _ux_system_class_audio_feature_unit_descriptor_structure,
                                            UX_HOST_CLASS_AUDIO_FEATURE_UNIT_DESCRIPTOR_ENTRIES, (UCHAR *) &feature_unit_interface_descriptor);
                                                        
                    /* Store the Feature Unit ID, used to set/get the audio controls.  */
                    audio -> ux_host_class_audio_feature_unit_id =  feature_unit_interface_descriptor.bUnitID;

                    /* Get the size of each control in bytes.  */
                    control_size_bytes =  feature_unit_interface_descriptor.bControlSize;
                            
                    /* Get the address of the first control bit map.  */
                    control_bit_map_address =  (UCHAR *) &feature_unit_interface_descriptor.bmaControls;
                            
                    /* Walk all the controls and retrieve them one by one.  */
                    for (channel_number = 0; channel_number < audio -> ux_host_class_audio_channels; channel_number++)
                    {

                        /* Each control has the same size.  */
                        switch (control_size_bytes)
                        {
                            
                        case 1:

                            /* Byte aligned control.  */
                            audio -> ux_host_class_audio_channel_control[channel_number] =  (ULONG) *control_bit_map_address;
                            break;

                           
                        case 2:

                            /* Word aligned control.  */
                            audio -> ux_host_class_audio_channel_control[channel_number] =  (ULONG) _ux_utility_short_get(control_bit_map_address);
                            break;
                                        

                        case 4:

                            /* Long aligned control.  */
                            audio -> ux_host_class_audio_channel_control[channel_number] =  _ux_utility_long_get(control_bit_map_address);
                            break;

                       
                        default:

                            /* Not sure what we should do here, we ignore the control.  */
                            break;
                        }
                                
                        /* Point to the next control.  */
                        control_bit_map_address +=  control_size_bytes;
                    }

                    /* Return successful completion.  */
                    return(UX_SUCCESS);
                }
            }
            break;
        }       

        /* Verify if the descriptor is still valid.  */
        if (descriptor_length > total_descriptor_length)
        {

            /* Error trap. */
            _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_DESCRIPTOR_CORRUPTED);

            /* If trace is enabled, insert this event into the trace buffer.  */
            UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_DESCRIPTOR_CORRUPTED, descriptor, 0, 0, UX_TRACE_ERRORS, 0, 0)

            return(UX_DESCRIPTOR_CORRUPTED);
        }            

        /* Jump to the next descriptor if we have not reached the end.  */
        descriptor +=  descriptor_length;

        /* And adjust the length left to parse in the descriptor.  */
        total_descriptor_length -=  descriptor_length;
    }

    /* Error trap. */
    _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_AUDIO_WRONG_TYPE);

    /* We get here when either the report descriptor has a problem or we could
       not find the right audio device.  */
    return(UX_HOST_CLASS_AUDIO_WRONG_TYPE);
}

