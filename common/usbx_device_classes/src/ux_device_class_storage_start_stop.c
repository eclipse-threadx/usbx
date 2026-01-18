/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/


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
/*    _ux_device_class_storage_start_stop                 PORTABLE C      */
/*                                                           6.4.6        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function starts or stops the media. The device load or eject   */
/*    the medium.                                                         */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    storage                               Pointer to storage class      */
/*    lun                                   Logical unit number           */
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
/*    None                                                                */
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
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            optimized command logic,    */
/*                                            resulting in version 6.1    */
/*  01-20-2026     Mohamed AYED             Modified comment(s),          */
/*                                            support load eject media    */
/*                                            resulting in version 6.4.6  */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_storage_start_stop(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun,
                                          UX_SLAVE_ENDPOINT *endpoint_in,
                                          UX_SLAVE_ENDPOINT *endpoint_out, UCHAR *cbwcb)
{

ULONG   power_condition;
ULONG   start;
ULONG   load_eject;

    UX_PARAMETER_NOT_USED(endpoint_in);
    UX_PARAMETER_NOT_USED(endpoint_out);

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_CLASS_STORAGE_START_STOP, storage, lun, 0, 0, UX_TRACE_DEVICE_CLASS_EVENTS, 0, 0)

    /* Parse START/STOP (SBC-2 0x1B): byte 4 has LOEJ (bit1) and START (bit0).  */
    power_condition = (ULONG)((cbwcb[4] & 0xF0) >> 0x04);
    start = (ULONG)(cbwcb[4] & 0x01);
    load_eject  = (ULONG)((cbwcb[4] & 0x02) >> 0x01);

    if ((load_eject == 1) &&
        (storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_prevent_medium_removal == 1) &&
        (storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_removable_flag == UX_SLAVE_CLASS_STORAGE_MEDIA_IS_NOT_REMOVABLE))
    {
        /* Update the REQUEST SENSE codes.  */
        storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_request_sense_status =
            UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(UX_SLAVE_CLASS_STORAGE_SENSE_KEY_ILLEGAL_REQUEST,
                                                 UX_SLAVE_CLASS_STORAGE_ASC_KEY_INVALID_COMMAND, 0x00);

        /* Set the CSW with failure.  */
        storage -> ux_slave_class_storage_csw_status = UX_SLAVE_CLASS_STORAGE_CSW_FAILED;

        /* Return error.  */
        return(UX_ERROR);
    }

    /*  power_condition = 0, load_eject = 0 : no action regarding loading or ejecting the medium.
        power_condition = 0, load_eject = 1, start = 0 : unload the medium.
        power_condition = 0, load_eject = 1, start = 1 : load the medium.
        */
    if (power_condition == UX_SLAVE_CLASS_STORAGE_POWER_CONDITION_START_VALID)
    {
        if (load_eject != 0)
        {
            if (start == 0)
            {
                /* Eject media: mark as empty. */
                storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_medium_loaded_status = 0;
            }
            else
            {
                /* Load media: mark as present/complete.  */
                storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_medium_loaded_status = 1;
            }
        }
    }

    /* Call the media start/stop function if available.  */
    if (storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_start_stop != UX_NULL)
        storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_media_start_stop(
                    storage, lun, power_condition, start, load_eject);

    /* We set the CSW with success.  */
    storage -> ux_slave_class_storage_csw_status = UX_SLAVE_CLASS_STORAGE_CSW_PASSED;

    /* Return successful completion.  */
    return(UX_SUCCESS);
}
