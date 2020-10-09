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
/**   Asix Class                                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_asix.h"
#include "ux_host_stack.h"

UX_HOST_CLASS_ASIX_NX_ETHERNET_POOL_ALLOCSIZE_ASSERT

/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_asix_activate                        PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function creates the asix instance, configure the device.      */ 
/*    WARNING !!!! The NX_PHYSICAL_HEADER should be set to 20 in nx_api.h */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    command                                Asix class command pointer   */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_class_asix_configure            Configure asix class       */ 
/*    _ux_host_class_asix_endpoints_get        Get endpoints of asix      */ 
/*    _ux_host_class_asix_setup                Set up asix                */
/*    _ux_host_stack_class_instance_create     Create class instance      */ 
/*    _ux_host_stack_class_instance_destroy    Destroy the class instance */ 
/*    _ux_host_stack_transfer_request          Transfer request           */
/*    _ux_utility_memory_allocate              Allocate memory block      */ 
/*    _ux_utility_memory_free                  Free memory block          */ 
/*    _ux_utility_semaphore_create             Create semaphore           */
/*    _ux_utility_semaphore_delete             Delete semaphore           */
/*    _ux_utility_thread_create                Create thread              */
/*    _ux_utility_thread_delete                Delete thread              */
/*    nx_packet_pool_create                    Create NetX packet pool    */
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    _ux_host_class_asix_entry                Entry of asix class        */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            used UX prefix to refer to  */
/*                                            TX symbols instead of using */
/*                                            them directly,              */
/*                                            resulting in version 6.1    */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_asix_activate(UX_HOST_CLASS_COMMAND *command)
{
#if NX_PHYSICAL_HEADER < 20

    UX_PARAMETER_NOT_USED(command);

    /* Error trap - function not supported due to NX lib settings.  */
    _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED);
    return(UX_FUNCTION_NOT_SUPPORTED);
