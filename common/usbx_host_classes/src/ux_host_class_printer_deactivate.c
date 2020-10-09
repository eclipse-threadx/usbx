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
/**   Printer Class                                                       */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_printer.h"
#include "ux_host_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_printer_deactivate                   PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function is called when this instance of the printer has been  */
/*    removed from the bus either directly or indirectly. The bulk in\out */ 
/*    pipes will be destroyed and the instanced removed.                  */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    command                               Printer class command pointer */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_stack_class_instance_destroy Destroy the class instance    */ 
/*    _ux_host_stack_endpoint_transfer_abort                              */
/*                                          Abort endpoint transfer       */ 
/*    _ux_utility_memory_free               Free memory block             */ 
/*    _ux_utility_semaphore_get             Get protection semaphore      */ 
/*    _ux_utility_semaphore_delete          Delete protection semaphore   */ 
/*    _ux_utility_thread_schedule_other     Schedule other threads        */
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    _ux_host_class_printer_entry          Entry of printer class        */ 
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
UINT  _ux_host_class_printer_deactivate(UX_HOST_CLASS_COMMAND *command)
{

UX_HOST_CLASS_PRINTER       *printer;
UINT                        status;


    /* Get the instance for this class.  */
    printer =  (UX_HOST_CLASS_PRINTER *) command -> ux_host_class_command_instance;

    /* The printer is being shut down.  */
    printer -> ux_host_class_printer_state =  UX_HOST_CLASS_INSTANCE_SHUTDOWN;

    /* Protect thread reentry to this instance.  */
    status =  _ux_utility_semaphore_get(&printer -> ux_host_class_printer_semaphore, UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)

        /* Return error.  */
        return(status);
    
    /* We need to abort transactions on the bulk out pipe.  */
    _ux_host_stack_endpoint_transfer_abort(printer -> ux_host_class_printer_bulk_out_endpoint);

    /* If the printer is bidirectional, we need to abort transactions on the bulk in pipe.  */
    if (printer -> ux_host_class_printer_interface -> ux_interface_descriptor.bInterfaceProtocol == UX_HOST_CLASS_PRINTER_PROTOCOL_BI_DIRECTIONAL)
        _ux_host_stack_endpoint_transfer_abort(printer -> ux_host_class_printer_bulk_in_endpoint);

    /* The enumeration thread needs to sleep a while to allow the application or the class that may be using
       endpoints to exit properly.  */
    _ux_utility_thread_schedule_other(UX_THREAD_PRIORITY_ENUM); 

    /* Destroy the instance.  */
    _ux_host_stack_class_instance_destroy(printer -> ux_host_class_printer_class, (VOID *) printer);

    /* Destroy the semaphore.  */
    _ux_utility_semaphore_delete(&printer -> ux_host_class_printer_semaphore);

    /* Before we free the device resources, we need to inform the application
        that the device is removed.  */
    if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
    {
        
        /* Inform the application the device is removed.  */
        _ux_system_host -> ux_system_host_change_function(UX_DEVICE_REMOVAL, printer -> ux_host_class_printer_class, (VOID *) printer);
    }

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_PRINTER_DEACTIVATE, printer, 0, 0, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

    /* If trace is enabled, register this object.  */
    UX_TRACE_OBJECT_UNREGISTER(printer);

    /* Free the printer instance memory.  */
    _ux_utility_memory_free(printer);

    /* Return successful status.  */
    return(UX_SUCCESS);         
}

