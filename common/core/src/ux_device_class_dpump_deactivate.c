/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation 
 * 
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 * 
 * SPDX-License-Identifier: MIT
 **************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** USBX Component                                                        */ 
/**                                                                       */
/**   Device Data Pump Class                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_dpump.h"
#include "ux_device_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_device_class_dpump_deactivate                   PORTABLE C      */ 
/*                                                           6.1.12       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function deactivate an instance of the dpump class.            */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    command                               Pointer to a class command    */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_device_stack_transfer_all_request_abort Abort all transfers     */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Device Data Pump Class                                              */
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  07-29-2022     Chaoqiong Xiao           Modified comment(s),          */
/*                                            fixed parameter/variable    */
/*                                            names conflict C++ keyword, */
/*                                            resulting in version 6.1.12 */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_dpump_deactivate(UX_SLAVE_CLASS_COMMAND *command)
{
                                          
UX_SLAVE_INTERFACE          *interface_ptr;
UX_SLAVE_CLASS              *class_ptr;
UX_SLAVE_CLASS_DPUMP        *dpump;
UX_SLAVE_ENDPOINT           *endpoint_in;
UX_SLAVE_ENDPOINT           *endpoint_out;

    /* Get the class container.  */
    class_ptr =  command -> ux_slave_class_command_class_ptr;

    /* Store the class instance in the container.  */
    dpump = (UX_SLAVE_CLASS_DPUMP *) class_ptr -> ux_slave_class_instance;

    /* We need the interface to the class.  */
    interface_ptr =  dpump -> ux_slave_class_dpump_interface;
    
    /* Locate the endpoints.  */
    endpoint_in =  interface_ptr -> ux_slave_interface_first_endpoint;
    
    /* Check the endpoint direction, if IN we have the correct endpoint.  */
    if ((endpoint_in -> ux_slave_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) != UX_ENDPOINT_IN)
    {

        /* Wrong direction, we found the OUT endpoint first.  */
        endpoint_out =  endpoint_in;
            
        /* So the next endpoint has to be the IN endpoint.  */
        endpoint_in =  endpoint_out -> ux_slave_endpoint_next_endpoint;
    }
    else
    {

        /* We found the endpoint IN first, so next endpoint is OUT.  */
        endpoint_out =  endpoint_in -> ux_slave_endpoint_next_endpoint;
    }
        
    /* Terminate the transactions pending on the endpoints.  */
    _ux_device_stack_transfer_all_request_abort(endpoint_in, UX_TRANSFER_BUS_RESET);
    _ux_device_stack_transfer_all_request_abort(endpoint_out, UX_TRANSFER_BUS_RESET);

    /* If there is a deactivate function call it.  */
    if (dpump -> ux_slave_class_dpump_parameter.ux_slave_class_dpump_instance_deactivate != UX_NULL)
    {
    
        /* Invoke the application.  */
        dpump -> ux_slave_class_dpump_parameter.ux_slave_class_dpump_instance_deactivate(dpump);
    }

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_CLASS_DPUMP_DEACTIVATE, dpump, 0, 0, 0, UX_TRACE_DEVICE_CLASS_EVENTS, 0, 0)
  
    /* If trace is enabled, register this object.  */
    UX_TRACE_OBJECT_UNREGISTER(dpump);

    /* Return completion status.  */
    return(UX_SUCCESS);
}

