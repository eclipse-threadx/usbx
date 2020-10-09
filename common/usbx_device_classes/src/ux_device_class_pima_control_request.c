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
/**   Device PIMA Class                                                   */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_pima.h"
#include "ux_device_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_device_class_pima_control_request               PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function manages the based sent by the host on the control     */ 
/*    endpoints with a CLASS or VENDOR SPECIFIC type.                     */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    pima                               Pointer to pima class            */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    None                                                                */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_device_stack_transfer_request     Transfer request              */ 
/*    _ux_device_stack_transfer_abort       Transfer abort                */
/*    _ux_device_class_pima_event_set       Put PIMA event into queue     */
/*    _ux_utility_short_put                 Put 16-bit value into buffer  */
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    PIMA Class                                                          */
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
UINT  _ux_device_class_pima_control_request(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_TRANSFER           *transfer_request;
UX_SLAVE_DEVICE             *device;
UX_SLAVE_CLASS              *class;
UX_SLAVE_CLASS_PIMA_EVENT   pima_event;
ULONG                       request;
UX_SLAVE_CLASS_PIMA         *pima;

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* Get the pointer to the transfer request associated with the control endpoint.  */
    transfer_request =  &device -> ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

    /* Extract all necessary fields of the request.  */
    request =  *(transfer_request -> ux_slave_transfer_request_setup + UX_SETUP_REQUEST);
    
     /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;
    
    /* Get the storage instance from this class container.  */
    pima =  (UX_SLAVE_CLASS_PIMA *) class -> ux_slave_class_instance;

    /* Here we proceed only the standard request we know of at the device level.  */
    switch (request)
    {

        case UX_DEVICE_CLASS_PIMA_REQUEST_CANCEL_COMMAND:

            /* We need to cancel the request that is currently pending.  It could be either
               on the bulk in or bulk out endpoint.  Load the bulk in transfer request first.  */
            transfer_request = &pima -> ux_device_class_pima_bulk_in_endpoint -> ux_slave_endpoint_transfer_request;
            
            /* And check if we have something waiting on this one.  */
            if (transfer_request -> ux_slave_transfer_request_status == UX_TRANSFER_STATUS_PENDING)               

                /* Yes, abort it.  */
                _ux_device_stack_transfer_abort(transfer_request, UX_TRANSFER_STATUS_ABORT);

            /* Maybe the bulk out is busy too.  */
            transfer_request = &pima -> ux_device_class_pima_bulk_out_endpoint -> ux_slave_endpoint_transfer_request;
            
            /* And check if we have something waiting on this one.  */
            if (transfer_request -> ux_slave_transfer_request_status == UX_TRANSFER_STATUS_PENDING)               

                /* Yes, abort it.  */
                _ux_device_stack_transfer_abort(transfer_request, UX_TRANSFER_STATUS_ABORT);

            /* The host is now waiting for an event of transfer cancelled to proceed.  
               Prepare an event command and set the event code to transaction cancelled*/
            pima_event.ux_device_class_pima_event_code =  UX_DEVICE_CLASS_PIMA_EC_CANCEL_TRANSACTION;

            /* Set all parameters to a 0 value.  */
            pima_event.ux_device_class_pima_event_parameter_1 =  0;
            pima_event.ux_device_class_pima_event_parameter_2 =  0;
            pima_event.ux_device_class_pima_event_parameter_3 =  0;
            
            /* Send the pima event into the queue. This will be processed by an asynchronous thread.  */            
            _ux_device_class_pima_event_set(pima, &pima_event);

            break ;

        case UX_DEVICE_CLASS_PIMA_REQUEST_STATUS_COMMAND:
            
            /* Fill the status data payload.  First with length.  */
            _ux_utility_short_put(transfer_request -> ux_slave_transfer_request_data_pointer, 4);

            /* Then the status.  */
            _ux_utility_short_put(transfer_request -> ux_slave_transfer_request_data_pointer + 2, UX_DEVICE_CLASS_PIMA_RC_OK);

            /* We have a request to obtain the status of the MTP. */
            _ux_device_stack_transfer_request(transfer_request, 4, 4);

            break ;
            
        default:

            /* Unknown function. It's not handled.  */
            return(UX_ERROR);
    }

    /* It's handled.  */
    return(UX_SUCCESS);
}

