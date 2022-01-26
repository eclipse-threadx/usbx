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
/**   ASIX Class                                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_asix.h"
#include "ux_host_stack.h"

/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_asix_thread                          PORTABLE C      */ 
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This is the Asix thread that monitors the link change flag.         */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    asix                                   Asix instance                */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_stack_transfer_request        Transfer request             */
/*    _ux_host_semaphore_get                 Get semaphore                */
/*    _ux_host_semaphore_put                 Put semaphore                */
/*    _ux_utility_memory_allocate            Allocate memory              */
/*    _ux_utility_memory_free                Free memory                  */
/*    _ux_utility_memory_set                 Set memory                   */
/*    _ux_network_driver_activate            Activate NetX USB interface  */
/*    _ux_network_driver_link_down           Set state to link down       */
/*    _ux_network_driver_link_up             Set state to link up         */
/*    nx_packet_allocate                     Allocate NetX packet         */
/*    nx_packet_transmit_release             Release NetX packet          */
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Asix class initialization                                           */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            verified memset and memcpy  */
/*                                            cases,                      */
/*                                            resulting in version 6.1    */
/*  01-31-2022     Chaoqiong Xiao           Modified comment(s),          */
/*                                            refined macros names,       */
/*                                            resulting in version 6.1.10 */
/*                                                                        */
/**************************************************************************/
VOID  _ux_host_class_asix_thread(ULONG parameter)
{

UX_HOST_CLASS_ASIX          *asix;
UCHAR                       *setup_buffer;
UX_ENDPOINT                 *control_endpoint;
UX_TRANSFER                 *transfer_request;
NX_PACKET                   *packet;
NX_PACKET                   *current_packet;
NX_PACKET                   *next_packet;
UINT                        status;
ULONG                        physical_address_msw;
ULONG                        physical_address_lsw;

    /* Cast the parameter passed in the thread into the asix pointer.  */
    UX_THREAD_EXTENSION_PTR_GET(asix, UX_HOST_CLASS_ASIX, parameter)

    /* Loop forever waiting for changes signaled through the semaphore. */     
    while (1)
    {   

        /* Wait for the semaphore to be put by the asix interrupt event.  */
        status = _ux_host_semaphore_get(&asix -> ux_host_class_asix_interrupt_notification_semaphore, UX_WAIT_FOREVER);

        /* Check for successful completion.  */
        if (status != UX_SUCCESS)
    
            return;

        /* Protect Thread reentry to this instance.  */
        status =  _ux_host_semaphore_get(&asix -> ux_host_class_asix_semaphore, UX_WAIT_FOREVER);

        /* Check for successful completion.  */
        if (status != UX_SUCCESS)
    
            return;
    
        /* Check the link state. It is either pending up or down.  */
        if (asix -> ux_host_class_asix_link_state == UX_HOST_CLASS_ASIX_LINK_STATE_PENDING_UP)
        {

            /* We need to get the default control endpoint transfer request pointer.  */
            control_endpoint =  &asix -> ux_host_class_asix_device -> ux_device_control_endpoint;
            transfer_request =  &control_endpoint -> ux_endpoint_transfer_request;

            /* Need to allocate memory for the buffer.  */
            setup_buffer =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_ASIX_SETUP_BUFFER_SIZE);
            if (setup_buffer == UX_NULL)
            {

                /* Unprotect thread reentry to this instance.  */
                _ux_host_semaphore_put(&asix -> ux_host_class_asix_semaphore);

                return;
            }                

            /* Request ownership of Serial Management Interface.  */
            transfer_request -> ux_transfer_request_data_pointer        =  UX_NULL;
            transfer_request -> ux_transfer_request_requested_length    =  0;
            transfer_request -> ux_transfer_request_function            =  UX_HOST_CLASS_ASIX_REQ_OWN_SMI ;
            transfer_request -> ux_transfer_request_type                =  UX_REQUEST_OUT | UX_REQUEST_TYPE_VENDOR | UX_REQUEST_TARGET_DEVICE;
            transfer_request -> ux_transfer_request_value               =  0;
            transfer_request -> ux_transfer_request_index               =  0;
        
            /* Send request to HCD layer.  */
            status =  _ux_host_stack_transfer_request(transfer_request);
        
            /* Check status, if error, do not proceed.  */
            if (status != UX_SUCCESS || transfer_request -> ux_transfer_request_completion_code != UX_SUCCESS)
            {
        
                /* Free all used resources.  */
                _ux_utility_memory_free(setup_buffer);

                /* Unprotect thread reentry to this instance.  */
                _ux_host_semaphore_put(&asix -> ux_host_class_asix_semaphore);

                /* Return completion status.  */
                return;
        
            }
        
            /* Get the value of the PHYIDR1 in the PHY register.  */
            transfer_request -> ux_transfer_request_data_pointer        =  setup_buffer;
            transfer_request -> ux_transfer_request_requested_length    =  2;
            transfer_request -> ux_transfer_request_function            =  UX_HOST_CLASS_ASIX_REQ_READ_PHY_REG;
            transfer_request -> ux_transfer_request_type                =  UX_REQUEST_IN | UX_REQUEST_TYPE_VENDOR | UX_REQUEST_TARGET_DEVICE;
            transfer_request -> ux_transfer_request_value               =  asix -> ux_host_class_asix_primary_phy_id;
            transfer_request -> ux_transfer_request_index               =  UX_HOST_CLASS_ASIX_PHY_REG_ANLPAR;
        
            /* Send request to HCD layer.  */
            status =  _ux_host_stack_transfer_request(transfer_request);
        
            /* Check status, if error, do not proceed.  */
            if (status != UX_SUCCESS || transfer_request -> ux_transfer_request_completion_code != UX_SUCCESS || 
                transfer_request -> ux_transfer_request_actual_length != 2)
            {
        
                /* Free all used resources.  */
                _ux_utility_memory_free(setup_buffer);

                /* Unprotect thread reentry to this instance.  */
                _ux_host_semaphore_put(&asix -> ux_host_class_asix_semaphore);

                /* Return completion status.  */
                return;
        
            }

            /* Release SMI ownership. */
            transfer_request -> ux_transfer_request_data_pointer        =  UX_NULL;
            transfer_request -> ux_transfer_request_requested_length    =  0;
            transfer_request -> ux_transfer_request_function            =  UX_HOST_CLASS_ASIX_REQ_RELEASE_SMI;
            transfer_request -> ux_transfer_request_type                =  UX_REQUEST_OUT | UX_REQUEST_TYPE_VENDOR | UX_REQUEST_TARGET_DEVICE;
            transfer_request -> ux_transfer_request_value               =  0;
            transfer_request -> ux_transfer_request_index               =  0;
        
            /* Send request to HCD layer.  */
            status =  _ux_host_stack_transfer_request(transfer_request);
        
            /* Check status, if error, do not proceed.  */
            if (status != UX_SUCCESS || transfer_request -> ux_transfer_request_completion_code != UX_SUCCESS)
            {
        
                /* Free all used resources.  */
                _ux_utility_memory_free(setup_buffer);

                /* Unprotect thread reentry to this instance.  */
                _ux_host_semaphore_put(&asix -> ux_host_class_asix_semaphore);

                /* Return completion status.  */
                return;
        
            }
    
            /* Check speed.  */
            if (asix -> ux_host_class_asix_speed_selected == UX_HOST_CLASS_ASIX_SPEED_SELECTED_100MPBS)
            
                /* Set speed at 100MBPS.  */
                transfer_request -> ux_transfer_request_value =  UX_HOST_CLASS_ASIX_MEDIUM_PS;
            
            /* Write the value of the Medium Mode. */
            transfer_request -> ux_transfer_request_data_pointer        =  UX_NULL;
            transfer_request -> ux_transfer_request_requested_length    =  0;
            transfer_request -> ux_transfer_request_function            =  UX_HOST_CLASS_ASIX_REQ_WRITE_MEDIUM_MODE;
            transfer_request -> ux_transfer_request_type                =  UX_REQUEST_OUT | UX_REQUEST_TYPE_VENDOR | UX_REQUEST_TARGET_DEVICE;
            transfer_request -> ux_transfer_request_value               |=  (UX_HOST_CLASS_ASIX_MEDIUM_FD | UX_HOST_CLASS_ASIX_MEDIUM_BIT2 | UX_HOST_CLASS_ASIX_MEDIUM_RFC_ENABLED |
                                                                            UX_HOST_CLASS_ASIX_MEDIUM_TFC_ENABLED | UX_HOST_CLASS_ASIX_MEDIUM_RE_ENABLED);
            transfer_request -> ux_transfer_request_index               =  0;
        
            /* Send request to HCD layer.  */
            status =  _ux_host_stack_transfer_request(transfer_request);
        
            /* Check status, if error, do not proceed.  */
            if (status != UX_SUCCESS || transfer_request -> ux_transfer_request_completion_code != UX_SUCCESS)
            {
        
                /* Free all used resources.  */
                _ux_utility_memory_free(setup_buffer);

                /* Unprotect thread reentry to this instance.  */
                _ux_host_semaphore_put(&asix -> ux_host_class_asix_semaphore);

                /* Return completion status.  */
                return;
        
            }

            /* Set the Rx Control register value.  */
            transfer_request -> ux_transfer_request_data_pointer        =  UX_NULL;
            transfer_request -> ux_transfer_request_requested_length    =  0;
            transfer_request -> ux_transfer_request_function            =  UX_HOST_CLASS_ASIX_REQ_WRITE_RX_CTL;
            transfer_request -> ux_transfer_request_type                =  UX_REQUEST_OUT | UX_REQUEST_TYPE_VENDOR | UX_REQUEST_TARGET_DEVICE;
            transfer_request -> ux_transfer_request_value               =  (UX_HOST_CLASS_ASIX_RXCR_AM | UX_HOST_CLASS_ASIX_RXCR_AB | 
                                                                            UX_HOST_CLASS_ASIX_RXCR_SO | UX_HOST_CLASS_ASIX_RXCR_MFB_2048);
            transfer_request -> ux_transfer_request_index               =  0;
        
            /* Send request to HCD layer.  */
            status =  _ux_host_stack_transfer_request(transfer_request);
        
            /* Check status, if error, do not proceed.  */
            if (status != UX_SUCCESS || transfer_request -> ux_transfer_request_completion_code != UX_SUCCESS)
            {
        
                /* Free all used resources.  */
                _ux_utility_memory_free(setup_buffer);
        
                /* Unprotect thread reentry to this instance.  */
                _ux_host_semaphore_put(&asix -> ux_host_class_asix_semaphore);

                /* Return completion status.  */
                return;
        
            }
        
            /* Set the multicast value.  */
            transfer_request -> ux_transfer_request_data_pointer        =  setup_buffer;
            transfer_request -> ux_transfer_request_requested_length    =  8;
            transfer_request -> ux_transfer_request_function            =  UX_HOST_CLASS_ASIX_REQ_WRITE_MULTICAST_FILTER;
            transfer_request -> ux_transfer_request_type                =  UX_REQUEST_OUT | UX_REQUEST_TYPE_VENDOR | UX_REQUEST_TARGET_DEVICE;
            transfer_request -> ux_transfer_request_value               =  0;
            transfer_request -> ux_transfer_request_index               =  0;

            /* Fill in the multicast filter.  */
            _ux_utility_memory_set(setup_buffer, 0, 8); /* Use case of memset is verified. */
            *(setup_buffer +1) = 0x40;
        
            /* Send request to HCD layer.  */
            status =  _ux_host_stack_transfer_request(transfer_request);
        
            /* Check status, if error, do not proceed.  */
            if (status != UX_SUCCESS || transfer_request -> ux_transfer_request_completion_code != UX_SUCCESS || 
                transfer_request -> ux_transfer_request_actual_length != 8)
            {
        
                /* Free all used resources.  */
                _ux_utility_memory_free(setup_buffer);
        
                /* Unprotect thread reentry to this instance.  */
                _ux_host_semaphore_put(&asix -> ux_host_class_asix_semaphore);

                /* Return completion status.  */
                return;
        
            }

            /* Free all used resources.  */
            _ux_utility_memory_free(setup_buffer);

            /* Setup the physical address of this IP instance.  */
            physical_address_msw =  (ULONG)((asix -> ux_host_class_asix_node_id[0] << 8) | (asix -> ux_host_class_asix_node_id[1]));
            physical_address_lsw =  (ULONG)((asix -> ux_host_class_asix_node_id[2] << 24) | (asix -> ux_host_class_asix_node_id[3] << 16) | 
                                                           (asix -> ux_host_class_asix_node_id[4] << 8) | (asix -> ux_host_class_asix_node_id[5]));
            
            /* Register this interface to the NetX USB interface broker.  */
            status = _ux_network_driver_activate((VOID *) asix, _ux_host_class_asix_write, 
                                        &asix -> ux_host_class_asix_network_handle, 
                                        physical_address_msw,
                                        physical_address_lsw);
            
            /* Now the link is up.  */
            asix -> ux_host_class_asix_link_state = UX_HOST_CLASS_ASIX_LINK_STATE_UP;

            /* Communicate the state with the network driver.  */
            _ux_network_driver_link_up(asix -> ux_host_class_asix_network_handle);
            
            /* We can accept reception. Get a NX Packet */
            if (nx_packet_allocate(&asix -> ux_host_class_asix_packet_pool, &packet, 
                                    NX_RECEIVE_PACKET, NX_NO_WAIT) == NX_SUCCESS)            
            {


                /* Adjust the prepend pointer to take into account the non 3 bit alignment of the ethernet header.  */
                packet -> nx_packet_prepend_ptr += sizeof(USHORT);

                /* We have a packet.  Link this packet to the reception transfer request on the bulk in endpoint. */
                transfer_request =  &asix -> ux_host_class_asix_bulk_in_endpoint -> ux_endpoint_transfer_request;
                
                /* Set the data pointer.  */                
                transfer_request -> ux_transfer_request_data_pointer     =  packet -> nx_packet_prepend_ptr;

                /* And length.  */
                transfer_request -> ux_transfer_request_requested_length =  UX_HOST_CLASS_ASIX_NX_PAYLOAD_SIZE;
                transfer_request -> ux_transfer_request_actual_length =     0;

                /* Store the packet that owns this transaction.  */
                transfer_request -> ux_transfer_request_user_specific = packet;

                /* Memorize this packet at the beginning of the queue.  */
                asix -> ux_host_class_asix_receive_queue = packet;
            
                /* Reset the queue pointer of this packet.  */
                packet -> nx_packet_queue_next = UX_NULL;

                /* Ask USB to schedule a reception.  */
                status =  _ux_host_stack_transfer_request(transfer_request);

                /* Check if the transaction was armed successfully. We do not wait for the packet to be sent here. */
                if (status != UX_SUCCESS)
                {

                    /* Cancel the packet.  */
                    asix -> ux_host_class_asix_receive_queue = UX_NULL;
            
                }

            }

            /* Unprotect thread reentry to this instance.  */
            _ux_host_semaphore_put(&asix -> ux_host_class_asix_semaphore);

            return;
        }

        /* Check the link state. It is either pending up or down.  */
        if (asix -> ux_host_class_asix_link_state == UX_HOST_CLASS_ASIX_LINK_STATE_PENDING_DOWN)
        {
        
            /* We need to free the packets that will not be sent.  */
            current_packet =  asix -> ux_host_class_asix_xmit_queue;
    
            /* Get the next packet associated with the first packet.  */
            next_packet = current_packet -> nx_packet_queue_next;
            
            /* Parse all these packets that were scheduled.  */
            while (current_packet != UX_NULL)
            {
            
                /* Free the packet that was just sent.  First do some housekeeping.  */
                current_packet -> nx_packet_prepend_ptr =  current_packet -> nx_packet_prepend_ptr + UX_HOST_CLASS_ASIX_ETHERNET_SIZE; 
                current_packet -> nx_packet_length =  current_packet -> nx_packet_length - UX_HOST_CLASS_ASIX_ETHERNET_SIZE;

                /* And ask Netx to release it.  */
                nx_packet_transmit_release(current_packet); 
                
                /* Next packet becomes the current one.  */
                current_packet = next_packet;
                
                /* Next packet now.  */
                if (current_packet != UX_NULL)

                    /* Get the next packet associated with the first packet.  */
                    next_packet = current_packet -> nx_packet_queue_next;
            
            }
            

            /* Now the link is down.  */
            asix -> ux_host_class_asix_link_state = UX_HOST_CLASS_ASIX_LINK_STATE_DOWN;

            /* Communicate the state with the network driver.  */
            _ux_network_driver_link_down(asix -> ux_host_class_asix_network_handle);
            
            /* Unprotect thread reentry to this instance.  */
            _ux_host_semaphore_put(&asix -> ux_host_class_asix_semaphore);
            
            return;

        }
    }    
}