#else
UX_DEVICE                           *device;
UX_HOST_CLASS_ASIX                  *asix;
UINT                                status;
UX_TRANSFER                         *transfer_request;
    
    /* We need to make sure that the value of the NX_PHYSICAL_HEADER is at least 20.  
       This should be changed in the nx_user.h file */


    /* The asix class is always activated by the device descriptor. */
    device =  (UX_DEVICE *) command -> ux_host_class_command_container;

    /* Obtain memory for this class instance.  */
    asix =  _ux_utility_memory_allocate(UX_NO_ALIGN, UX_CACHE_SAFE_MEMORY, sizeof(UX_HOST_CLASS_ASIX));
    if (asix == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Store the class container into this instance.  */
    asix -> ux_host_class_asix_class =  command -> ux_host_class_command_class_ptr;

    /* Store the device container into the asix class instance.  */
    asix -> ux_host_class_asix_device =  device;

    /* Store the instance in the device container, this is for the USBX stack
       when it needs to invoke the class for deactivation.  */        
    device -> ux_device_class_instance =  (VOID *) asix;

    /* Create this class instance.  */
    _ux_host_stack_class_instance_create(asix -> ux_host_class_asix_class, (VOID *) asix);

    /* Configure the asix class.  */
    status =  _ux_host_class_asix_configure(asix);     

    /* Get the asix endpoint(s). We need to search for Bulk Out and Bulk In endpoints 
       and the interrupt endpoint.  */
    if (status == UX_SUCCESS)
        status =  _ux_host_class_asix_endpoints_get(asix);

    /* Create the semaphore to protect 2 threads from accessing the same asix instance.  */
    if (status == UX_SUCCESS)
    {
        status =  _ux_utility_semaphore_create(&asix -> ux_host_class_asix_semaphore, "ux_host_class_asix_semaphore", 1);
        if (status != UX_SUCCESS)
            status = UX_SEMAPHORE_ERROR;
    }

    /* Create the semaphore to wake up the Asix thread.  */
    if(status == UX_SUCCESS)
    {
        status =  _ux_utility_semaphore_create(&asix -> ux_host_class_asix_interrupt_notification_semaphore, "ux_host_class_asix_interrupt_notification_semaphore", 0);
        if (status != UX_SUCCESS)
            status = UX_SEMAPHORE_ERROR;
    }

    /* Allocate a Thread stack.  */
    if (status == UX_SUCCESS)
    {
        asix -> ux_host_class_asix_thread_stack =  
                    _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, UX_THREAD_STACK_SIZE);

        /* Check the completion status.  */
        if (asix -> ux_host_class_asix_thread_stack == UX_NULL)
            status = UX_MEMORY_INSUFFICIENT;
    }

    /* Create the asix class thread.  */
    if (status == UX_SUCCESS)
    {
        status =  _ux_utility_thread_create(&asix -> ux_host_class_asix_thread,
                                "ux_asix_thread", _ux_host_class_asix_thread,
                                (ULONG) asix, 
                                asix -> ux_host_class_asix_thread_stack,
                                UX_THREAD_STACK_SIZE, 
                                UX_THREAD_PRIORITY_CLASS,
                                UX_THREAD_PRIORITY_CLASS,
                                TX_NO_TIME_SLICE, UX_AUTO_START);
                    
        /* Check the completion status.  */
        if (status != UX_SUCCESS)
            status = UX_THREAD_ERROR;
    }

    UX_THREAD_EXTENSION_PTR_SET(&(asix -> ux_host_class_asix_thread), asix)

    /* The asix chip needs to be setup properly.  */
    if (status == UX_SUCCESS)
        status = _ux_host_class_asix_setup(asix);

    /* Go on if there is no error.  */
    if (status == UX_SUCCESS)
    {

        /* The ethernet link is down by default.  */
        asix -> ux_host_class_asix_link_state = UX_HOST_CLASS_ASIX_LINK_STATE_DOWN;
        
        /* Start the interrupt pipe now.  */
        transfer_request =  &asix -> ux_host_class_asix_interrupt_endpoint -> ux_endpoint_transfer_request;
        status =  _ux_host_stack_transfer_request(transfer_request);
    }

    /* Allocate some packet pool for reception.  */
    if (status == UX_SUCCESS)
    {

        /* UX_HOST_CLASS_ASIX_NX_ETHERNET_POOL_ALLOCSIZE overflow has been checked by
         * UX_HOST_CLASS_ASIX_NX_ETHERNET_POOL_ALLOCSIZE_ASSERT outside of function.
         */
        asix -> ux_host_class_asix_pool_memory =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_ASIX_NX_ETHERNET_POOL_ALLOCSIZE);
        if (asix -> ux_host_class_asix_pool_memory  == UX_NULL)
            status = UX_MEMORY_INSUFFICIENT;
    }

    /* Create a packet pool.  */
    if (status == UX_SUCCESS)
    {
        status =  nx_packet_pool_create(&asix -> ux_host_class_asix_packet_pool, "Asix Packet Pool", 
                                    UX_HOST_CLASS_ASIX_NX_PAYLOAD_SIZE, asix -> ux_host_class_asix_pool_memory, UX_HOST_CLASS_ASIX_NX_ETHERNET_POOL_ALLOCSIZE);

        /* Check for pool creation error.  */
        if (status)
            status = UX_ERROR;
    }

    /* Do final things if success.  */
    if (status == UX_SUCCESS)
    {

        /* Mark the asix instance as live now.  */
        asix -> ux_host_class_asix_state =  UX_HOST_CLASS_INSTANCE_LIVE;
        
        /* If all is fine and the device is mounted, we need to inform the application
        if a function has been programmed in the system structure. */
        if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
        {
            
            /* Call system change function.  */
            _ux_system_host ->  ux_system_host_change_function(UX_DEVICE_INSERTION, asix -> ux_host_class_asix_class, (VOID *) asix);
        }

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_ASIX_ACTIVATE, asix, 0, 0, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

        /* If trace is enabled, register this object.  */
        UX_TRACE_OBJECT_REGISTER(UX_TRACE_HOST_OBJECT_TYPE_INTERFACE, asix, 0, 0, 0)

        /* Return completion status.  */
        return(UX_SUCCESS);    
    }

    /* There was a problem, so free the resources.  */

    /* Last resource, asix -> ux_host_class_asix_packet_pool is not created or created error,
       no need to free.  */

    /* Free asix -> ux_host_class_asix_pool_memory.  */
    if (asix -> ux_host_class_asix_pool_memory)
        _ux_utility_memory_free(asix -> ux_host_class_asix_pool_memory);

    /* Free asix -> ux_host_class_asix_thread.  */
    if (asix -> ux_host_class_asix_thread.tx_thread_id)
        _ux_utility_thread_delete(&asix -> ux_host_class_asix_thread);

    /* Free asix -> ux_host_class_asix_thread_stack.  */
    if (asix -> ux_host_class_asix_thread_stack)
        _ux_utility_memory_free(asix -> ux_host_class_asix_thread_stack);

    /* Free asix -> ux_host_class_asix_interrupt_notification_semaphore.  */
    if (asix -> ux_host_class_asix_interrupt_notification_semaphore.tx_semaphore_id != 0)
        _ux_utility_semaphore_delete(&asix -> ux_host_class_asix_interrupt_notification_semaphore);

    /* Free asix -> ux_host_class_asix_semaphore.  */
    if (asix -> ux_host_class_asix_semaphore.tx_semaphore_id != 0)
        _ux_utility_semaphore_delete(&asix -> ux_host_class_asix_semaphore);

    /* Destroy class instance.  */
    _ux_host_stack_class_instance_destroy(asix -> ux_host_class_asix_class, (VOID *) asix);

    /* Detach instance.  */
    device -> ux_device_class_instance = UX_NULL;

    /* Free instance memory.  */
    _ux_utility_memory_free(asix);

    /* Return completion status.  */
    return(status);
#endif
}

