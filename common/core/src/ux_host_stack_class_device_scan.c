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
/*    _ux_host_stack_class_device_scan                    PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function will scan all registered classes with a device        */ 
/*    candidate. Priority is given to the PID/VID and then the            */ 
/*    Class/Subclass/Protocol.                                            */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    device                                Pointer to device             */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_stack_class_call             Call host stack class         */ 
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
UINT  _ux_host_stack_class_device_scan(UX_DEVICE *device)
{

UINT                        status;
UX_HOST_CLASS               *class;
UX_HOST_CLASS_COMMAND       class_command;

    /* Perform the command initialization.  */
    class_command.ux_host_class_command_request      =   UX_HOST_CLASS_COMMAND_QUERY;
    class_command.ux_host_class_command_container    =   (VOID *) device;
    class_command.ux_host_class_command_vid          =   device -> ux_device_descriptor.idVendor;
    class_command.ux_host_class_command_pid          =   device -> ux_device_descriptor.idProduct;
    class_command.ux_host_class_command_class        =   device -> ux_device_descriptor.bDeviceClass;
    class_command.ux_host_class_command_subclass     =   device -> ux_device_descriptor.bDeviceSubClass;
    class_command.ux_host_class_command_subclass     =   device -> ux_device_descriptor.bDeviceSubClass;
    class_command.ux_host_class_command_protocol     =   device -> ux_device_descriptor.bDeviceProtocol;
    class_command.ux_host_class_command_iad_class    =   0;
    class_command.ux_host_class_command_iad_subclass =   0;
    class_command.ux_host_class_command_iad_protocol =   0;

    /* We start with the PID/VID for this device.  */
    class_command.ux_host_class_command_usage =  UX_HOST_CLASS_COMMAND_USAGE_PIDVID;
    class =  _ux_host_stack_class_call(&class_command);

    /* On return, either we have found a class or the device is still an orphan.  */
    if (class != UX_NULL)
    {

        device -> ux_device_class =  class;
        class_command.ux_host_class_command_class_ptr =  class;
        class_command.ux_host_class_command_request =  UX_HOST_CLASS_COMMAND_ACTIVATE;
        status =  device -> ux_device_class ->  ux_host_class_entry_function(&class_command);

        /* Return result of activation.  */
        return(status);
    }

    /* It the PID/VID did not work, we continue looking for the Class\Subclass\Protocol match. */  
    class_command.ux_host_class_command_request      =   UX_HOST_CLASS_COMMAND_QUERY;
    class_command.ux_host_class_command_container    =   (VOID *) device;
    class_command.ux_host_class_command_usage        =   UX_HOST_CLASS_COMMAND_USAGE_CSP;
    
    class =  _ux_host_stack_class_call(&class_command);

    /* On return, either we have found a class or the device is still an orphan.  */
    if (class != UX_NULL)
    {

        device -> ux_device_class =  class;
        class_command.ux_host_class_command_class_ptr =  class;
        class_command.ux_host_class_command_request =  UX_HOST_CLASS_COMMAND_ACTIVATE;
        status =  device -> ux_device_class ->  ux_host_class_entry_function(&class_command);

        /* Return result of activation.  */
        return(status);
    }

    /* Return an error.  */
    return(UX_NO_CLASS_MATCH);
}

