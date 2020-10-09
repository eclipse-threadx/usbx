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
/**   Acm Cdc Class                                                       */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_cdc_acm.h"
#include "ux_host_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_cdc_acm_entry                        PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function is the entry point of the cdc_acm class. It will be   */ 
/*    called by the USBX stack enumeration module when there is a new     */ 
/*    cdc_acm on the bus or when the USB cdc_acm is removed.              */ 
/*                                                                        */ 
/*    A CDC class can have multiple interfaces, one for Control and one   */ 
/*    for Data. Here we filter for the Communication Class with ACM       */ 
/*    subclass and the Communication Data Class.                          */ 
/*                                                                        */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    command                                  Acm Cdc class command      */
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_class_cdc_acm_activate       Activate cdc_acm class        */ 
/*    _ux_host_class_cdc_acm_deactivate     Deactivate cdc_acm class      */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Acm Cdc Class                                                       */ 
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
UINT  _ux_host_class_cdc_acm_entry(UX_HOST_CLASS_COMMAND *command)
{

UINT    status;


    /* The command request will tell us we need to do here, either a enumeration
       query, an activation or a deactivation.  */
    switch (command -> ux_host_class_command_request)
    {

    case UX_HOST_CLASS_COMMAND_QUERY:

        /* The query command is used to let the stack enumeration process know if we want to own
           this device or not.  */
        if((command -> ux_host_class_command_usage == UX_HOST_CLASS_COMMAND_USAGE_CSP) &&
                             ((command -> ux_host_class_command_class == UX_HOST_CLASS_CDC_DATA_CLASS) ||
                             ((command -> ux_host_class_command_class == UX_HOST_CLASS_CDC_CONTROL_CLASS) &&
                             (command -> ux_host_class_command_subclass == UX_HOST_CLASS_CDC_ACM_SUBCLASS)) ||
                             ((command -> ux_host_class_command_class == UX_HOST_CLASS_CDC_CONTROL_CLASS) &&
                             (command -> ux_host_class_command_subclass == UX_HOST_CLASS_CDC_DLC_SUBCLASS))))
        {
            /* Check for IAD presence.  */
            if ((command -> ux_host_class_command_iad_class == 0) && (command -> ux_host_class_command_iad_subclass == 0))
            
                /* No IAD, we accept this class.  */
                return(UX_SUCCESS);            
            
            else
            {

                if ((command -> ux_host_class_command_iad_class == UX_HOST_CLASS_CDC_CONTROL_CLASS) &&
                        (command -> ux_host_class_command_iad_subclass == UX_HOST_CLASS_CDC_ACM_SUBCLASS))
            
                    /* There is an IAD and this is for CDC-ACM.  */
                    return(UX_SUCCESS);                        

                else
                
                    /* The IAD does not match with CDC-ACM.  */
                    return(UX_NO_CLASS_MATCH);                        
            }
        }

        else            

            /* No match.  */
            return(UX_NO_CLASS_MATCH);                        
                
    case UX_HOST_CLASS_COMMAND_ACTIVATE:

        /* The activate command is used when the device inserted has found a parent and
           is ready to complete the enumeration.  */
        status =  _ux_host_class_cdc_acm_activate(command);
        return(status);

    case UX_HOST_CLASS_COMMAND_DEACTIVATE:

        /* The deactivate command is used when the device has been extracted either      
           directly or when its parents has been extracted.  */
        status =  _ux_host_class_cdc_acm_deactivate(command);
        return(status);

    default: 
            
        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_FUNCTION_NOT_SUPPORTED);
    }   
}

