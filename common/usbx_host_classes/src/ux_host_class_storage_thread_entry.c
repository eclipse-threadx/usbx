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
/*    _ux_host_class_storage_thread_entry                 PORTABLE C      */
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is awaken every 2 seconds to check if there was a     */
/*    device insertion on a specific media. This is the only way we can   */
/*    remount a media after the storage instance has opened the media to  */
/*    FileX and the media is either not present or was removed and is     */
/*    being re-)inserted.                                                 */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    class_address                         Class address                 */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    fx_media_close                        Close FileX media             */
/*    _ux_host_class_storage_device_reset   Reset device                  */
/*    _ux_host_class_storage_media_mount    Mount the media               */
/*    _ux_host_class_storage_unit_ready_test                              */
/*                                          Test for unit ready           */
/*    _ux_host_class_storage_media_characteristics_get                    */
/*                                          Get media characteristics     */
/*    _ux_host_class_storage_media_format_capacity_get                    */
/*                                          Get media format capacity     */
/*    _ux_utility_memory_free               Free memory block             */
/*    _ux_utility_semaphore_get             Get a semaphore               */
/*    _ux_utility_semaphore_put             Put a semaphore               */
/*    _ux_utility_delay_ms                  Thread sleep                  */
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
VOID  _ux_host_class_storage_thread_entry(ULONG class_address)
{

UX_HOST_CLASS                   *class;
UX_HOST_CLASS_STORAGE           *storage;
UINT                            status;
ULONG                           lun_index;
UX_HOST_CLASS_STORAGE_MEDIA     *storage_media;
FX_MEDIA                        *media;
UCHAR                           *memory;
UINT                            media_index;


    /* Setup pointer to class.  */
    UX_THREAD_EXTENSION_PTR_GET(class, UX_HOST_CLASS, class_address)

    /* This thread goes on forever once started.  */
    while(1)
    {

        /* We need to wake every 2 seconds or so.  */
        _ux_utility_delay_ms(UX_HOST_CLASS_STORAGE_THREAD_SLEEP_TIME);

        /* We need to parse all the storage instances and check for a removable
           media flag.  */
        storage =  (UX_HOST_CLASS_STORAGE *) class -> ux_host_class_first_instance;

        while (storage != UX_NULL)
        {

            /* Check if the instance is live and the device is removable.  */
            if ((storage -> ux_host_class_storage_state == UX_HOST_CLASS_INSTANCE_LIVE))
            {

                /* We need to ensure nobody is accessing this storage instance. We use
                   the storage class instance semaphore to protect.  */
                status =  _ux_utility_semaphore_get(&storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);
                if (status != UX_SUCCESS)
                    break;

                /* Each LUN must be parsed and mounted.  */
                for (lun_index = 0; lun_index <= storage -> ux_host_class_storage_max_lun; lun_index++)
                {

                    if (storage -> ux_host_class_storage_lun_removable_media_flags[lun_index] != UX_HOST_CLASS_STORAGE_MEDIA_REMOVABLE)
                        continue;

                    /* Check the type of LUN, we only deal with the ones we know how to mount.  */
                    if ((storage -> ux_host_class_storage_lun_types[lun_index] == UX_HOST_CLASS_STORAGE_MEDIA_FAT_DISK) ||
                        (storage -> ux_host_class_storage_lun_types[lun_index] == UX_HOST_CLASS_STORAGE_MEDIA_OPTICAL_DISK) ||
                        (storage -> ux_host_class_storage_lun_types[lun_index] == UX_HOST_CLASS_STORAGE_MEDIA_IOMEGA_CLICK))
                    {

                        /* Set the LUN into the storage instance.  */
                        storage -> ux_host_class_storage_lun =  lun_index;

                        /* Check if the device is now ready.  */
                        status =  _ux_host_class_storage_unit_ready_test(storage);

                        /* If we have an transport failure here, we are in trouble! The storage device
                           is in an unstable state and should be reset completely.  */
                        if (status != UX_SUCCESS)
                        {

                            /* Reset device.  */
                            _ux_host_class_storage_device_reset(storage);
                            break;
                        }

                        /* Process relative to device status.  */
                        switch(storage -> ux_host_class_storage_sense_code >> 16)
                        {

                        case UX_HOST_CLASS_STORAGE_SENSE_KEY_NOT_READY:

                            /* We may need to unmount this partition if it was mounted before.
                               To do so, we need to parse the existing media instance and find out
                               if this partition was already mounted.  */

                            storage_media = (UX_HOST_CLASS_STORAGE_MEDIA *) class -> ux_host_class_media;

                            /* Scan all instances of media.  */
                            for (media_index = 0; media_index < UX_HOST_CLASS_STORAGE_MAX_MEDIA; media_index++)
                            {

                                /* Get the FileX Media attached to this media.  */
                                media =  &storage_media -> ux_host_class_storage_media;

                                /* Check for the storage instance and lun number.  */
                                if ((media -> fx_media_id != 0) && (media -> fx_media_driver_info == (VOID *) storage) &&
                                    (storage_media -> ux_host_class_storage_media_lun == storage -> ux_host_class_storage_lun))
                                {

                                    /* We preserve the memory used by this media.  */
                                    memory =  storage_media -> ux_host_class_storage_media_memory;

                                    /* Let FileX use this instance.  */
                                    status =  _ux_utility_semaphore_put(&storage -> ux_host_class_storage_semaphore);

                                    /* Ask FileX to unmount the partition.  */
                                    fx_media_close(media);

                                    /* This device is now unmounted.  */
                                    storage_media -> ux_host_class_storage_media_status = UX_HOST_CLASS_STORAGE_MEDIA_UNMOUNTED;

                                    /* Reset the media ID.  */
                                    media -> fx_media_id =  0;

                                    /* Now, we protect the storage instance.  */
                                    status =  _ux_utility_semaphore_get(&storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);
                                    if (status != UX_SUCCESS)
                                        break;

                                    /* Free the memory block used for data transfer on behalf of FileX.  */
                                    _ux_utility_memory_free(memory);
                                }

                                /* Next entry in the media array.  */
                                storage_media++;
                            }
                            break;

                            case UX_HOST_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION:

                                /* We may need to unmount this partition if it was mounted before.
                                   To do so, we need to parse the existing media instance and find out
                                   if this partition was already mounted.  */
                                storage_media = (UX_HOST_CLASS_STORAGE_MEDIA *) class -> ux_host_class_media;

                                /* Scan all instances of media.   */
                                for (media_index = 0; media_index < UX_HOST_CLASS_STORAGE_MAX_MEDIA; media_index++)
                                {

                                    /* Get the FileX Media attached to this media.  */
                                    media = &storage_media -> ux_host_class_storage_media;

                                    /* Check for the storage instance and lun number. */
                                    if ((media -> fx_media_id != 0) && (media -> fx_media_driver_info == (VOID *) storage) &&
                                            (storage_media -> ux_host_class_storage_media_lun == storage -> ux_host_class_storage_lun))
                                    {

                                        /* We preserve the memory used by this media.  */
                                        memory =  storage_media -> ux_host_class_storage_media_memory;

                                        /* Let FileX use this instance.  */
                                        status =  _ux_utility_semaphore_put(&storage -> ux_host_class_storage_semaphore);

                                        /* Ask FileX to unmount the partition.  */
                                        fx_media_close(media);

                                        /* This device is now unmounted.  */
                                        storage_media -> ux_host_class_storage_media_status =  UX_HOST_CLASS_STORAGE_MEDIA_UNMOUNTED;

                                        /* Reset the media ID.  */
                                        media -> fx_media_id =  0;

                                        /* Now, we protect the storage instance.  */
                                        status =  _ux_utility_semaphore_get(&storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);
                                        if (status != UX_SUCCESS)
                                            break;

                                        /* Free the memory block used for data transfer on behalf of FileX.  */
                                        _ux_utility_memory_free(memory);
                                    }

                                    /* Next entry in the media array.  */
                                    storage_media++;
                                }

                                /* Now, we need to retest the media to see if it is there.  */
                                status =  _ux_host_class_storage_unit_ready_test(storage);

                                /* If we have an transport failure here, we are in trouble! The storage device
                                   is in an unstable state and should be reset completely.  */
                                if (status != UX_SUCCESS)
                                {

                                    /* Reset device.  */
                                    _ux_host_class_storage_device_reset(storage);
                                    break;
                                }

                                if (storage -> ux_host_class_storage_sense_code == 0)
                                {

                                    /* Get the media type supported by this storage device.  */
                                    status =  _ux_host_class_storage_media_characteristics_get(storage);
                                    if (status != UX_SUCCESS)
                                        break;

                                    /* Get the format capacity of this storage device.  */
                                    status =  _ux_host_class_storage_media_format_capacity_get(storage);
                                    if (status != UX_SUCCESS)
                                        break;

                                    /* Let FileX use this instance.  */
                                    _ux_utility_semaphore_put(&storage -> ux_host_class_storage_semaphore);

                                    /* The device seems to have been inserted, try to mount it.  */
                                    _ux_host_class_storage_media_mount(storage, 0);

                                    /* Now, we protect the storage instance.  */
                                    _ux_utility_semaphore_get(&storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);
                                }
                                break;

                            default:
                                break;
                        }
                    }
                }

                /* Other threads are now allowed to access this storage instance.  */
                status =  _ux_utility_semaphore_put(&storage -> ux_host_class_storage_semaphore);
            }

            /* Move to the next entry in the storage instances link.  */
            storage =  storage -> ux_host_class_storage_next_instance;
        }
    }
}
