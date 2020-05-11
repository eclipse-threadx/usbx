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
/**   Storage Class                                                       */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_storage.h"
#include "ux_host_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_storage_device_initialize            PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function initializes the USB storage device.                   */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    storage                               Pointer to storage class      */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_class_storage_device_reset   Reset device                  */ 
/*    _ux_host_class_storage_device_support_check                         */ 
/*                                          Check protocol support        */ 
/*    _ux_host_class_storage_endpoints_get  Get all endpoints             */ 
/*    _ux_host_class_storage_max_lun_get    Get maximum number of LUNs    */ 
/*    _ux_host_class_storage_media_characteristics_get                    */ 
/*                                          Get media characteristics     */ 
/*    _ux_host_class_storage_media_format_capacity_get                    */
/*                                          Get format capacity           */
/*    _ux_host_class_storage_media_mount    Mount the media               */ 
/*    _ux_utility_delay_ms                  Delay ms                      */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Storage Class                                                       */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_storage_device_initialize(UX_HOST_CLASS_STORAGE *storage)
{

UINT        status;
ULONG       lun_index;

    /* Check the device protocol support and initialize the transport layer.  */
    status =  _ux_host_class_storage_device_support_check(storage);
    if (status != UX_SUCCESS)
        return(status);
    
    /* Get the maximum number of LUN (Bulk Only device only, other device
       will set the LUN number to 0).  */
    status =  _ux_host_class_storage_max_lun_get(storage);
    if (status != UX_SUCCESS)
        return(status);

    /* Search all the endpoints for the storage interface (Bulk Out, Bulk in,
       and optional Interrupt endpoint).  */
    status =  _ux_host_class_storage_endpoints_get(storage);
    if (status != UX_SUCCESS)
        return(status);

    /* We need to wait for some device to settle. The INTUIX Flash disk is an example of
       these device who fail the first Inquiry command if sent too quickly.  
       The timing does not have to be precise so we use the thread sleep function.  
       The default sleep value is 2 seconds.  */
    _ux_utility_delay_ms(UX_HOST_CLASS_STORAGE_DEVICE_INIT_DELAY);

    /* Each LUN must be parsed and mounted.  */
    for (lun_index = 0; lun_index <= storage -> ux_host_class_storage_max_lun; lun_index++)
    {

        /* Set the LUN into the storage instance.  */
        storage -> ux_host_class_storage_lun =  lun_index;

        /* Get the media type supported by this storage device.  */
        status =  _ux_host_class_storage_media_characteristics_get(storage);
        if (status == UX_HOST_CLASS_MEDIA_NOT_SUPPORTED)
        {
            /* Unsupported device.  */

            /* Error trap. */
            _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_MEDIA_NOT_SUPPORTED);

            /* If trace is enabled, insert this event into the trace buffer.  */
            UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_HOST_CLASS_MEDIA_NOT_SUPPORTED, storage, 0, 0, UX_TRACE_ERRORS, 0, 0)

            continue;
        }
        if (status != UX_SUCCESS)
            return(status);

        /* Get the format capacity of this storage device.  */
        status =  _ux_host_class_storage_media_format_capacity_get(storage);
        if (status != UX_SUCCESS)
            return(status);

        /* Store the LUN type in the LUN type array. */
        storage -> ux_host_class_storage_lun_types[lun_index] = storage -> ux_host_class_storage_media_type;

        /* Check the media type. We support regular FAT drives and optical drives. 
           No CD-ROM support in this release.  */
        switch (storage -> ux_host_class_storage_media_type)
        {

        case UX_HOST_CLASS_STORAGE_MEDIA_FAT_DISK:
        case UX_HOST_CLASS_STORAGE_MEDIA_OPTICAL_DISK:
        case UX_HOST_CLASS_STORAGE_MEDIA_IOMEGA_CLICK:

            /* Try to read the device media in search for a partition table or boot sector.
               We are at the root of the disk, so use sector 0 as the starting point.  */
            _ux_host_class_storage_media_mount(storage, 0);
            break;

        case UX_HOST_CLASS_STORAGE_MEDIA_CDROM:
        default:

           /* In the case of CD-ROM, we do no need to mount any file system yet. The application
              can read sectors directly.  */

            break;
        }
    }

    /* Some LUNs may succeed and some may fail. For simplicity's sake, we just
       return success. The storage thread will try to remount the ones that failed.  */
    return(UX_SUCCESS);
}

