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
/**   Device Stack                                                        */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                                 RELEASE      */
/*                                                                        */
/*    _ux_device_stack_class_unregister                     PORTABLE C    */
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function unregisters a slave class to the slave stack.         */
/*                                                                        */
/*    Note: The C string of class_name must be NULL-terminated and the    */
/*    length of it (without the NULL-terminator itself) must be no larger */
/*    than UX_MAX_CLASS_NAME_LENGTH.                                      */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    class_name                            Name of class                 */
/*    class_function_entry                  Class entry function          */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */ 
/*                                                                        */
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_utility_string_length_check       Check C string and return     */
/*                                          its length if null-terminated */
/*    _ux_utility_memory_compare            Memory compare                */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Application                                                         */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_stack_class_unregister(UCHAR *class_name,
                        UINT (*class_entry_function)(struct UX_SLAVE_CLASS_COMMAND_STRUCT *))
{

UX_SLAVE_CLASS              *class;
ULONG                       class_index;
UINT                        status;
UX_SLAVE_CLASS_COMMAND      command;
UINT                        class_name_length =  0;


    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_STACK_CLASS_UNREGISTER, class_name, 0, 0, 0, UX_TRACE_DEVICE_STACK_EVENTS, 0, 0)

    /* Get the length of the class name (exclude null-terminator).  */
    status =  _ux_utility_string_length_check(class_name, &class_name_length, UX_MAX_CLASS_NAME_LENGTH);
    if (status)
        return(status);

    /* We need to parse the class table to find the right class.  */
    class =  _ux_system_slave -> ux_system_slave_class_array;
    for (class_index = 0; class_index < _ux_system_slave -> ux_system_slave_max_class; class_index++)
    {

        /* Check if this class is the right one.  */
        if (class -> ux_slave_class_status == UX_USED)
        {

            /* We have found a used container with a  class. Compare the name (include null-terminator).  */
            if (_ux_utility_memory_compare(class -> ux_slave_class_name, class_name, class_name_length + 1) == UX_SUCCESS)
            {
                        
                /* Build all the fields of the Class Command to uninitialize the class.  */
                command.ux_slave_class_command_request    =  UX_SLAVE_CLASS_COMMAND_UNINITIALIZE;
                command.ux_slave_class_command_class_ptr  =  class;

                /* Call the class uninitialization routine.  */
                status = class_entry_function(&command);
            
                /* Check the status.  */
                if (status != UX_SUCCESS)
                    return(status);
            
                /* Make this class unused now.  */
                class -> ux_slave_class_status = UX_UNUSED;
            
                /* Erase the instance of the class.  */
                class -> ux_slave_class_instance = UX_NULL;

                /* Return successful completion.  */
                return(UX_SUCCESS);
            }
        }

        /* Move to the next class.  */
        class++;
    }    

    /* No class match.  */
    return(UX_NO_CLASS_MATCH);
}

