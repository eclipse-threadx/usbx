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
/*    _ux_host_stack_class_call                           PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function will call all the registered classes to the USBX      */
/*    stack. Each class will have the possibility to own the device or    */
/*    one of the interfaces of a device.                                  */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    class_command                         Class command structure       */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Number of owners                                                    */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    (ux_host_class_entry_function)        Class entry function          */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    USBX Components                                                     */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/
UX_HOST_CLASS  *_ux_host_stack_class_call(UX_HOST_CLASS_COMMAND *class_command)
{

UX_HOST_CLASS        *class;
ULONG           class_index;
UINT            status;


    /* Start from the 1st registered classes with USBX.  */
    class =  _ux_system_host -> ux_system_host_class_array;

    /* Parse all the class drivers.  */
    for (class_index = 0; class_index < _ux_system_host -> ux_system_host_max_class; class_index++)
    {

        /* Check if this class driver is used.  */
        if (class -> ux_host_class_status == UX_USED)
        {

            /* We have found a potential candidate. Call this registered class entry function.  */
            status = class -> ux_host_class_entry_function(class_command);

            /* The status tells us if the registered class wants to own this class.  */
            if (status == UX_SUCCESS)
            {

                /* Yes, return this class pointer.  */
                return(class); 
            }
        }    

        /* Move to the next registered class. */
        class++;
    }       

    /* There is no driver who want to own this class!  */
    return(UX_NULL);
}

