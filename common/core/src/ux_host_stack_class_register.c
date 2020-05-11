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
/**   Host Stack                                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_stack_class_register                       PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function registers a USB class to the USB stack. The Class     */ 
/*    must specify an entry point for the USB stack to send commands      */ 
/*    such as:                                                            */
/*                                                                        */
/*                  UX_HOST_CLASS_COMMAND_QUERY                           */
/*                  UX_HOST_CLASS_COMMAND_ACTIVATE                        */
/*                  UX_HOST_CLASS_COMMAND_DESTROY                         */ 
/*                                                                        */
/*    Note: The C string of class_name must be NULL-terminated and the    */
/*    length of it (without the NULL-terminator itself) must be no larger */
/*    than UX_MAX_CLASS_NAME_LENGTH.                                      */
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    class_name                            Name of class                 */ 
/*    class_entry_function                  Entry function of the class   */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_utility_string_length_check       Check C string and return     */
/*                                          length if null-terminated     */
/*    _ux_utility_memory_copy               Copy memory block             */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Application                                                         */ 
/*    USBX Components                                                     */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_stack_class_register(UCHAR *class_name,
                        UINT (*class_entry_function)(struct UX_HOST_CLASS_COMMAND_STRUCT *))
{

UX_HOST_CLASS       *class;
ULONG               class_index;
UINT                status;
UINT                class_name_length =  0;


    /* Get the length of the class name (exclude null-terminator).  */
    status =  _ux_utility_string_length_check(class_name, &class_name_length, UX_MAX_CLASS_NAME_LENGTH);
    if (status)
        return(status);

    /* We need to parse the class table to find an empty spot.  */
    class =  _ux_system_host -> ux_system_host_class_array;
    for (class_index = 0; class_index < _ux_system_host -> ux_system_host_max_class; class_index++)
    {

        /* Check if this class is already used.  */
        if (class -> ux_host_class_status == UX_UNUSED)
        {

            /* We have found a free container for the class. Copy the name (with null-terminator).  */
            _ux_utility_memory_copy(class -> ux_host_class_name, class_name, class_name_length + 1);
            
            /* Memorize the entry function of this class.  */
            class -> ux_host_class_entry_function =  class_entry_function;

            /* Mark it as used.  */
            class -> ux_host_class_status =  UX_USED;

            /* Return successful completion.  */
            return(UX_SUCCESS);
        }

        /* Do a sanity check to make sure the class is not already installed by
           mistake. To verify this, we simple check for the class entry point.  */
        else
        {

            /* Check for an already installed class entry function.  */
            if(class -> ux_host_class_entry_function == class_entry_function)
            {

                /* Error trap. */
                _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_INIT, UX_HOST_CLASS_ALREADY_INSTALLED);

                /* If trace is enabled, insert this event into the trace buffer.  */
                UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_HOST_CLASS_ALREADY_INSTALLED, class_name, 0, 0, UX_TRACE_ERRORS, 0, 0)

                /* Yes, return an error.  */
                return(UX_HOST_CLASS_ALREADY_INSTALLED);
            }
        }

        /* Move to the next class.  */
        class++;
    }    
    
    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_MEMORY_ARRAY_FULL, class_name, 0, 0, UX_TRACE_ERRORS, 0, 0)

    /* Error trap. */
    _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_INIT, UX_MEMORY_ARRAY_FULL);

    /* No more entries in the class table.  */
    return(UX_MEMORY_ARRAY_FULL);
}

