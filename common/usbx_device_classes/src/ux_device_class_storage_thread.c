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
/*    _ux_device_class_storage_thread                     PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function is the thread of the storage class.                   */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    class                                 Address of storage class      */ 
/*                                            container                   */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    None                                                                */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_device_class_storage_format       Storage class format          */ 
/*    _ux_device_class_storage_inquiry      Storage class inquiry         */ 
/*    _ux_device_class_storage_mode_select  Mode select                   */ 
/*    _ux_device_class_storage_mode_sense   Mode sense                    */ 
/*    _ux_device_class_storage_prevent_allow_media_removal                */ 
/*                                          Prevent media removal         */ 
/*    _ux_device_class_storage_read         Read                          */ 
/*    _ux_device_class_storage_read_capacity                              */
/*                                          Read capacity                 */ 
/*    _ux_device_class_storage_read_format_capacity                       */ 
/*                                          Read format capacity          */ 
/*    _ux_device_class_storage_request_sense                              */
/*                                          Sense request                 */ 
/*    _ux_device_class_storage_start_stop   Start/Stop                    */
/*    _ux_device_class_storage_synchronize_cache                          */ 
/*                                          Synchronize cache             */
/*    _ux_device_class_storage_test_ready   Ready test                    */ 
/*    _ux_device_class_storage_verify       Verify                        */ 
/*    _ux_device_class_storage_write        Write                         */
/*    _ux_device_stack_endpoint_stall       Endpoint stall                */ 
/*    _ux_device_stack_interface_delete     Interface delete              */ 
/*    _ux_device_stack_transfer_request     Transfer request              */ 
/*    _ux_utility_long_get                  Get 32-bit value              */ 
/*    _ux_utility_memory_allocate           Allocate memory               */ 
/*    _ux_utility_semaphore_create          Create semaphore              */ 
/*    _ux_utility_thread_suspend            Suspend thread                */ 
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
/*                                                                        */
/**************************************************************************/
VOID  _ux_device_class_storage_thread(ULONG storage_class)
{

UX_SLAVE_CLASS              *class;
UX_SLAVE_CLASS_STORAGE      *storage;
UX_SLAVE_TRANSFER           *transfer_request;
UX_SLAVE_DEVICE             *device;
UX_SLAVE_INTERFACE          *interface;
UX_SLAVE_ENDPOINT           *endpoint_in;
UX_SLAVE_ENDPOINT           *endpoint_out;
UINT                        status;
ULONG                       length;
ULONG                       cbwcb_length;
ULONG                       lun;
UCHAR                       *scsi_command;


    /* This thread runs forever but can be suspended or resumed.  */
    while(1)
    {

        /* Cast properly the storage instance.  */
        UX_THREAD_EXTENSION_PTR_GET(class, UX_SLAVE_CLASS, storage_class)
        
        /* Get the storage instance from this class container.  */
        storage =  (UX_SLAVE_CLASS_STORAGE *) class -> ux_slave_class_instance;
    
        /* Get the pointer to the device.  */
        device =  &_ux_system_slave -> ux_system_slave_device;
        
        /* This is the first time we are activated. We need the interface to the class.  */
        interface =  storage -> ux_slave_class_storage_interface;
        
        /* Locate the endpoints.  */
        endpoint_in =  interface -> ux_slave_interface_first_endpoint;
        
        /* Check the endpoint direction, if IN we have the correct endpoint.  */
        if ((endpoint_in -> ux_slave_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) != UX_ENDPOINT_IN)
        {

            /* Wrong direction, we found the OUT endpoint first.  */
            endpoint_out =  endpoint_in;
                
            /* So the next endpoint has to be the IN endpoint.  */
            endpoint_in =  endpoint_out -> ux_slave_endpoint_next_endpoint;
        }
        else
        {

            /* We found the endpoint IN first, so next endpoint is OUT.  */
            endpoint_out =  endpoint_in -> ux_slave_endpoint_next_endpoint;
        }
            
        /* All SCSI commands are on the endpoint OUT, from the host.  */
        transfer_request =  &endpoint_out -> ux_slave_endpoint_transfer_request;
    
        /* As long as the device is in the CONFIGURED state.  */
        while (device -> ux_slave_device_state == UX_DEVICE_CONFIGURED)
        { 

            /* We assume the worst situation.  */
            status =  UX_ERROR;
        
            /* Check state, they must be both RESET.  */
            if (endpoint_out -> ux_slave_endpoint_state == UX_ENDPOINT_RESET &&
                !storage -> ux_slave_class_storage_phase_error)
            {

                /* Send the request to the device controller.  */
                status =  _ux_device_stack_transfer_request(transfer_request, 64, 64);

            }                
    
            /* Check the status. Our status is UX_ERROR if one of the endpoint was STALLED. We must wait for the host
               to clear the mess.   */    
            if (status == UX_SUCCESS)
            {

                /* Obtain the length of the transaction.  */
                length =  transfer_request -> ux_slave_transfer_request_actual_length;
                
                /* Obtain the buffer address containing the SCSI command.  */
                scsi_command =  transfer_request -> ux_slave_transfer_request_data_pointer;
                
                /* Obtain the lun from the CBW.  */
                lun =  (ULONG) *(scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_LUN);
                
                /* We have to memorize the SCSI command tag for the CSW phase.  */
                storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_scsi_tag =  _ux_utility_long_get(scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_TAG);

                /* Ensure the LUN number is within our declared values and check the command 
                   content and format. First we make sure we have a complete CBW.  */
                if ((lun < storage -> ux_slave_class_storage_number_lun) && (length == UX_SLAVE_CLASS_STORAGE_CBW_LENGTH))
                {

                    /* The length of the CBW is correct, analyze the header.  */
                    if (_ux_utility_long_get(scsi_command) == UX_SLAVE_CLASS_STORAGE_CBW_SIGNATURE_MASK)
                    {

                        /* Get the length of the CBWCB.  */
                        cbwcb_length =  (ULONG) *(scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB_LENGTH);
    
                        /* Check the length of the CBWCB to ensure there is at least a command.  */
                        if (cbwcb_length != 0)
                        {

                            /* Analyze the command stored in the CBWCB.  */
                            switch (*(scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB))
                            {

                            case UX_SLAVE_CLASS_STORAGE_SCSI_TEST_READY:

                                _ux_device_class_storage_test_ready(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB);
                                break;
                                    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_REQUEST_SENSE:

                                _ux_device_class_storage_request_sense(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_FORMAT:

                                _ux_device_class_storage_format(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_INQUIRY:

                                _ux_device_class_storage_inquiry(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_START_STOP:

                                _ux_device_class_storage_start_stop(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB);
                                break;
                                    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_PREVENT_ALLOW_MEDIA_REMOVAL:

                                _ux_device_class_storage_prevent_allow_media_removal(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_READ_FORMAT_CAPACITY:

                                _ux_device_class_storage_read_format_capacity(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_READ_CAPACITY:

                                _ux_device_class_storage_read_capacity(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_VERIFY:

                                _ux_device_class_storage_verify(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SELECT:

                                _ux_device_class_storage_mode_select(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE_SHORT:
                            case UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE:

                                _ux_device_class_storage_mode_sense(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_READ32:

                                _ux_device_class_storage_read(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB, 
                                                                UX_SLAVE_CLASS_STORAGE_SCSI_READ32);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_READ16:

                                _ux_device_class_storage_read(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB, 
                                                                UX_SLAVE_CLASS_STORAGE_SCSI_READ16);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_WRITE32:

                                _ux_device_class_storage_write(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB,
                                                                UX_SLAVE_CLASS_STORAGE_SCSI_WRITE32);
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16:

                                _ux_device_class_storage_write(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB, 
                                                                UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16);
                                break;

                            case UX_SLAVE_CLASS_STORAGE_SCSI_SYNCHRONIZE_CACHE:

                                _ux_device_class_storage_synchronize_cache(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB, *(scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB));
                                break;

#ifdef UX_SLAVE_CLASS_STORAGE_INCLUDE_MMC
                            case UX_SLAVE_CLASS_STORAGE_SCSI_GET_STATUS_NOTIFICATION:

                                _ux_device_class_storage_get_status_notification(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB); 
                                break;

                            case UX_SLAVE_CLASS_STORAGE_SCSI_GET_CONFIGURATION:

                                _ux_device_class_storage_get_configuration(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB); 
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_READ_DISK_INFORMATION:

                                _ux_device_class_storage_read_disk_information(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB); 
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_REPORT_KEY:

                                _ux_device_class_storage_report_key(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB); 
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_GET_PERFORMANCE:

                                _ux_device_class_storage_get_performance(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB); 
                                break;
    
                            case UX_SLAVE_CLASS_STORAGE_SCSI_READ_DVD_STRUCTURE:

                                _ux_device_class_storage_read_dvd_structure(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB); 
                                break;

                            case UX_SLAVE_CLASS_STORAGE_SCSI_READ_TOC:

                                status = _ux_device_class_storage_read_toc(storage, lun, endpoint_in, endpoint_out, scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_CB); 

                                /* Special treatment of TOC command. If error, default to Stall endpoint.  */
                                if (status == UX_SUCCESS)
                                    break;
                                /* fall through */
#endif

                            default:
    
                                /* The command is unknown or unsupported, so we stall the endpoint.  */

                                if (_ux_utility_long_get(scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_DATA_LENGTH) > 0 &&
                                    ((*(scsi_command + UX_SLAVE_CLASS_STORAGE_CBW_FLAGS) & 0x80) == 0))

                                    /* Data-Out from host to device, stall OUT.  */
                                    _ux_device_stack_endpoint_stall(endpoint_out);
                                else

                                    /* Data-In from device to host, stall IN.  */
                                    _ux_device_stack_endpoint_stall(endpoint_in);
                                
                                /* Initialize the request sense keys.  */
                                storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_sense_key =       UX_SLAVE_CLASS_STORAGE_SENSE_KEY_ILLEGAL_REQUEST;
                                storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code =            UX_SLAVE_CLASS_STORAGE_ASC_KEY_INVALID_COMMAND;
                                storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code_qualifier =  0;

                                /* This is the tricky part of the SCSI state machine. We must send the CSW BUT need to wait
                                   for the endpoint_in to be reset by the host.  */
                                while (device -> ux_slave_device_state == UX_DEVICE_CONFIGURED)
                                { 

                                    /* Check the endpoint state.  */
                                    if (endpoint_in -> ux_slave_endpoint_state == UX_ENDPOINT_RESET)
                                    {

                                        /* Now we return a CSW with failure.  */
                                        status =  _ux_device_class_storage_csw_send(storage, lun, endpoint_in, UX_SLAVE_CLASS_STORAGE_CSW_FAILED);
            
                                        /* Check error code. */
                                        if (status != UX_SUCCESS)

                                            /* Error trap. */
                                            _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, status);
            
                                        break;
                                    }                                        

                                    else

                                        /* We must therefore wait a while.  */
                                        _ux_utility_thread_relinquish();
                                }
                                break;
                            }
                        }
                        else

                            /* Phase error!  */
                            storage -> ux_slave_class_storage_phase_error = TX_TRUE;
                    }
                    
                    else

                        /* Phase error!  */
                        storage -> ux_slave_class_storage_phase_error = TX_TRUE;
                }
                else

                    /* Phase error!  */
                    storage -> ux_slave_class_storage_phase_error = TX_TRUE;
            }
            else
            {

                if (storage -> ux_slave_class_storage_phase_error == TX_TRUE)
                {

                    /* We should keep the endpoints stalled.  */
                    _ux_device_stack_endpoint_stall(endpoint_out);
                    _ux_device_stack_endpoint_stall(endpoint_in);
                }

                /* We must therefore wait a while.  */
                _ux_utility_thread_relinquish();
            }
        }

        /* We need to suspend ourselves. We will be resumed by the 
           device enumeration module.  */
        _ux_utility_thread_suspend(&class -> ux_slave_class_thread);
    }
}

