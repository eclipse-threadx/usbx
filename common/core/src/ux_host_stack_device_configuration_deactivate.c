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
/*    _ux_host_stack_device_configuration_deactivate      PORTABLE C      */
/*                                                           6.1.7        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function deactivate current configuration for a device.        */
/*    When this configuration is deactivated, all the device interfaces   */
/*    are deactivated on the device.                                      */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_utility_semaphore_get             Get semaphore                 */
/*    _ux_utility_semaphore_put             Put semaphore                 */
/*    (ux_host_class_entry_function)        Class entry function          */
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
/*  02-02-2021     Chaoqiong Xiao           Initial Version 6.1.4         */
/*  06-02-2021     Chaoqiong Xiao           Modified comment(s),          */
/*                                            fixed trace enabled error,  */
/*                                            resulting in version 6.1.7  */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_stack_device_configuration_deactivate(UX_DEVICE *device)
{

UX_HOST_CLASS_COMMAND       command;
UX_CONFIGURATION            *configuration;
UX_INTERFACE                *interface;
UINT                        status;


    /* Do a sanity check on the device handle.  */
    if (device -> ux_device_handle != (ULONG) (ALIGN_TYPE) device)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ENUMERATOR, UX_DEVICE_HANDLE_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_DEVICE_HANDLE_UNKNOWN, device, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_DEVICE_HANDLE_UNKNOWN);
    }

    /* Get the configuration.  */
    configuration = device -> ux_device_current_configuration;

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_STACK_DEVICE_CONFIGURATION_DEACTIVATE, device, configuration, 0, 0, UX_TRACE_HOST_STACK_EVENTS, 0, 0)

    /* Protect the control endpoint semaphore here.  It will be unprotected in the
       transfer request function.  */
    status =  _ux_utility_semaphore_get(&device -> ux_device_protection_semaphore, UX_WAIT_FOREVER);

    /* Check for status.  */
    if (status != UX_SUCCESS)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ENUMERATOR, UX_SEMAPHORE_ERROR);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_SEMAPHORE_ERROR, configuration, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_SEMAPHORE_ERROR);
    }

    /* Check for the state of the device, if not configured, we are done.  */
    if (device -> ux_device_state != UX_DEVICE_CONFIGURED)
    {
        _ux_utility_semaphore_put(&device -> ux_device_protection_semaphore);
        return(UX_SUCCESS);
    }

    /* Deactivate classes by command.  */
    command.ux_host_class_command_request =  UX_HOST_CLASS_COMMAND_DEACTIVATE;

    /* Search for the active configuration.  */
    configuration =  device -> ux_device_current_configuration;

    /* If device configured configuration must be activated.  */

    /* We have the correct configuration, search the interface(s).  */
    interface =  configuration -> ux_configuration_first_interface;

    /* Loop to perform the search.  */
    while (interface != UX_NULL)
    {

        /* Check if an instance of the interface is present.  */
        if (interface -> ux_interface_class_instance != UX_NULL)
        {

            /* We need to stop the class instance for the device.  */
            command.ux_host_class_command_instance =  interface -> ux_interface_class_instance;

            /* Call the class.  */
            interface -> ux_interface_class -> ux_host_class_entry_function(&command);
        }

        /* Move to next interface.  */
        interface =  interface -> ux_interface_next_interface;
    }

    /* The device can now be un-configured.  */
    status =  _ux_host_stack_device_configuration_reset(device);

    /* Return completion status.  */
    return(status);
}
