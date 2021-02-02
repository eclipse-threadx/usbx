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
/*    _ux_host_class_printer_soft_reset                   PORTABLE C      */ 
/*                                                           6.1.4        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function performs a soft reset of the printer in case the      */ 
/*    printer gets into an error mode.                                    */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    printer                               Pointer to printer class      */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_stack_transfer_request       Process transfer request      */ 
/*    _ux_utility_semaphore_get             Get protection semaphore      */ 
/*    _ux_utility_semaphore_put             Release protection semaphore  */ 
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
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  02-02-2021     Chaoqiong Xiao           Modified comment(s),          */
/*                                            supported interface other   */
/*                                            than number zero,           */
/*                                            resulting in version 6.1.4  */
/*                                                                        */
/**************************************************************************/
UINT   _ux_host_class_printer_soft_reset(UX_HOST_CLASS_PRINTER *printer)
{


UX_INTERFACE    *interface;
UX_ENDPOINT     *control_endpoint;
UX_TRANSFER     *transfer_request;
UINT            status;

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_PRINTER_SOFT_RESET, printer, 0, 0, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

    /* Ensure the instance is valid.  */
    if (printer -> ux_host_class_printer_state !=  UX_HOST_CLASS_INSTANCE_LIVE)
    {        

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_INSTANCE_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_HOST_CLASS_INSTANCE_UNKNOWN, printer, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_HOST_CLASS_INSTANCE_UNKNOWN);
    }

    /* Protect thread reentry to this instance.  */
    status =  _ux_utility_semaphore_get(&printer -> ux_host_class_printer_semaphore, UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)

        /* Return error.  */
        return(status);

    /* Protect the control endpoint semaphore here.  It will be unprotected in the
       transfer request function.  */
    status =  _ux_utility_semaphore_get(&printer -> ux_host_class_printer_device -> ux_device_protection_semaphore, UX_WAIT_FOREVER);

    /* Check for status.  */
    if (status != UX_SUCCESS)
    {
        _ux_utility_semaphore_put(&printer -> ux_host_class_printer_semaphore);
        return(status);
    }

    /* We need to get the default control endpoint transfer request pointer.  */
    control_endpoint =  &printer -> ux_host_class_printer_device -> ux_device_control_endpoint;
    transfer_request =  &control_endpoint -> ux_endpoint_transfer_request;

    /* Need interface for wIndex.  */
    interface = printer -> ux_host_class_printer_interface;

    /* Create a transfer_request for the SOFT_RESET request.  */
    transfer_request -> ux_transfer_request_data_pointer =      UX_NULL;
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_PRINTER_SOFT_RESET;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Send request to HCD layer.  */
    status =  _ux_host_stack_transfer_request(transfer_request);

    /* Unprotect thread reentry to this instance.  */
    _ux_utility_semaphore_put(&printer -> ux_host_class_printer_semaphore);

    /* Return completion status.  */
    return(status);
}

