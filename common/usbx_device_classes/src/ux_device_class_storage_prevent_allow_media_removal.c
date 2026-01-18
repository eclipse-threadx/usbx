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
/*    _ux_device_class_storage_prevent_allow_media_removal                */
/*                                                        PORTABLE C      */
/*                                                           6.4.6        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function allows or prevents the removal of the media.          */
/*    The PREVENT ALLOW MEDIUM REMOVAL command allows an application      */
/*    client to restrict the demounting of the removable                  */
/*    medium.                                                             */
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
UINT  _ux_device_class_storage_prevent_allow_media_removal(UX_SLAVE_CLASS_STORAGE *storage, ULONG lun,
                                                           UX_SLAVE_ENDPOINT *endpoint_in,
                                                           UX_SLAVE_ENDPOINT *endpoint_out, UCHAR *cbwcb)
{

ULONG   prevent_flag;

    UX_PARAMETER_NOT_USED(endpoint_in);
    UX_PARAMETER_NOT_USED(endpoint_out);

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_CLASS_STORAGE_PREVENT_ALLOW_MEDIA_REMOVAL, storage, lun, 0, 0, UX_TRACE_DEVICE_CLASS_EVENTS, 0, 0)

    /* Parse the PREVENT/ALLOW MEDIA REMOVAL command: byte 4 bit 0 is the prevent flag.  */
    prevent_flag = (ULONG)(cbwcb[4] & 0x01);

    /* Update internal flag to track prevent/allow state for this LUN.  */
    if (prevent_flag == UX_SLAVE_CLASS_STORAGE_MEDIUM_REMOVAL_IS_ALLOWED)
        storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_prevent_medium_removal = 0;
    else
        storage -> ux_slave_class_storage_lun[lun].ux_slave_class_storage_prevent_medium_removal = 1;

    /* We set the CSW with success.  */
    storage -> ux_slave_class_storage_csw_status = UX_SLAVE_CLASS_STORAGE_CSW_PASSED;

    /* Return successful completion.  */
    return(UX_SUCCESS);
}
