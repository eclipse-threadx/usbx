#include "ux_test.h"
#include "ux_api.h"
#include "ux_host_class_storage.h"

#ifdef UX_TEST_RACE_CONDITION_TESTS_ON

/* We override this so we can disconnect the device right before the device state check. We can't do it in the HCD
   because that's after the device state check and that's the only thing that can make us return an error. */
UINT  _ux_host_stack_transfer_request(UX_TRANSFER *transfer_request)
{

UX_ENDPOINT     *endpoint;  
UX_DEVICE       *device;    
UX_HCD          *hcd;
UINT            status;
#ifdef BUGFIX /* USBX_162 */
TX_THREAD       *this_thread;
UINT            old_threshold;
#endif
UX_TEST_HOST_STACK_TRANSFER_REQUEST_PARAMS  params = { transfer_request };
UX_TEST_ACTION                              action;


    action = ux_test_action_handler(UX_TEST_HOST_STACK_TRANSFER_REQUEST, &params);
    ux_test_do_action_before(&action, &params);

    /* Get the endpoint container from the transfer_request */
    endpoint =  transfer_request -> ux_transfer_request_endpoint;

    /* Get the device container from the endpoint.  */
    device =  endpoint -> ux_endpoint_device;
    
#ifdef BUGFIX /* USBX_162 */
    /* Get the pointer to this thread.  */
    this_thread = tx_thread_identify();

    /* Make the following check and assignment atomic. */
    tx_thread_preemption_change(this_thread, 0, &old_threshold);

    /* We can only transfer when the device is ATTACHED, ADDRESSED OR CONFIGURED.  */
    if ((device -> ux_device_state == UX_DEVICE_ATTACHED) || (device -> ux_device_state == UX_DEVICE_ADDRESSED)
            || (device -> ux_device_state == UX_DEVICE_CONFIGURED))
    {
        
        /* Set the pending transfer request.  */
        transfer_request -> ux_transfer_request_completion_code  =     UX_TRANSFER_STATUS_PENDING;

#ifdef BUGFIX /* USBX_115 */
        /* Save the thread waiting for the transfer to complete. */
        transfer_request -> ux_transfer_request_thread_pending = this_thread;
#endif
    }

    tx_thread_preemption_change(this_thread, old_threshold, &old_threshold);

    if (transfer_request -> ux_transfer_request_completion_code != UX_TRANSFER_STATUS_PENDING)
        return(UX_TRANSFER_NOT_READY);

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_STACK_TRANSFER_REQUEST, device, endpoint, transfer_request, 0, UX_TRACE_HOST_STACK_EVENTS, 0, 0)
    
    /* With the device we have the pointer to the HCD.  */
    hcd = UX_DEVICE_HCD_GET(device);

    /* If this is endpoint 0, we protect the endpoint from a possible re-entry.  */
    if ((endpoint -> ux_endpoint_descriptor.bEndpointAddress & (UINT)~UX_ENDPOINT_DIRECTION) == 0)
    {

        /* Check if the class has already protected it.  */
        if (device -> ux_device_protection_semaphore.tx_semaphore_count != 0)        
        {

            /* We are using endpoint 0. Protect with semaphore.  */
            status =  _ux_utility_semaphore_get(&device -> ux_device_protection_semaphore, UX_WAIT_FOREVER);
    
            /* Check for status.  */
            if (status != UX_SUCCESS)
            
                /* Something went wrong. */
                return(status);
        }        
    }             
    
    /* Send the command to the controller.  */    
    status =  hcd -> ux_hcd_entry_function(hcd, UX_HCD_TRANSFER_REQUEST, transfer_request);
    
    /* Check result from transfer request preparation.  */
    if (status == UX_SUCCESS)
    {

        /* If this is endpoint 0, we unprotect the endpoint. */
        if ((endpoint -> ux_endpoint_descriptor.bEndpointAddress & (UINT)~UX_ENDPOINT_DIRECTION) == 0)

            /* We are using endpoint 0. Unprotect with semaphore.  */
            _ux_utility_semaphore_put(&device -> ux_device_protection_semaphore);
    }                
