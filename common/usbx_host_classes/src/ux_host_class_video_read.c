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
/**   Video Class                                                         */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_video.h"
#include "ux_host_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_video_read                           PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function reads from the video streaming interface.             */ 
/*                                                                        */
/*    Note if the transfer request is not linked (next pointer is NULL),  */
/*    a single request is performed. If the transfer request links into a */
/*    list, the whole list is added.                                      */
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    video                                 Pointer to video class        */ 
/*    video_transfer_request                Pointer to transfer request   */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_class_video_transfer_request Video transfer request        */ 
/*    _ux_host_stack_class_instance_verify  Verify instance is valid      */ 
/*    _ux_utility_semaphore_get             Get semaphore                 */ 
/*    _ux_utility_semaphore_put             Put semaphore                 */ 
/*    _ux_system_error_handler              System error log              */
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Application                                                         */ 
/*    Video Class                                                         */ 
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
UINT  _ux_host_class_video_read(UX_HOST_CLASS_VIDEO *video, UX_HOST_CLASS_VIDEO_TRANSFER_REQUEST *video_transfer_request)
{

UINT                                    status;
ULONG                                   packet_size;
UX_HOST_CLASS_VIDEO_TRANSFER_REQUEST    *transfer_list;


    /* Ensure the instance is valid.  */
    if (_ux_host_stack_class_instance_verify(_ux_system_host_class_video_name, (VOID *) video) != UX_SUCCESS)
    {        

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_INSTANCE_UNKNOWN);

        return(UX_HOST_CLASS_INSTANCE_UNKNOWN);
    }

    /* Protect thread reentry to this instance.  */
    status =  _ux_utility_semaphore_get(&video -> ux_host_class_video_semaphore, UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
        return(status);

    /* By default the transmission will be successful.  */
    status =  UX_SUCCESS;
    
    /* Ensure we have a selected interface that allows isoch transmission.  */
    if ((video -> ux_host_class_video_isochronous_endpoint == UX_NULL) ||
        (video -> ux_host_class_video_isochronous_endpoint -> ux_endpoint_descriptor.wMaxPacketSize == 0))
    {

        /* Unprotect thread reentry to this instance.  */
        status =  _ux_utility_semaphore_put(&video -> ux_host_class_video_semaphore);

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_VIDEO_WRONG_INTERFACE);

        /* Return error status.  */
        return(UX_HOST_CLASS_VIDEO_WRONG_INTERFACE);
    }

    /* Calculate max transfer size on the endpoint.  */
    packet_size = video -> ux_host_class_video_isochronous_endpoint -> ux_endpoint_descriptor.wMaxPacketSize;
    if (packet_size & UX_MAX_NUMBER_OF_TRANSACTIONS_MASK)
        packet_size = (packet_size & UX_MAX_PACKET_SIZE_MASK) * (((packet_size & UX_MAX_NUMBER_OF_TRANSACTIONS_MASK) >> UX_MAX_NUMBER_OF_TRANSACTIONS_SHIFT) + 1);

    /* Save list head.  */
    transfer_list = video_transfer_request;

    /* Handle the request (list).  */
    while(video_transfer_request)
    {

        /* For video in, we read packets at a time, so the transfer request size is the size of the
           endpoint.  */
        video_transfer_request -> ux_host_class_video_transfer_request_requested_length = packet_size;

        /* Next request.  */
        video_transfer_request = video_transfer_request -> ux_host_class_video_transfer_request_next_video_transfer_request;

    }

    /* Ask the stack to hook this transfer request to the iso ED.  */
    status =  _ux_host_class_video_transfer_request(video, transfer_list);

    /* Unprotect thread reentry to this instance.  */
    _ux_utility_semaphore_put(&video -> ux_host_class_video_semaphore);

    /* Return completion status.  */
    return(status);
}

