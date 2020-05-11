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
/*    _ux_device_class_storage_csw_send                   PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function sends the status phase of SCSI transaction. Note that */ 
/*    dCSWDataResidue is always set to 0 because we either transfer all   */ 
/*    the data, or report command failure.                                */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    storage                               Pointer to storage class      */ 
/*    endpoint_in                           Pointer to IN endpoint        */ 
/*    status                                Status of CSW                 */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_device_stack_transfer_request     Transfer request              */ 
/*    _ux_utility_long_put                  Put long word                 */ 
/*    _ux_utility_memory_copy               Copy memory                   */ 
/*    _ux_utility_memory_set                Set memory                    */ 
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
UINT  _ux_device_class_storage_csw_send(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, 
                                UX_SLAVE_ENDPOINT *endpoint_in, UCHAR csw_status)
{

UINT                    status;
UX_SLAVE_TRANSFER       *transfer_request;
UCHAR                   csw_buffer[UX_SLAVE_CLASS_STORAGE_CSW_LENGTH];

    /* Obtain the pointer to the transfer request.  */
    transfer_request =  &endpoint_in -> ux_slave_endpoint_transfer_request;

    /* Ensure it is cleaned.  */
    _ux_utility_memory_set(csw_buffer, 0, UX_SLAVE_CLASS_STORAGE_CSW_LENGTH);

    /* Store the signature of the CSW.  */
    _ux_utility_long_put(&csw_buffer[UX_SLAVE_CLASS_STORAGE_CSW_SIGNATURE], UX_SLAVE_CLASS_STORAGE_CSW_SIGNATURE_MASK);

    /* Store the SCSI tag from the CBW.  */
    _ux_utility_long_put(&csw_buffer[UX_SLAVE_CLASS_STORAGE_CSW_TAG], storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_scsi_tag);

    /* Store the status of the previous operation.  */
    csw_buffer[UX_SLAVE_CLASS_STORAGE_CSW_STATUS] =  csw_status;

    /* Copy the CSW into the transfer request memory.  */
    _ux_utility_memory_copy(transfer_request -> ux_slave_transfer_request_data_pointer,
                                        csw_buffer, UX_SLAVE_CLASS_STORAGE_CSW_LENGTH);

    /* We may be in a special state machine condition where the endpoint is stalled waiting for
       a CLEAR_FEATURE.  We will wait until the host clears the endpoint.  
       The transfer_request function does that.  */
    /* Send the CSW back to the host.  */
    status =  _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_STORAGE_CSW_LENGTH, 
                                    UX_SLAVE_CLASS_STORAGE_CSW_LENGTH);

    /* Return completion status.  */
    return(status);
}

