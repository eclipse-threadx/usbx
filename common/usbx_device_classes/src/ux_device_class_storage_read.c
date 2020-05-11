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


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_device_class_storage.h"
#include "ux_device_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_device_class_storage_read                       PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function performs a READ command in 32 or 16 bits.             */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    storage                               Pointer to storage class      */ 
/*    endpoint_in                           Pointer to IN endpoint        */
/*    endpoint_out                          Pointer to OUT endpoint       */
/*    cbwcb                                 Pointer to CBWCB              */ 
/*    scsi_command                          SCSI command                  */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    (ux_slave_class_storage_media_read)   Read from media               */ 
/*    (ux_slave_class_storage_media_status) Get media status              */ 
/*    _ux_device_stack_endpoint_stall       Stall endpoint                */ 
/*    _ux_device_stack_transfer_request     Transfer request              */ 
/*    _ux_utility_long_get_big_endian       Get 32-bit big endian         */ 
/*    _ux_utility_short_get_big_endian      Get 16-bit big endian         */ 
/*    _ux_device_class_storage_csw_send     Send CSW                      */
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
UINT  _ux_device_class_storage_read(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, 
                                            UX_SLAVE_ENDPOINT *endpoint_in,
                                            UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb, UCHAR scsi_command)
{

UINT                    status;
UX_SLAVE_TRANSFER       *transfer_request;
ULONG                   lba;
ULONG                   total_number_blocks; 
ULONG                   number_blocks; 
ULONG                   media_status;
ULONG                   total_length;
ULONG                   transfer_length;

    UX_PARAMETER_NOT_USED(endpoint_out);

    /* Get the LBA from the CBWCB.  */
    lba =  _ux_utility_long_get_big_endian(cbwcb + UX_SLAVE_CLASS_STORAGE_READ_LBA);

    /* The type of commands will tell us the width of the field containing the number
       of sectors to read.  */
    if (scsi_command == UX_SLAVE_CLASS_STORAGE_SCSI_READ16)

        /* Get the number of blocks from the CBWCB in 16 bits.  */
        total_number_blocks =  _ux_utility_short_get_big_endian(cbwcb + UX_SLAVE_CLASS_STORAGE_READ_TRANSFER_LENGTH_16);

    else        

        /* Get the number of blocks from the CBWCB in 32 bits.  */
        total_number_blocks =  _ux_utility_long_get_big_endian(cbwcb + UX_SLAVE_CLASS_STORAGE_READ_TRANSFER_LENGTH_32);

    /* Obtain the pointer to the transfer request.  */
    transfer_request =  &endpoint_in -> ux_slave_endpoint_transfer_request;

    /* Compute the total length to transfer and how much remains.  */
    total_length =  total_number_blocks * storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_block_length;

    /* It may take several transfers to send the requested data.  */
    while (total_number_blocks)
    {

        /* Obtain the status of the device.  */
        status =  storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_status(storage, lun, 
                                    storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_id, &media_status);
    
        /* Update the request sense.  */
        storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_sense_key         =  (UCHAR) (media_status & 0xff);
        storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code              =  (UCHAR) ((media_status >> 8 ) & 0xff);
        storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code_qualifier    =  (UCHAR) ((media_status >> 16 ) & 0xff);
    
        /* If there is a problem, return a failed command.  */
        if (status != UX_SUCCESS)
        {
    
            /* We have a problem, media status error. Return a bad completion and wait for the
               REQUEST_SENSE command.  */
            _ux_device_stack_endpoint_stall(endpoint_in);
    
            /* Now we return a CSW with failure.  */
            _ux_device_class_storage_csw_send(storage, lun, endpoint_in, UX_SLAVE_CLASS_STORAGE_CSW_FAILED);
    
            /* Return an error.  */
            return(UX_ERROR);
        }

        /* How much can we send in this transfer?  */
        if (total_length > UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE)

            /* Compute the transfer length based on the maximum allowed.  */
            transfer_length =  UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE;
            
        else

            /* Compute the transfer length based on what is left to transfer.  */
            transfer_length =  total_length;

        /* Compute the number of blocks to transfer.  */
        number_blocks = transfer_length / storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_block_length;

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_CLASS_STORAGE_READ, storage, lun, transfer_request -> ux_slave_transfer_request_data_pointer, 
                                number_blocks, UX_TRACE_DEVICE_CLASS_EVENTS, 0, 0)

        /* Execute the read command from the local media.  */
        status =  storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_read(storage, lun, 
                                                    transfer_request -> ux_slave_transfer_request_data_pointer, number_blocks, lba, &media_status); 

        /* If there is a problem, return a failed command.  */
        if (status != UX_SUCCESS)
        {
    
            /* We have a problem, request error. Return a bad completion and wait for the
               REQUEST_SENSE command.  */
            _ux_device_stack_endpoint_stall(endpoint_in);
    
            /* And update the REQUEST_SENSE codes.  */
            storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_sense_key         =  (UCHAR) (media_status & 0xff);
            storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code              =  (UCHAR) ((media_status >> 8 ) & 0xff);
            storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code_qualifier    =  (UCHAR) ((media_status >> 16 ) & 0xff);
    
            /* Now we return a CSW with failure.  */
            _ux_device_class_storage_csw_send(storage, lun, endpoint_in, UX_SLAVE_CLASS_STORAGE_CSW_FAILED);
    
            /* Return an error.  */
            return(UX_ERROR);
        }

        /* Sends the data payload back to the caller.  */
        status =  _ux_device_stack_transfer_request(transfer_request, transfer_length, transfer_length);

        /* Check the status.  */
        if(status != UX_SUCCESS)
        {

            /* We have a problem, request error. Return a bad completion and wait for the
               REQUEST_SENSE command.  */
            _ux_device_stack_endpoint_stall(endpoint_in);
    
            /* Update the REQUEST_SENSE codes.  */
            storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_sense_key         =  0x02;
            storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code              =  0x54;
            storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code_qualifier    =  0x00;

            /* Now we return a CSW with failure.  */
            _ux_device_class_storage_csw_send(storage, lun, endpoint_in, UX_SLAVE_CLASS_STORAGE_CSW_FAILED);
    
            /* Return an error.  */
            return(UX_ERROR);

        }

        /* Update the LBA address.  */
        lba += number_blocks;
        
        /* Update the length to remain.  */
        total_length -= transfer_length;        
        
        /* Update the number of blocks to read.  */
        total_number_blocks -= number_blocks;
    }
        
    /* Now we return a CSW with success.  */
    status =  _ux_device_class_storage_csw_send(storage, lun, endpoint_in, UX_SLAVE_CLASS_STORAGE_CSW_PASSED);

    /* Return completion status.  */
    return(status);
}
