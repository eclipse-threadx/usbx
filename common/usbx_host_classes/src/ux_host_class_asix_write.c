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


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_asix_write                           PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function writes to the asix interface. The call is blocking    */ 
/*    and only returns when there is either an error or when the transfer */ 
/*    is complete.                                                        */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    asix                                  Pointer to asix class         */ 
/*    packet                                Packet to write or queue      */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_stack_transfer_request       Process transfer request      */ 
/*    _ux_utility_short_put                 Put 16-bit value              */
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
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_asix_write(VOID *asix_class, NX_PACKET *packet)
{

UX_TRANSFER         *transfer_request;
UINT                status;
NX_PACKET           *current_packet;
NX_PACKET           *next_packet;
UCHAR               *packet_header;
UX_HOST_CLASS_ASIX  *asix;

    /* Proper class casting.  */
    asix = (UX_HOST_CLASS_ASIX *) asix_class;

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_ASIX_WRITE, asix, 0, 0, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

    /* Ensure the instance is valid.  */
    if (asix -> ux_host_class_asix_state !=  UX_HOST_CLASS_INSTANCE_LIVE)
    {        

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_INSTANCE_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_HOST_CLASS_INSTANCE_UNKNOWN, asix, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_HOST_CLASS_INSTANCE_UNKNOWN);
    }

    /* Load the address of the current packet header at the physical header.  */
    packet_header =  packet -> nx_packet_prepend_ptr;
    
    /* Substract 2 USHORT to store length of the packet.  */
    packet_header -= sizeof(USHORT) * 2;

#if defined(UX_HOST_CLASS_ASIX_HEADER_CHECK_ENABLE)
    /* Check packet compatibility to avoid writing to unexpected area.  */
    if (packet_header < packet -> nx_packet_data_start)
    {

        /* Error trap.  */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_MEMORY_ERROR);
        return(UX_HOST_CLASS_MEMORY_ERROR);
    }
#endif

    /* Store the length of the payload in the first USHORT.  */
    _ux_utility_short_put(packet_header, (USHORT)(packet -> nx_packet_length));
    
    /* Store the negative length of the payload in the first USHORT.  */
    _ux_utility_short_put(packet_header + sizeof(USHORT), (USHORT)(~packet -> nx_packet_length));

    /* Check the queue. See if there is something that is being sent. */
    if (asix -> ux_host_class_asix_xmit_queue == UX_NULL)
    {

        /* Nothing is in the queue. We need to arm this transfer. */
        /* Get the pointer to the bulk out endpoint transfer request.  */
        transfer_request =  &asix -> ux_host_class_asix_bulk_out_endpoint -> ux_endpoint_transfer_request;

        /* Setup the transaction parameters.  */
        transfer_request -> ux_transfer_request_data_pointer     =  packet_header;
        transfer_request -> ux_transfer_request_requested_length =  packet -> nx_packet_length + (ULONG)sizeof(USHORT) * 2;
        
        /* Store the packet that owns this transaction.  */
        transfer_request -> ux_transfer_request_user_specific = packet;

        /* Perform the transfer.  */
        status =  _ux_host_stack_transfer_request(transfer_request);

        /* Check if the transaction was armed successfully. We do not wait for the packet to be sent here. */
        if (status == UX_SUCCESS)
        {
            
            /* Memorize this packet at the beginning of the queue.  */
            asix -> ux_host_class_asix_xmit_queue = packet;
            
            /* Reset the queue pointer of this packet.  */
            packet -> nx_packet_queue_next = UX_NULL;

        }
        
        else

            /* Could not arm this transfer.  */
            return(UX_ERROR);

    }

    else
    
    {
    
        /* We get here when there is something in the queue.  */
        current_packet =  asix -> ux_host_class_asix_xmit_queue;

        /* Get the next packet associated with the first packet.  */
        next_packet = current_packet -> nx_packet_queue_next;

        /* Parse the current chain for the end.  */
        while (next_packet != NX_NULL)
        {
            /* Remember the current packet.  */
            current_packet = next_packet;
            
            /* See what the next packet in the chain is.  */
            next_packet = current_packet -> nx_packet_queue_next;
        }

        /* Memorize the packet to be sent.  */
        current_packet -> nx_packet_queue_next = packet;

        /* The packet to be sent is the last in the chain.  */
        packet -> nx_packet_queue_next = NX_NULL;

    }

    /* We are done here.  */
    return(UX_SUCCESS);            

}

