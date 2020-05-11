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
/*    _ux_device_class_storage_request_sense              PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function performs a request sense command.                     */ 
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
UINT  _ux_device_class_storage_request_sense(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                                            UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb)
{

UINT                    status;
UX_SLAVE_TRANSFER       *transfer_request;
UCHAR                   sense_buffer[UX_SLAVE_CLASS_STORAGE_INQUIRY_RESPONSE_LENGTH];

    UX_PARAMETER_NOT_USED(cbwcb);
    UX_PARAMETER_NOT_USED(endpoint_out);

    /* Obtain the pointer to the transfer request.  */
    transfer_request =  &endpoint_in -> ux_slave_endpoint_transfer_request;

    /* Ensure it is cleaned.  */
    _ux_utility_memory_set(sense_buffer, 0, UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH);
    
    /* Initialize the response buffer with the error code.  */
    sense_buffer[UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_ERROR_CODE] = 
                    UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_ERROR_CODE_VALUE;

    /* Initialize the response buffer with the sense key.  */
    sense_buffer[UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_SENSE_KEY] = 
                    storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_sense_key;

    /* Initialize the response buffer with the code.  */
    sense_buffer[UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_CODE] = 
                    storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code;

    /* Initialize the response buffer with the code qualifier.  */
    sense_buffer[UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_CODE_QUALIFIER] = 
                    storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code_qualifier;

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_CLASS_STORAGE_REQUEST_SENSE, storage, lun, 
                            storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_sense_key, 
                            storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code, UX_TRACE_DEVICE_CLASS_EVENTS, 0, 0)

    /* Initialize the response buffer with the additional length.  */
    sense_buffer[UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_ADD_LENGTH] =  10;


    /* Copy the request sense response into the transfer request memory.  */
    _ux_utility_memory_copy(transfer_request -> ux_slave_transfer_request_data_pointer, 
                                        sense_buffer, UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH);

    /* Send a data payload with the sense codes.  */
    _ux_device_stack_transfer_request(transfer_request, UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH,
                              UX_SLAVE_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH);
    
    /* Now we return a CSW with success.  */
    status =  _ux_device_class_storage_csw_send(storage, lun, endpoint_in, UX_SLAVE_CLASS_STORAGE_CSW_PASSED);

    /* Return completion status.  */    
    return(status);
}

