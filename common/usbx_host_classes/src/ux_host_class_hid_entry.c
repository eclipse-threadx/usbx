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
/**   HID Class                                                           */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_hid.h"
#include "ux_host_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_hid_entry                            PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function is the entry point of the HID class. It will be       */ 
/*    called by the USB stack enumeration module when there is a new      */ 
/*    device on the bus or when there is a device extraction.             */ 
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
/*    _ux_host_class_hid_activate           Activate HID class            */ 
/*    _ux_host_class_hid_deactivate         Deactivate HID class          */ 
/*    _ux_utility_memory_free               Free memory                   */
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Application                                                         */ 
/*    HID Class                                                           */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            added destroy command,      */
/*                                            resulting in version 6.1    */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_hid_entry(UX_HOST_CLASS_COMMAND *command)
{

UINT                                status;
INT                                 scan_index;
UX_HOST_CLASS_HID_CLIENT            *client;
UX_HOST_CLASS_HID_CLIENT_COMMAND    client_command;


    /* The command request will tell us we need to do here, either a enumeration
       query, an activation or a deactivation.  */
    switch (command -> ux_host_class_command_request)
    {

    case UX_HOST_CLASS_COMMAND_QUERY:

        /* The query command is used to let the stack enumeration process know if we want to own
           this device or not.  */
        if ((command -> ux_host_class_command_usage == UX_HOST_CLASS_COMMAND_USAGE_CSP) &&
            (command -> ux_host_class_command_class == UX_HOST_CLASS_HID_CLASS))
            return(UX_SUCCESS);                        
        else            
            return(UX_NO_CLASS_MATCH);


    case UX_HOST_CLASS_COMMAND_ACTIVATE:

        /* The activate command is used when the device inserted has found a parent and
           is ready to complete the enumeration.   */

        status =  _ux_host_class_hid_activate(command);
        return(status);


    case UX_HOST_CLASS_COMMAND_DEACTIVATE:

        /* The deactivate command is used when the device has been extracted either      
           directly or when its parents has been extracted.  */

        status =  _ux_host_class_hid_deactivate(command);
        return(status);

    case UX_HOST_CLASS_COMMAND_DESTROY:

        /* The destroy command is used when the class is unregistered.  */

        /* Free allocated resources for clients.  */
        if (command -> ux_host_class_command_class_ptr -> ux_host_class_client != UX_NULL)
        {

            /* Get client.  */
            client = command -> ux_host_class_command_class_ptr -> ux_host_class_client;

            /* Inform clients for destroy.  */
            for (scan_index = 0; scan_index < UX_HOST_CLASS_HID_MAX_CLIENTS; scan_index ++)
            {

                /* Inform client for destroy.  */
                client_command.ux_host_class_hid_client_command_request = UX_HOST_CLASS_COMMAND_DESTROY;
                client_command.ux_host_class_hid_client_command_container = (VOID *)command -> ux_host_class_command_class_ptr;
                client -> ux_host_class_hid_client_handler(&client_command);
            }

            /* Free clients memory.  */
            _ux_utility_memory_free(command -> ux_host_class_command_class_ptr -> ux_host_class_client);
            command -> ux_host_class_command_class_ptr -> ux_host_class_client = UX_NULL;
        }
        return(UX_SUCCESS);

    default: 

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        /* Return error status.  */
        return(UX_FUNCTION_NOT_SUPPORTED);
    }   
}

