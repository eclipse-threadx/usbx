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
/*    _ux_host_class_storage_media_open                   PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function will ask FileX to mount a new partition for this      */ 
/*    device. This function has some FileX dependencies.                  */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    storage                               Pointer to storage class      */ 
/*    hidden_sectors                        Number of hidden sectors      */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    fx_media_open                         Media open                    */ 
/*    _ux_host_class_storage_media_protection_check                       */
/*                                          Check for protection          */ 
/*    _ux_utility_memory_allocate           Allocate memory block         */ 
/*    _ux_utility_memory_free               Free memory block             */
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    _ux_host_class_storage_media_mount            Media open            */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_storage_media_open(UX_HOST_CLASS_STORAGE *storage, ULONG hidden_sectors)
{

UINT                                status;
UINT                                media_index;
UX_HOST_CLASS_STORAGE_MEDIA         *storage_media;
FX_MEDIA                            *media;
UX_HOST_CLASS                       *class;
    

    /* We need the class container.  */
    class =  storage -> ux_host_class_storage_class;
    
    /* Point the media structure to the first media in the container.  */
    storage_media =  (UX_HOST_CLASS_STORAGE_MEDIA *) class -> ux_host_class_media;

    /* Locate a free partition in the storage instance.  */
    for (media_index = 0; media_index < UX_HOST_CLASS_STORAGE_MAX_MEDIA; media_index++)
    {

        /* Get the FileX media pointer for that media.  */
        media =  &storage_media -> ux_host_class_storage_media;
        
        /* Is the media valid?  */
        if (media -> fx_media_id == 0)
        {

            /* Save the storage instance in the media instance.  */
            media -> fx_media_driver_info =  (VOID *) storage;
                            
            /* Save the number of hidden sectors in this partition.  */
            storage_media -> ux_host_class_storage_media_partition_start =  hidden_sectors;

            /* Save the LUN number in the storage media instance.  */
            storage_media -> ux_host_class_storage_media_lun =  storage -> ux_host_class_storage_lun;

            /* Save the Sector size in the storage media instance.  */
            storage_media -> ux_host_class_storage_media_sector_size =  storage -> ux_host_class_storage_sector_size;

            /* Save the storage media instance in the user reserved area in the FX_MEDIA structure.  */
            media -> fx_media_reserved_for_user =  (ALIGN_TYPE) storage_media;

            /* We now need to allocate a block of memory for FileX to use when doing transfers 
               The default buffer size is 8K. The value used for the definition is UX_HOST_CLASS_STORAGE_MEMORY_BUFFER_SIZE. 
               This value can be changed to save on memory space but should not be smaller than 
               the media sector size (which should be 512 bytes). Because USB devices are SCSI 
               devices and there is a great deal of overhead when doing read/writes, it is better   
               to leave the default buffer size or even increase it. */
            storage_media -> ux_host_class_storage_media_memory =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_STORAGE_MEMORY_BUFFER_SIZE);
            if (storage_media -> ux_host_class_storage_media_memory == UX_NULL)
                return(UX_MEMORY_INSUFFICIENT);

            /* If trace is enabled, insert this event into the trace buffer.  */
            UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_STORAGE_MEDIA_OPEN, storage, media, 0, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

            /* Ask FileX to mount the partition.  */
            status =  fx_media_open(media, UX_HOST_CLASS_STORAGE_MEDIA_NAME, _ux_host_class_storage_driver_entry,
                                        storage, storage_media -> ux_host_class_storage_media_memory, 
                                        UX_HOST_CLASS_STORAGE_MEMORY_BUFFER_SIZE);

            /* If the media is mounted, update the status for the application.  */
            if (status == UX_SUCCESS)
                storage_media -> ux_host_class_storage_media_status = UX_HOST_CLASS_STORAGE_MEDIA_MOUNTED;

            else

                /* Free the memory resources.  */
                _ux_utility_memory_free(storage_media -> ux_host_class_storage_media_memory);

            /* Return completion status.  */
            return(status);         
        }

        /* Move to next entry in the media array.  */
        storage_media++;
    }

    /* Error trap. */
    _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_MEMORY_ERROR);

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_HOST_CLASS_MEMORY_ERROR, storage, 0, 0, UX_TRACE_ERRORS, 0, 0)

    /* Return error.  */    
    return(UX_HOST_CLASS_MEMORY_ERROR);
}

