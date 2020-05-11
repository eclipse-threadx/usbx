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
/**   Device Storage Class                                                */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_storage.h"
#include "ux_device_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_device_class_storage_get_status_notification    PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function performs a GET_STATUS_NOTIFICATION command.           */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    storage                               Pointer to storage class      */ 
/*    endpoint_in                           Pointer to IN endpoint        */
/*    endpoint_out                          Pointer to OUT endpoint       */
/*    cbwcb                                 Pointer to CBWCB              */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_device_class_storage_csw_send     Send CSW                      */ 
/*    _ux_device_stack_transfer_request     Transfer request              */ 
/*    _ux_device_stack_endpoint_stall       Stall endpoint                */
/*    _ux_utility_short_put_big_endian      Put 16-bit big endian         */
/*    _ux_utility_memory_copy               Copy memory                   */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Device Storage Class                                                */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_storage_get_status_notification(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun,
                                            UX_SLAVE_ENDPOINT *endpoint_in,
                                            UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb)
{

UINT                    status;
UX_SLAVE_TRANSFER       *transfer_request;
UCHAR                   *media_notification;
ULONG                   media_notification_length;
ULONG                   notification_class;

    UX_PARAMETER_NOT_USED(endpoint_out);

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_CLASS_STORAGE_READ_CAPACITY, storage, lun, 0, 0, UX_TRACE_DEVICE_CLASS_EVENTS, 0, 0)

    /* Ensure the callback has been initialized.  */
    if (storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_notification == UX_NULL)
    {

        /* We need to STALL the IN endpoint.  The endpoint will be reset by the host.  */
        _ux_device_stack_endpoint_stall(endpoint_in);

        /* Now we return a CSW with Error.  */
        _ux_device_class_storage_csw_send(storage, lun, endpoint_in, UX_SLAVE_CLASS_STORAGE_CSW_FAILED);

        /* Return error.  */
        return(UX_FUNCTION_NOT_SUPPORTED);
    }

    /* Extract the notification from the cbwcb.  */
    notification_class = (ULONG) *(cbwcb + UX_SLAVE_CLASS_STORAGE_EVENT_NOTIFICATION_CLASS_REQUEST);
    
    /* Obtain the notification of the device.  */
    status =  storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_notification(storage, lun, 
                                storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_id,
                                notification_class, 
                                &media_notification, 
                                &media_notification_length);

    /* Check the status for error.  */
    if (status != UX_SUCCESS)
    {
        
        /* We need to STALL the IN endpoint.  The endpoint will be reset by the host.  */
        _ux_device_stack_endpoint_stall(endpoint_in);

        /* Now we return a CSW with Error.  */
        _ux_device_class_storage_csw_send(storage, lun, endpoint_in, UX_SLAVE_CLASS_STORAGE_CSW_FAILED);
    }    
    else
    {

        /* Obtain the pointer to the transfer request.  */
        transfer_request =  &endpoint_in -> ux_slave_endpoint_transfer_request;
        
        /* Put the length of the notification length in the buffer.  */
        _ux_utility_short_put_big_endian(transfer_request -> ux_slave_transfer_request_data_pointer, (USHORT)media_notification_length);

        /* Copy the CSW into the transfer request memory.  */
        _ux_utility_memory_copy(transfer_request -> ux_slave_transfer_request_data_pointer + sizeof (USHORT), 
                                            media_notification, 
                                            media_notification_length);
        
        /* Update the notification length. */
        media_notification_length += (ULONG)sizeof (USHORT);

        /* Send a data payload with the notification buffer.  */
        _ux_device_stack_transfer_request(transfer_request, 
                                  media_notification_length,
                                  media_notification_length);
        
        /* Now we return a CSW with success.  */
        status =  _ux_device_class_storage_csw_send(storage, lun, endpoint_in, UX_SLAVE_CLASS_STORAGE_CSW_PASSED);
    }
        
    /* Return completion status.  */
    return(status);
}

