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
/**   Device RNDIS Class                                                  */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_rndis.h"
#include "ux_device_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_device_class_rndis_bulkin_thread                PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function is the thread of the rndis bulkin endpoint. The bulk  */ 
/*    IN endpoint is used when the device wants to write data to be sent  */ 
/*    to the host.                                                        */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    rndis_class                             Address of rndis class      */ 
/*                                                container               */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    None                                                                */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_device_stack_transfer_request     Request transfer              */ 
/*    _ux_utility_event_flags_get           Get event flags               */
/*    _ux_utility_mutex_on                  Take mutex                    */
/*    _ux_utility_mutex_off                 Release mutex                 */
/*    _ux_utility_long_put                  Put 32-bit value              */
/*    nx_packet_transmit_release            Release NetX packet           */
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    ThreadX                                                             */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            verified memset and memcpy  */
/*                                            cases, used UX prefix to    */
/*                                            refer to TX symbols instead */
/*                                            of using them directly,     */
/*                                            resulting in version 6.1    */
/*                                                                        */
/**************************************************************************/
VOID  _ux_device_class_rndis_bulkin_thread(ULONG rndis_class)
{

UX_SLAVE_CLASS                  *class;
UX_SLAVE_CLASS_RNDIS            *rndis;
UX_SLAVE_DEVICE                 *device;
UX_SLAVE_TRANSFER               *transfer_request;
UINT                            status;
ULONG                           actual_flags;
NX_PACKET                       *current_packet;
UCHAR                           *packet_header;
ULONG                           transfer_length;

    /* Cast properly the rndis instance.  */
    UX_THREAD_EXTENSION_PTR_GET(class, UX_SLAVE_CLASS, rndis_class)
    
    /* Get the rndis instance from this class container.  */
    rndis =  (UX_SLAVE_CLASS_RNDIS *) class -> ux_slave_class_instance;
    
    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;
    
    /* This thread runs forever but can be suspended or resumed.  */
    while(1)
    {

        /* Get the transfer request for the bulk IN pip.  */
        transfer_request =  &rndis -> ux_slave_class_rndis_bulkin_endpoint -> ux_slave_endpoint_transfer_request;

        /* As long as the device is in the CONFIGURED state.  */
        while (device -> ux_slave_device_state == UX_DEVICE_CONFIGURED)
        { 

            /* Wait until we have a event sent by the application. We do not treat yet the case where a timeout based
               on the interrupt pipe frequency or a change in the idle state forces us to send an empty report.  */
            status =  _ux_utility_event_flags_get(&rndis -> ux_slave_class_rndis_event_flags_group, (UX_DEVICE_CLASS_RNDIS_NEW_BULKIN_EVENT |
                                                                                            UX_DEVICE_CLASS_RNDIS_NEW_DEVICE_STATE_CHANGE_EVENT), 
                                                                                            UX_OR_CLEAR, &actual_flags, UX_WAIT_FOREVER);
                                                                                            
            /* Check the completion code and the actual flags returned. */
            if (status == UX_SUCCESS && (actual_flags & UX_DEVICE_CLASS_RNDIS_NEW_DEVICE_STATE_CHANGE_EVENT) == 0)
            {
                
                /* Parse all packets.  */
                while(rndis -> ux_slave_class_rndis_xmit_queue != UX_NULL)
                {

                    /* Protect this thread.  */
                    _ux_utility_mutex_on(&rndis -> ux_slave_class_rndis_mutex);
                
                    /* Get the current packet in the list.  */
                    current_packet =  rndis -> ux_slave_class_rndis_xmit_queue;
                    
                    /* Set the next packet (or a NULL value) as the head of the xmit queue. */
                    rndis -> ux_slave_class_rndis_xmit_queue =  current_packet -> nx_packet_queue_next;
                    
                    /* Free Mutex resource.  */
                    _ux_utility_mutex_off(&rndis -> ux_slave_class_rndis_mutex);
                        
                    /* If the link is down no need to rearm a packet. */
                    if (rndis -> ux_slave_class_rndis_link_state == UX_DEVICE_CLASS_RNDIS_LINK_STATE_UP)
                    {
                
                        /* Load the address of the current packet header at the physical header.  */
                        packet_header =  current_packet -> nx_packet_prepend_ptr;
        
                        /* Calculate the transfer length.  */
                        transfer_length =  current_packet -> nx_packet_length + UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH;

                        /* Is there enough space for this packet in the transfer buffer?  */
                        if (transfer_length <= UX_SLAVE_REQUEST_DATA_MAX_LENGTH)
                        {

                            /* Copy the packet in the transfer descriptor buffer.  */
                            _ux_utility_memory_copy (transfer_request -> ux_slave_transfer_request_data_pointer + UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH, packet_header, current_packet -> nx_packet_length); /* Use case of memcpy is verified. */
                            
                            /* Add the RNDIS header to this packet.  */
                            _ux_utility_long_put(transfer_request -> ux_slave_transfer_request_data_pointer + UX_DEVICE_CLASS_RNDIS_PACKET_MESSAGE_TYPE, UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_MSG);
                            _ux_utility_long_put(transfer_request -> ux_slave_transfer_request_data_pointer + UX_DEVICE_CLASS_RNDIS_PACKET_MESSAGE_LENGTH, transfer_length);
                            _ux_utility_long_put(transfer_request -> ux_slave_transfer_request_data_pointer + UX_DEVICE_CLASS_RNDIS_PACKET_DATA_OFFSET, 
                                                    UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH - UX_DEVICE_CLASS_RNDIS_PACKET_DATA_OFFSET);
                            _ux_utility_long_put(transfer_request -> ux_slave_transfer_request_data_pointer + UX_DEVICE_CLASS_RNDIS_PACKET_DATA_LENGTH, current_packet -> nx_packet_length);
                                                    
                            /* If trace is enabled, insert this event into the trace buffer.  */
                            UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_CLASS_RNDIS_PACKET_TRANSMIT, rndis, 0, 0, 0, UX_TRACE_DEVICE_CLASS_EVENTS, 0, 0)

                            /* Send the request to the device controller.  */
                            status =  _ux_device_stack_transfer_request(transfer_request, transfer_length, UX_DEVICE_CLASS_RNDIS_ETHERNET_PACKET_SIZE);
                            
                            /* Check for error. */
                            if (status != UX_SUCCESS)
                            
                                /* Error trap. */
                                _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, status);
                        }
                        else
                        {

                            /* No, there is not enough space.  */

                            /* Report error to application. */
                            _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_MEMORY_INSUFFICIENT);
                        }
                    }        
               
                    /* Free the packet that was just sent.  First do some housekeeping.  */
                    current_packet -> nx_packet_prepend_ptr =  current_packet -> nx_packet_prepend_ptr + UX_DEVICE_CLASS_RNDIS_ETHERNET_SIZE; 
                    current_packet -> nx_packet_length =  current_packet -> nx_packet_length - UX_DEVICE_CLASS_RNDIS_ETHERNET_SIZE;
                
                    /* And ask Netx to release it.  */
                    nx_packet_transmit_release(current_packet); 
                }
            }
            else
            {

                /* We get here when the link is down or the last transmission failed. All packets pending must be freed.  */
                while(rndis -> ux_slave_class_rndis_xmit_queue != UX_NULL)
                {

                    /* Protect the chain of packets.  */
                    _ux_utility_mutex_on(&rndis -> ux_slave_class_rndis_mutex);
                
                    /* Get the current packet in the list.  */
                    current_packet =  rndis -> ux_slave_class_rndis_xmit_queue;
                    
                    /* Set the next packet (or a NULL value) as the head of the xmit queue. */
                    rndis -> ux_slave_class_rndis_xmit_queue =  current_packet -> nx_packet_queue_next;
                    
                    /* Free Mutex resource.  */
                    _ux_utility_mutex_off(&rndis -> ux_slave_class_rndis_mutex);
                    
                    /* Free the packet */
                    current_packet -> nx_packet_prepend_ptr =  current_packet -> nx_packet_prepend_ptr + UX_DEVICE_CLASS_RNDIS_ETHERNET_SIZE; 
                    current_packet -> nx_packet_length =  current_packet -> nx_packet_length - UX_DEVICE_CLASS_RNDIS_ETHERNET_SIZE;
                
                    /* And ask Netx to release it.  */
                    nx_packet_transmit_release(current_packet); 
                }
            }
        }
       
        /* We need to suspend ourselves. We will be resumed by the device enumeration module.  */
        _ux_utility_thread_suspend(&rndis -> ux_slave_class_rndis_bulkin_thread);
    }
}