#else
    /* We can only transfer when the device is ATTACHED, ADDRESSED OR CONFIGURED.  */
    if ((device -> ux_device_state == UX_DEVICE_ATTACHED) || (device -> ux_device_state == UX_DEVICE_ADDRESSED)
            || (device -> ux_device_state == UX_DEVICE_CONFIGURED))
    {            
    

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_STACK_TRANSFER_REQUEST, device, endpoint, transfer_request, 0, UX_TRACE_HOST_STACK_EVENTS, 0, 0)
        
        /* With the device we have the pointer to the HCD.  */
        hcd = UX_DEVICE_HCD_GET(device);
    
        /* If this is endpoint 0, we protect the endpoint from a possible re-entry.  */
        if ((endpoint -> ux_endpoint_descriptor.bEndpointAddress & (UINT)~UX_ENDPOINT_DIRECTION) == 0)
        {

            /* Check if the class has already protected it.  */
            if (device -> ux_device_protection_semaphore.tx_semaphore_count != 0)        
            {
    
                /* We are using endpoint 0. Protect with semaphore.  */
                status =  _ux_utility_semaphore_get(&device -> ux_device_protection_semaphore, UX_WAIT_FOREVER);
        
                /* Check for status.  */
                if (status != UX_SUCCESS)
                
                    /* Something went wrong. */
                    return(status);
            }        
        }             
        
        /* Set the pending transfer request.  */
        transfer_request -> ux_transfer_request_completion_code  =     UX_TRANSFER_STATUS_PENDING;
        
        /* Send the command to the controller.  */    
        status =  hcd -> ux_hcd_entry_function(hcd, UX_HCD_TRANSFER_REQUEST, transfer_request);
        
        /* Check result from transfer request preparation.  */
        if (status == UX_SUCCESS)
        {
    
            /* If this is endpoint 0, we unprotect the endpoint. */
            if ((endpoint -> ux_endpoint_descriptor.bEndpointAddress & (UINT)~UX_ENDPOINT_DIRECTION) == 0)
    
                /* We are using endpoint 0. Unprotect with semaphore.  */
                _ux_utility_semaphore_put(&device -> ux_device_protection_semaphore);
        }                
    }
    else
    
        /* We come here when the device is not in a state which allows transmission.  */
        status = UX_TRANSFER_NOT_READY;
#endif

    ux_test_do_action_after(&action, &params);

    /* And return the status.  */
    return(status);
}

UINT  _ux_host_stack_interface_set(UX_INTERFACE *interface)
{

UX_DEVICE               *device;
UX_CONFIGURATION        *configuration;
UX_TRANSFER             *transfer_request;
UINT                    status;
UX_ENDPOINT             *control_endpoint;
UX_TEST_HOST_STACK_TRANSFER_REQUEST_PARAMS  params = { transfer_request };
UX_TEST_ACTION                              action;


    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_STACK_INTERFACE_SET, interface, 0, 0, 0, UX_TRACE_HOST_STACK_EVENTS, 0, 0)

    /* Retrieve the pointer to the control endpoint and its transfer_request.
       From the interface we go back to the configuration, then the device.
       The device contains the default control endpoint container.  */
    configuration =     interface -> ux_interface_configuration;
    device =            configuration -> ux_configuration_device;
    control_endpoint =  &device -> ux_device_control_endpoint;
    transfer_request =  &control_endpoint -> ux_endpoint_transfer_request;

    /* Create a transfer_request for the SET_INTERFACE request. No data for this request */
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_SET_INTERFACE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_index =             (USHORT) interface -> ux_interface_descriptor.bInterfaceNumber;
    transfer_request -> ux_transfer_request_value =             (USHORT) interface -> ux_interface_descriptor.bAlternateSetting;

    action = ux_test_action_handler(UX_TEST_HOST_STACK_INTERFACE_SET, &params);
    ux_test_do_action_before(&action, &params);

    /* Send request to HCD layer.  */
    status =  _ux_host_stack_hcd_transfer_request(transfer_request);

    /* Return status completion.  */
    return(status);
}

#endif