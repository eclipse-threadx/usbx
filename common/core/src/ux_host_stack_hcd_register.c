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
/*    _ux_host_stack_hcd_register                         PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function registers a USB controller driver with the USBX stack */
/*    and invokes the HCD driver's initialization function.               */ 
/*                                                                        */
/*    Note: The C string of hcd_name must be NULL-terminated and the      */
/*    length of it (without the NULL-terminator itself) must be no larger */
/*    than UX_MAX_HCD_NAME_LENGTH.                                        */
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    hcd_name                              Name of HCD to register       */ 
/*    hcd_entry_function                    Entry function of HCD driver  */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_utility_string_length_check       Check and return C string     */
/*                                          length if no error            */
/*    _ux_utility_memory_copy               Copy name into HCD structure  */ 
/*    (hcd_init_function)                   Init function of HCD driver   */ 
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
UINT  _ux_host_stack_hcd_register(UCHAR *hcd_name,
                                    UINT (*hcd_init_function)(struct UX_HCD_STRUCT *), ULONG hcd_param1, ULONG hcd_param2)
{

UX_HCD      *hcd;
ULONG       hcd_index;
UINT        status;
UINT        hcd_name_length =  0;


    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_STACK_HCD_REGISTER, hcd_name, 0, 0, 0, UX_TRACE_HOST_STACK_EVENTS, 0, 0)

    /* Get the length of the class name (exclude null-terminator).  */
    status =  _ux_utility_string_length_check(hcd_name, &hcd_name_length, UX_MAX_HCD_NAME_LENGTH);
    if (status)
        return(status);

    /* We need to parse the controller driver table to find an empty spot.  */
    hcd =  _ux_system_host -> ux_system_host_hcd_array;
    for(hcd_index = 0; hcd_index < _ux_system_host -> ux_system_host_max_hcd; hcd_index++)
    {

        /* Is this slot available?  */
        if(hcd -> ux_hcd_status == UX_UNUSED)
        {

            /* Yes, setup the new HCD entry.  */

            /* Initialize the array of the new controller with its name (include null-terminator).  */
            _ux_utility_memory_copy(hcd -> ux_hcd_name, hcd_name, hcd_name_length + 1);

            /* Store the hardware resources of the controller */
            hcd -> ux_hcd_io =   hcd_param1;
            hcd -> ux_hcd_irq =  hcd_param2;
    
            /* This controller is now used */
            hcd -> ux_hcd_status =  UX_USED;

            /* And we have one new controller registered.  */
            _ux_system_host -> ux_system_host_registered_hcd++;

            /* We are now calling the HCD driver initialization.  */
            status =  hcd_init_function(hcd);

            /* Return the completion status to the caller.  */
            return(status);
        }

        /* Try the next HCD structure */
        hcd++;            
    }    

    /* We have exhausted the array of the HCDs, return an error.  */
    return(UX_MEMORY_INSUFFICIENT);
}

