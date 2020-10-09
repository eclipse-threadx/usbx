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
/**   Audio Class                                                         */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_audio.h"
#include "ux_host_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_audio_transfer_request               PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function submits an isochronous audio transfer request to the  */ 
/*    USBX stack.                                                         */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    audio                                 Pointer to audio class        */ 
/*    audio_transfer_request                Pointer to transfer request   */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_stack_transfer_request       Process transfer request      */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Audio Class                                                         */ 
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
UINT  _ux_host_class_audio_transfer_request(UX_HOST_CLASS_AUDIO *audio,
                            UX_HOST_CLASS_AUDIO_TRANSFER_REQUEST *audio_transfer_request)
{

UINT            status;
UX_TRANSFER     *transfer_request;
    

    /* The transfer request is embedded in the application transfer request.  */
    transfer_request =  &audio_transfer_request -> ux_host_class_audio_transfer_request;

    /* Select the direction. We do this by taking the endpoint direction.  */
    transfer_request -> ux_transfer_request_type =  audio -> ux_host_class_audio_isochronous_endpoint -> 
                                                            ux_endpoint_descriptor.bEndpointAddress&UX_REQUEST_DIRECTION;

    /* Fill the transfer request with all the required fields.  */
    transfer_request -> ux_transfer_request_endpoint =             audio -> ux_host_class_audio_isochronous_endpoint;
    transfer_request -> ux_transfer_request_data_pointer =         audio_transfer_request -> ux_host_class_audio_transfer_request_data_pointer;
    transfer_request -> ux_transfer_request_requested_length =     audio_transfer_request -> ux_host_class_audio_transfer_request_requested_length;
    transfer_request -> ux_transfer_request_completion_function =  _ux_host_class_audio_transfer_request_completed;
    transfer_request -> ux_transfer_request_class_instance =       audio;

    /* We memorize the application transfer request in the local transfer request.  */
    transfer_request -> ux_transfer_request_user_specific =  (VOID *) audio_transfer_request;

    /* Transfer the transfer request.  */
    status =  _ux_host_stack_transfer_request(transfer_request);

    /* Return completion status.  */
    return(status);
}

