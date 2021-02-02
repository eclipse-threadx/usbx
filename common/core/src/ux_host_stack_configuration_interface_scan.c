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
/*    _ux_host_stack_configuration_interface_scan         PORTABLE C      */
/*                                                           6.1.4        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will scan all default interfaces for a specific       */
/*    configuration and call the registered class with the                */
/*    Class/SubClass/Protocol of the interface.                           */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    configuration                         Configuration pointer         */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Result of operation                                                 */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_host_stack_class_call             Call class command            */
/*    _ux_host_stack_device_configuration_select                          */
/*                                          Select configuration          */
/*    _ux_host_stack_class_call             Call class from host stack    */
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
/*  02-02-2021     Chaoqiong Xiao           Initial Version 6.1.4         */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_stack_configuration_interface_scan(UX_CONFIGURATION *configuration)
{

UX_INTERFACE            *interface;
UINT                    nb_class_owners;
UX_HOST_CLASS           *class;
UX_HOST_CLASS_COMMAND   class_command;
UINT                    status;


    /* Initialize class owners to 0.  */
    nb_class_owners =  0;

    /* Get the first interface container for this configuration.  */
    interface =  configuration -> ux_configuration_first_interface;

    /* We now scan all the alternate settings 0 for each of the interfaces.  */
    while (interface !=  UX_NULL)
    {

        /* Is there a default interface?  */
        if(interface -> ux_interface_descriptor.bAlternateSetting == 0)
        {

            /* We have a default interface for this configuration. Call each class
               with the class\subclass\protocol.  We include the IAD for the cdc classes.  */
            class_command.ux_host_class_command_request      =   UX_HOST_CLASS_COMMAND_QUERY;
            class_command.ux_host_class_command_container    =   (VOID *)interface;
            class_command.ux_host_class_command_usage        =   UX_HOST_CLASS_COMMAND_USAGE_CSP;
            class_command.ux_host_class_command_class        =   interface -> ux_interface_descriptor.bInterfaceClass;
            class_command.ux_host_class_command_subclass     =   interface -> ux_interface_descriptor.bInterfaceSubClass;
            class_command.ux_host_class_command_protocol     =   interface -> ux_interface_descriptor.bInterfaceProtocol;
            class_command.ux_host_class_command_iad_class    =   interface -> ux_interface_iad_class   ;
            class_command.ux_host_class_command_iad_subclass =   interface -> ux_interface_iad_subclass;
            class_command.ux_host_class_command_iad_protocol =   interface -> ux_interface_iad_protocol;

            class =  _ux_host_stack_class_call(&class_command);

            /* On return, either we have found a class or the interface is still an orphan.  */
            if (class != UX_NULL)
            {

                /* There is a class.  */
                nb_class_owners++;
                interface -> ux_interface_class =  class;
            }
        }

        /* point to the next interface until end of the list.  */
        interface =  interface -> ux_interface_next_interface;
    }

    /* Assume no classes.  */
    status = UX_NO_CLASS_MATCH;

    /* Check the number of class owner found.  */
    if (nb_class_owners != 0)
    {

        /* If we have found one or more classes for any of the interfaces,
           we can safely do a SET_CONFIGURATION of the device.  */
        status =  _ux_host_stack_device_configuration_select(configuration);

        /* Check the completion status.  */
        if (status == UX_SUCCESS)
        {

            /* The device is in the CONFIGURED state, we have to call each of the classes
               again with an ACTIVATE signal.  */
            interface =  configuration -> ux_configuration_first_interface;

            while (interface != UX_NULL)
            {

                /* Is there a default interface?  */
                if (interface -> ux_interface_descriptor.bAlternateSetting == 0)
                {

                    /* We have found the default interface. If this interface is owned,
                       activate its class.  */
                    class_command.ux_host_class_command_request =    UX_HOST_CLASS_COMMAND_ACTIVATE;
                    class_command.ux_host_class_command_container =  (VOID *) interface;

                    if (interface -> ux_interface_class != UX_NULL)
                    {

                        /* Save the class in the command container */
                        class_command.ux_host_class_command_class_ptr =  interface -> ux_interface_class;

                        /* Send the ACTIVATE command to the class */
                        status =  interface -> ux_interface_class -> ux_host_class_entry_function(&class_command);

                    }
                }

                /* Point to the next interface until end of the list.  */
                interface =  interface -> ux_interface_next_interface;
            }
        }
    }

    /* Return operation result.  */
    return(status);
}

