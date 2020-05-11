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
/*    _ux_device_class_storage_test_ready                 PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function tests if the SCSI slave device is ready.              */ 
/*    The status function of the storage devices is called. If there is   */ 
/*    an error, the request sense parameters are set for the host to      */ 
/*    investigate.                                                        */ 
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
/*    (ux_slave_class_storage_media_status) Get media status              */ 
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
UINT  _ux_device_class_storage_test_ready(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun, UX_SLAVE_ENDPOINT *endpoint_in,
                                          UX_SLAVE_ENDPOINT *endpoint_out, UCHAR * cbwcb)
{

UINT        status;
ULONG        media_status;

    UX_PARAMETER_NOT_USED(cbwcb);
    UX_PARAMETER_NOT_USED(endpoint_out);

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_CLASS_STORAGE_TEST_READY, storage, lun, 0, 0, UX_TRACE_DEVICE_CLASS_EVENTS, 0, 0)

    /* Obtain the status of the device.  */
    status =  storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_status(storage, lun, 
                                storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_id, &media_status);

    /* Set the sense/code/qualifier codes for the REQUEST_SENSE command.  */
    storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_sense_key =       (media_status & 0xff);
    storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code =            ((media_status >> 8) & 0xff);
    storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_code_qualifier =  ((media_status >> 16) & 0xff);

    /* Check the status for error.  */
    if (status != UX_SUCCESS)
    {

        /* We return a CSW with error.  */
        status =  _ux_device_class_storage_csw_send(storage, lun, endpoint_in, UX_SLAVE_CLASS_STORAGE_CSW_FAILED);
    }        
    else
    {

        /* We return a CSW with success.  */
        status =  _ux_device_class_storage_csw_send(storage, lun, endpoint_in, UX_SLAVE_CLASS_STORAGE_CSW_PASSED);
    }
    
    /* Return completion status.  */
    return(status);
}

