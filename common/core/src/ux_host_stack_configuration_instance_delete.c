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
/*    _ux_host_stack_configuration_instance_delete        PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function will delete a configuration instance. It does not     */ 
/*    delete configuration container but it deletes all the alternate     */ 
/*    current alternate settings for each interface it owns.              */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    configuration                         Pointer to configuration      */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_stack_interface_instance_delete  Delete interface instance */ 
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
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*                                                                        */
/**************************************************************************/
VOID  _ux_host_stack_configuration_instance_delete(UX_CONFIGURATION *configuration)
{

UX_INTERFACE    *interface;
ULONG           current_alternate_setting;
    
    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_STACK_CONFIGURATION_INSTANCE_DELETE, configuration, 0, 0, 0, UX_TRACE_HOST_STACK_EVENTS, 0, 0)

    /* Obtain the first interface for this configuration.  */
    interface =  configuration -> ux_configuration_first_interface;
    
    /* In order to keep the compiler happy, we reset the alternate setting.  */
    current_alternate_setting =  0;

    /* Each selected alternate setting for each interface must be deleted.  */
    while (interface != UX_NULL)
    {

        /* If this is the first alternate setting, the current alternate setting is maintained here.  */
        if (interface -> ux_interface_descriptor.bAlternateSetting == 0)
        {

            current_alternate_setting =  interface -> ux_interface_current_alternate_setting;
        }
        
        if (interface -> ux_interface_descriptor.bAlternateSetting == current_alternate_setting)
        {
            _ux_host_stack_interface_instance_delete(interface);
        }

        interface =  interface -> ux_interface_next_interface;
    }

    return; 
}

