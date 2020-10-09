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
/**   Device DFU Class                                                    */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_dfu.h"
#include "ux_device_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_device_class_dfu_control_request                PORTABLE C      */ 
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
/*    dfu                                       Pointer to dfu class      */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    None                                                                */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_device_stack_endpoint_stall       Endpoint stall                */ 
/*    _ux_device_stack_transfer_request     Transfer request              */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    DFU Class                                                           */
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
UINT  _ux_device_class_dfu_control_request(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_TRANSFER       *transfer_request;
UX_SLAVE_DEVICE         *device;
UX_SLAVE_CLASS          *class;
UX_SLAVE_CLASS_DFU      *dfu;

ULONG                   request;
ULONG                   request_length;
ULONG                   actual_length;
ULONG                   media_status;


    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

     /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;
    
    /* Get the storage instance from this class container.  */
    dfu =  (UX_SLAVE_CLASS_DFU *) class -> ux_slave_class_instance;

    /* Get the pointer to the transfer request associated with the control endpoint.  */
    transfer_request =  &device -> ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

    /* Extract all necessary fields of the request.  */
    request =  *(transfer_request -> ux_slave_transfer_request_setup + UX_SETUP_REQUEST);

    /* Pickup the request length.  */
    request_length =   _ux_utility_short_get(transfer_request -> ux_slave_transfer_request_setup + UX_SETUP_LENGTH);

    /* What state are we in ? */
    switch (_ux_system_slave -> ux_system_slave_device_dfu_state_machine) 
    {
    
        case UX_SYSTEM_DFU_STATE_APP_IDLE         :


            /* Here we process only the request we can accept in the APP IDLE state.  */
            switch (request)
            {
        
                case UX_SLAVE_CLASS_DFU_COMMAND_DETACH :
                
                    /* The host is asking for a Detach and switch to the DFU mode. Either we force the reset here or
                       we wait for a specified timer. If there is no reset while this timer is running we abandon 
                       the DFU Detach.*/
                    if (_ux_system_slave -> ux_system_slave_device_dfu_capabilities &  UX_SLAVE_CLASS_DFU_CAPABILITY_WILL_DETACH)
                    {
        
                        /* Wake up the DFU thread and send a detach request..  */
                        _ux_utility_event_flags_set(&dfu -> ux_slave_class_dfu_event_flags_group, UX_DEVICE_CLASS_DFU_THREAD_EVENT_DISCONNECT, UX_OR);                

                    }
                    else
                    {

                        /* We expect the host to issue a reset.  Arm a timer in the DFU thread.  */
                        _ux_utility_event_flags_set(&dfu -> ux_slave_class_dfu_event_flags_group, UX_DEVICE_CLASS_DFU_THREAD_EVENT_WAIT_RESET, UX_OR);                

                    }

                    /* We can switch dfu state machine.  */
                    _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SYSTEM_DFU_STATE_APP_DETACH;
        
                    break;

                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATUS :
                
                    /* Fill the status data payload.  First with status.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) dfu -> ux_slave_class_dfu_status; 

                    /* Poll time out value is set to 1ms.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT) = 1;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 1) = 0;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 2) = 0;

                    /* Next state.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STATE) = (UCHAR) _ux_system_slave -> ux_system_slave_device_dfu_state_machine;

                    /* String index set to 0.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STRING) = 0;
                    
                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH);
                    
                    break;

                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATE :
                
                    /* Fill the status data payload.  First with state.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) _ux_system_slave -> ux_system_slave_device_dfu_state_machine; 

                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH);

                    break;

                default:
        
                    /* Unknown function. Stall the endpoint.  */
                    _ux_device_stack_endpoint_stall(&device -> ux_slave_device_control_endpoint);
                    break;
            }

            break;
            
        case UX_SYSTEM_DFU_STATE_APP_DETACH         :

            /* Here we process only the request we can accept in the APP DETACH state.  */
            switch (request)
            {
        
                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATUS :
                
                    /* Fill the status data payload.  First with status.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) dfu -> ux_slave_class_dfu_status; 

                    /* Poll time out value is set to 1ms.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT) = 1;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 1) = 0;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 2) = 0;

                    /* Next state.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STATE) = (UCHAR) _ux_system_slave -> ux_system_slave_device_dfu_state_machine;

                    /* String index set to 0.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STRING) = 0;
                    
                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH);
                    
                    break;

                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATE :
                
                    /* Fill the status data payload.  First with state.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) _ux_system_slave -> ux_system_slave_device_dfu_state_machine; 

                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH);
                    
                    break;

                default:
        
                    /* Unknown function. Stall the endpoint.  */
                    _ux_device_stack_endpoint_stall(&device -> ux_slave_device_control_endpoint);
                    break;
            }

            break;
            
        case UX_SYSTEM_DFU_STATE_DFU_IDLE         :

            /* Here we process only the request we can accept in the DFU mode IDLE state.  */
            switch (request)
            {
        
                case UX_SLAVE_CLASS_DFU_COMMAND_DOWNLOAD :

                    /* We received a DOWNLOAD command. Check the length field of the request. It cannot be 0.  */
                    if (request_length == 0)
                    {

                        /* Zero length download is not accepted. Stall the endpoint.  */
                        _ux_device_stack_endpoint_stall(&device -> ux_slave_device_control_endpoint);

                        /* In the system, state the DFU state machine to dfu ERROR.  */
                        _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SYSTEM_DFU_STATE_DFU_ERROR;

                    }
                    else
                    {

                        /* Have we declared a DOWNLOAD possible in our device framework ? */
                        if (_ux_system_slave -> ux_system_slave_device_dfu_capabilities & UX_SLAVE_CLASS_DFU_CAPABILITY_CAN_DOWNLOAD)
                        {
    
                            /* Send a notification to the application.  Begin of transfer.  */
                            dfu -> ux_slave_class_dfu_notify(dfu, UX_SLAVE_CLASS_DFU_NOTIFICATION_BEGIN_DOWNLOAD);
                        
                            /* Write the first block to the firmware.  */
                            dfu -> ux_slave_class_dfu_write(dfu, dfu -> ux_slave_class_dfu_download_block_count, 
                                                                transfer_request -> ux_slave_transfer_request_data_pointer,
                                                                request_length, 
                                                                &actual_length);
    
                            /* In the system, state the DFU state machine to dfu DOWNLOAD SYNC.  */
                            _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SYSTEM_DFU_STATE_DFU_DNLOAD_SYNC;
    
                            /* Increase the block count.  */
                            dfu -> ux_slave_class_dfu_download_block_count++;
    
                        }
                        else
                        {
    
                            /* Download is not accepted. Stall the endpoint.  */
                            _ux_device_stack_endpoint_stall(&device -> ux_slave_device_control_endpoint);
    
                            /* In the system, state the DFU state machine to dfu ERROR.  */
                            _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SYSTEM_DFU_STATE_DFU_ERROR;
    
                        }

                    }
                    break;
                    
                case UX_SLAVE_CLASS_DFU_COMMAND_ABORT :

                        /* Send a notification to the application.  */
                        dfu -> ux_slave_class_dfu_notify(dfu, UX_SLAVE_CLASS_DFU_NOTIFICATION_ABORT_DOWNLOAD);
                    
                        /* In the system, state the DFU state machine to dfu IDLE.  */
                        _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SYSTEM_DFU_STATE_DFU_IDLE;
                
                        /* Reset the download/upload parameters.  */
                        dfu -> ux_slave_class_dfu_download_block_count = 0;
                        dfu -> ux_slave_class_dfu_upload_block_count = 0;
                
                    break;

                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATUS :
                
                    /* Fill the status data payload.  First with status.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) dfu -> ux_slave_class_dfu_status; 

                    /* Poll time out value is set to 1ms.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT) = 1;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 1) = 0;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 2) = 0;

                    /* Next state.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STATE) = (UCHAR) UX_SYSTEM_DFU_STATE_DFU_IDLE;

                    /* String index set to 0.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STRING) = 0;
                    
                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH);
                    
                    break;
                    
                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATE :
                
                    /* Fill the status data payload.  First with state.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) _ux_system_slave -> ux_system_slave_device_dfu_state_machine; 

                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH);
                    
                    break;
                    
                default:
        
                    /* Unknown function. Stall the endpoint.  */
                    _ux_device_stack_endpoint_stall(&device -> ux_slave_device_control_endpoint);

                    /* In the system, state the DFU state machine to dfu ERROR.  */
                       _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SYSTEM_DFU_STATE_DFU_ERROR;

                    break;
            }

            break;
            

        case UX_SYSTEM_DFU_STATE_DFU_DNLOAD_SYNC         :

            /* Here we process only the request we can accept in the DFU mode DOWNLOAD state.  */
            switch (request)
            {
        
                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATUS :

                    /* Check if the device is still buys performing the write. Write could be delayed.  */
                    dfu -> ux_slave_class_dfu_get_status(dfu, &media_status);
                    
                    /* Check status of device.  */
                    switch (media_status)
                    {

                        case     UX_SLAVE_CLASS_DFU_MEDIA_STATUS_OK        :

                            /* Set the next state for idle and no error status.  */
                            dfu -> ux_slave_class_dfu_status = UX_SLAVE_CLASS_DFU_STATUS_OK ;
                               _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SLAVE_CLASS_DFU_STATUS_STATE_DFU_DNLOAD_IDLE;
                            break;
                        
                        case     UX_SLAVE_CLASS_DFU_MEDIA_STATUS_BUSY    :
                        
                            /* Set the next state for busy but no error status.  */
                            dfu -> ux_slave_class_dfu_status = UX_SLAVE_CLASS_DFU_STATUS_OK ;
                               _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SLAVE_CLASS_DFU_STATUS_STATE_DFU_DNBUSY;
                            break;
                            
                        case    UX_SLAVE_CLASS_DFU_MEDIA_STATUS_ERROR   :

                            /* Set the next state for busy and error status.  */
                            dfu -> ux_slave_class_dfu_status = UX_SLAVE_CLASS_DFU_STATUS_ERROR_WRITE ;
                               _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SLAVE_CLASS_DFU_STATUS_STATE_DFU_ERROR;
                            break;

                    }

                    /* Fill the status data payload.  First with status.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) dfu -> ux_slave_class_dfu_status; 

                    /* Poll time out value is set to 1ms.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT) = 1;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 1) = 0;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 2) = 0;

                    /* Next state.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STATE) = (UCHAR) _ux_system_slave -> ux_system_slave_device_dfu_state_machine;

                    /* String index set to 0.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STRING) = 0;
                    
                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH);
                    
                    break;
                    
                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATE :
                
                    /* Fill the status data payload.  First with state.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) _ux_system_slave -> ux_system_slave_device_dfu_state_machine; 

                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH);
                    
                    break;
                    
                default:
        
                    /* Unknown function. Stall the endpoint.  */
                    _ux_device_stack_endpoint_stall(&device -> ux_slave_device_control_endpoint);

                    /* In the system, state the DFU state machine to dfu ERROR.  */
                       _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SYSTEM_DFU_STATE_DFU_ERROR;

                    break;
            }

            break;

        case UX_SLAVE_CLASS_DFU_STATUS_STATE_DFU_DNLOAD_IDLE         :

            /* Here we process only the request we can accept in the DFU mode DNLOAD state.  */
            switch (request)
            {
        
                case UX_SLAVE_CLASS_DFU_COMMAND_DOWNLOAD :

                    /* We received a DOWNLOAD command. Check the length field of the request. If it is 0,
                       we are done with the transfer.  */
                    if (request_length == 0)
                    {

                        /* Send the notification of end of download to application.  */
                        dfu -> ux_slave_class_dfu_notify(dfu, UX_SLAVE_CLASS_DFU_NOTIFICATION_END_DOWNLOAD);
                        
                        /* In the system, state the DFU state machine to DFU MANIFEST SYNCH.  */
                        _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SLAVE_CLASS_DFU_STATUS_STATE_DFU_MANIFEST_SYNC;

                    }

                    else
                    {
                        
                        /* Write the next block to the firmware.  */
                        dfu -> ux_slave_class_dfu_write(dfu, dfu -> ux_slave_class_dfu_download_block_count, 
                                                            transfer_request -> ux_slave_transfer_request_data_pointer,
                                                            request_length, 
                                                            &actual_length);

                        /* In the system, state the DFU state machine to dfu DOWNLOAD SYNC.  */
                        _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SYSTEM_DFU_STATE_DFU_DNLOAD_SYNC;

                        /* Increase the block count.  */
                        dfu -> ux_slave_class_dfu_download_block_count++;

                    }

                    break;
                    
                case UX_SLAVE_CLASS_DFU_COMMAND_ABORT :

                        /* Send a notification to the application.  */
                        dfu -> ux_slave_class_dfu_notify(dfu, UX_SLAVE_CLASS_DFU_NOTIFICATION_ABORT_DOWNLOAD);
                    
                        /* In the system, state the DFU state machine to dfu IDLE.  */
                        _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SYSTEM_DFU_STATE_DFU_IDLE;

                        /* Reset the download/upload parameters.  */
                        dfu -> ux_slave_class_dfu_download_block_count = 0;
                        dfu -> ux_slave_class_dfu_upload_block_count = 0;
                
                    break;

                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATUS :
                
                    /* Fill the status data payload.  First with status.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) dfu -> ux_slave_class_dfu_status; 

                    /* Poll time out value is set to 1ms.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT) = 1;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 1) = 0;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 2) = 0;

                    /* Next state.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STATE) = (UCHAR) _ux_system_slave -> ux_system_slave_device_dfu_state_machine;

                    /* String index set to 0.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STRING) = 0;
                    
                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH);
                    
                    break;
                    
                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATE :
                
                    /* Fill the status data payload.  First with state.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) _ux_system_slave -> ux_system_slave_device_dfu_state_machine; 

                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH);
                    
                    break;
                    
                default:
        
                    /* Unknown function. Stall the endpoint.  */
                    _ux_device_stack_endpoint_stall(&device -> ux_slave_device_control_endpoint);

                    /* In the system, state the DFU state machine to dfu ERROR.  */
                       _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SYSTEM_DFU_STATE_DFU_ERROR;

                    break;
            }

            break;

        case UX_SLAVE_CLASS_DFU_STATUS_STATE_DFU_MANIFEST_SYNC         :

            /* Here we process only the request we can accept in the MANIFEST SYNCH state.  */
            switch (request)
            {
        
                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATUS :
                
                    /* Check if the device is still buys performing the write. Write could be delayed.  */
                    dfu -> ux_slave_class_dfu_get_status(dfu, &media_status);
                    
                    /* Check status of device.  */
                    switch (media_status)
                    {

                        case     UX_SLAVE_CLASS_DFU_MEDIA_STATUS_OK        :

                            /* Set the next state for wait reset and no error status.  */
                            dfu -> ux_slave_class_dfu_status = UX_SLAVE_CLASS_DFU_STATUS_OK ;
                               _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SLAVE_CLASS_DFU_STATUS_STATE_DFU_MANIFEST_WAIT_RESET;

                            /* Check who is responsible for the RESET.  */
                            if (_ux_system_slave -> ux_system_slave_device_dfu_capabilities &  UX_SLAVE_CLASS_DFU_CAPABILITY_WILL_DETACH)
                            {
        
                                /* Wake up the DFU thread and send a detach request..  */
                                _ux_utility_event_flags_set(&dfu -> ux_slave_class_dfu_event_flags_group, UX_DEVICE_CLASS_DFU_THREAD_EVENT_DISCONNECT, UX_OR);                

                            }

                            break;
                        
                        case     UX_SLAVE_CLASS_DFU_MEDIA_STATUS_BUSY    :
                        
                            /* Set the next state for busy but no error status.  */
                            dfu -> ux_slave_class_dfu_status = UX_SLAVE_CLASS_DFU_STATUS_OK ;
                               _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SLAVE_CLASS_DFU_STATUS_STATE_DFU_MANIFEST;
                            break;
                            
                        case    UX_SLAVE_CLASS_DFU_MEDIA_STATUS_ERROR   :

                            /* Set the next state for busy and error status.  */
                            dfu -> ux_slave_class_dfu_status = UX_SLAVE_CLASS_DFU_STATUS_ERROR_WRITE ;
                               _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SLAVE_CLASS_DFU_STATUS_STATE_DFU_ERROR;
                            break;
                    }

                    /* Fill the status data payload.  First with status.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) dfu -> ux_slave_class_dfu_status; 

                    /* Poll time out value is set to 1ms.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT) = 1;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 1) = 0;
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 2) = 0;

                    /* Next state.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STATE) = (UCHAR) _ux_system_slave -> ux_system_slave_device_dfu_state_machine;

                    /* String index set to 0.  */
                    *(transfer_request -> ux_slave_transfer_request_data_pointer + UX_SLAVE_CLASS_DFU_GET_STATUS_STRING) = 0;
                    
                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH);
                    
                    break;

                case UX_SLAVE_CLASS_DFU_COMMAND_GET_STATE :
                
                    /* Fill the status data payload.  First with state.  */
                    *transfer_request -> ux_slave_transfer_request_data_pointer = (UCHAR) _ux_system_slave -> ux_system_slave_device_dfu_state_machine; 

                    /* We have a request to obtain the status of the DFU instance. */
                    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATE_LENGTH);
                    
                    break;

                default:
        
                    /* Unknown function. Stall the endpoint.  */
                    _ux_device_stack_endpoint_stall(&device -> ux_slave_device_control_endpoint);
                    break;
            }

            break;
            
        case UX_SLAVE_CLASS_DFU_STATUS_STATE_DFU_ERROR         :

            /* Here we process only the request we can accept in the ERROR state.  */
            switch (request)
            {
        
                case UX_SLAVE_CLASS_DFU_COMMAND_CLEAR_STATUS :

                    /* In the system, state the DFU state machine to dfu IDLE.  */
                    _ux_system_slave -> ux_system_slave_device_dfu_state_machine = UX_SYSTEM_DFU_STATE_DFU_IDLE;
                
                    break;

                default:
        
                    /* Unknown function. Stall the endpoint.  */
                    _ux_device_stack_endpoint_stall(&device -> ux_slave_device_control_endpoint);
                    break;

            }

            break;


        default:

            /* Unknown state. Should not happen.  */
            return(UX_ERROR);
    }

    return(UX_SUCCESS);
}

