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
/*    _ux_device_class_rndis_bulkout_thread               PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function is the thread of the rndis bulk out endpoint. It      */ 
/*    is waiting for the host to send data on the bulk out endpoint to    */ 
/*    the device.                                                         */ 
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
/*    _ux_network_driver_packet_received    Process received packet       */
/*    _ux_utility_long_get                  Get 32-bit value              */
/*    _ux_utility_thread_suspend            Suspend thread                */
/*    nx_packet_allocate                    Allocate NetX packet          */
/*    nx_packet_release                     Release NetX packet           */
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
/*                                            prefixed UX to MS_TO_TICK,  */
/*                                            verified memset and memcpy  */
/*                                            cases,                      */
/*                                            resulting in version 6.1    */
/*                                                                        */
/**************************************************************************/
VOID  _ux_device_class_rndis_bulkout_thread(ULONG rndis_class)
{

UX_SLAVE_CLASS                  *class;
UX_SLAVE_CLASS_RNDIS            *rndis;
UX_SLAVE_DEVICE                 *device;
UX_SLAVE_TRANSFER               *transfer_request;
UINT                            status;
NX_PACKET                       *packet;
ULONG                           ip_given_length;
ULONG                           packet_payload;

    /* Cast properly the rndis instance.  */
    UX_THREAD_EXTENSION_PTR_GET(class, UX_SLAVE_CLASS, rndis_class)
    
    /* Get the rndis instance from this class container.  */
    rndis =  (UX_SLAVE_CLASS_RNDIS *) class -> ux_slave_class_instance;
    
    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;
    
    /* This thread runs forever but can be suspended or resumed.  */
    while(1)
    {

        /* Select the transfer request associated with BULK OUT endpoint.   */
        transfer_request =  &rndis -> ux_slave_class_rndis_bulkout_endpoint -> ux_slave_endpoint_transfer_request;

        /* As long as the device is in the CONFIGURED state.  */
        while (device -> ux_slave_device_state == UX_DEVICE_CONFIGURED)
        { 
            
            /* We can accept new reception. Get a NX Packet.  */
            status =  nx_packet_allocate(&rndis -> ux_slave_class_rndis_packet_pool, &packet, 
                                         NX_RECEIVE_PACKET, UX_MS_TO_TICK(UX_DEVICE_CLASS_RNDIS_PACKET_POOL_WAIT));

            if (status == NX_SUCCESS)
            {

                /* And length.  */
                transfer_request -> ux_slave_transfer_request_requested_length =  UX_DEVICE_CLASS_RNDIS_MAX_PACKET_LENGTH;
                transfer_request -> ux_slave_transfer_request_actual_length =     0;
            
                /* Memorize this packet at the beginning of the queue.  */
                rndis -> ux_slave_class_rndis_receive_queue = packet;
            
                /* Reset the queue pointer of this packet.  */
                packet -> nx_packet_queue_next = UX_NULL;
                        
                /* Send the request to the device controller.  */
                status =  _ux_device_stack_transfer_request(transfer_request, UX_DEVICE_CLASS_RNDIS_MAX_PACKET_LENGTH + UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH,
                                                                UX_DEVICE_CLASS_RNDIS_MAX_PACKET_LENGTH + UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH);

                /* Check the completion code. */
                if (status == UX_SUCCESS)
                {

                    /* We only proceed with packets that are received OK, if error, ignore the packet. */
                    /* If trace is enabled, insert this event into the trace buffer.  */
                    UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_CLASS_RNDIS_PACKET_RECEIVE, rndis, 0, 0, 0, UX_TRACE_DEVICE_CLASS_EVENTS, 0, 0)

                    /* Check the state of the transfer.  If there is an error, we do not proceed with this report.  Also ensure the header has a valid ID of 1.  */
                    if (_ux_utility_long_get(transfer_request -> ux_slave_transfer_request_data_pointer + UX_DEVICE_CLASS_RNDIS_PACKET_MESSAGE_TYPE) == UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_MSG)
                    {

                        /* Ensure this packet is at least larger than the header.  */
                        if (transfer_request -> ux_slave_transfer_request_actual_length > UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH)
                        {
                
                            /* Get the size of the payload.  */
                            packet_payload =  _ux_utility_long_get(transfer_request -> ux_slave_transfer_request_data_pointer + UX_DEVICE_CLASS_RNDIS_PACKET_DATA_LENGTH);

                            /* Ensure the length reported in the RNDIS header is not larger than it actually is.
                               The reason we can't check to see if the length reported in the header and the
                               actual length are exactly equal is because there might other data after the payload 
                               (padding, or even a message). */
                            if (packet_payload <= transfer_request -> ux_slave_transfer_request_actual_length - UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH)
                            {

                                /* Adjust the prepend pointer to take into account the non 3 bit alignment of the ethernet header.  */
                                packet -> nx_packet_prepend_ptr += sizeof(USHORT);
                    
                                /* Adjust the prepend, length, and append fields.  */ 
                                packet -> nx_packet_length = packet_payload;
                                packet -> nx_packet_append_ptr = packet -> nx_packet_prepend_ptr + packet_payload;
                                

                                /* Copy the received packet in the IP packet data area.  */
                                _ux_utility_memory_copy(packet -> nx_packet_prepend_ptr, transfer_request -> ux_slave_transfer_request_data_pointer + UX_DEVICE_CLASS_RNDIS_PACKET_BUFFER, packet_payload); /* Use case of memcpy is verified. */
                        
                                /* Calculate the accurate packet length from ip header */ 
                                if((*(packet -> nx_packet_prepend_ptr + 12) == 0x08) && 
                                   (*(packet -> nx_packet_prepend_ptr + 13) == 0))
                                {

                                    ip_given_length = _ux_utility_short_get_big_endian(packet -> nx_packet_prepend_ptr + 16) + UX_DEVICE_CLASS_RNDIS_ETHERNET_SIZE;
                                    packet -> nx_packet_length =  ip_given_length ;
                                    packet -> nx_packet_append_ptr =  packet -> nx_packet_prepend_ptr + ip_given_length;
                                }

                                /* Send that packet to the NetX USB broker.  */
                                _ux_network_driver_packet_received(rndis -> ux_slave_class_rndis_network_handle, packet);
                            }
                            else

                                /* We received a malformed packet. Report to application.  */
                                _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_CLASS_MALFORMED_PACKET_RECEIVED_ERROR);
                        }
                        else

                            /* We received a malformed packet. Report to application.  */
                            _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_CLASS_MALFORMED_PACKET_RECEIVED_ERROR);
                    }
                }
                else

                    /* Free the packet that was not successfully received.  */
                    nx_packet_release(packet);
            }
            else
            {

                /* Packet allocation timed out. Note that the timeout value is
                   configurable.  */

                /* Error trap. No need for trace, since NetX does it.  */
                _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_MEMORY_INSUFFICIENT);
            }
        }
             
        /* We need to suspend ourselves. We will be resumed by the device enumeration module.  */
        _ux_utility_thread_suspend(&rndis -> ux_slave_class_rndis_bulkout_thread);
    }
}

