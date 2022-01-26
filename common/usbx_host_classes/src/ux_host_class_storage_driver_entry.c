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


#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX) && !defined(UX_HOST_STANDALONE)
/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_storage_driver_entry                 PORTABLE C      */ 
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function is the entry point for the FileX file system. All     */ 
/*    FileX driver I/O calls are are multiplexed here and rerouted to     */ 
/*    the proper USB storage class functions.                             */
/*                                                                        */
/*    This entry is for FX support, it's not available if FX media is not */
/*    integrated or standalone mode is used.                              */
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    media                                 FileX media pointer           */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    None                                                                */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_class_storage_sense_code_translate                         */
/*                                          Translate error status codes  */ 
/*    _ux_host_class_storage_media_read     Read sector(s)                */ 
/*    _ux_host_class_storage_media_write    Write sector(s)               */ 
/*    _ux_host_semaphore_get                Get protection semaphore      */ 
/*    _ux_host_semaphore_put                Release protection semaphore  */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    FileX                                                               */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            added option to disable FX  */
/*                                            media integration,          */
/*                                            resulting in version 6.1    */
/*  01-31-2022     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1.10 */
/*                                                                        */
/**************************************************************************/
VOID  _ux_host_class_storage_driver_entry(FX_MEDIA *media)
{

UINT                            status;
UX_HOST_CLASS_STORAGE           *storage;
UX_HOST_CLASS_STORAGE_MEDIA     *storage_media;
    

    /* Get the pointer to the storage instance.  */
    storage =  (UX_HOST_CLASS_STORAGE *) media -> fx_media_driver_info;

    /* Get the pointer to the media instance.  */
    storage_media =  (UX_HOST_CLASS_STORAGE_MEDIA *) media -> fx_media_reserved_for_user;
    
    /* Ensure the instance is valid.  */
    if ((storage -> ux_host_class_storage_state !=  UX_HOST_CLASS_INSTANCE_LIVE) &&
        (storage -> ux_host_class_storage_state !=  UX_HOST_CLASS_INSTANCE_MOUNTING))
    {

        /* Class instance is invalid. Return an error!  */
        media -> fx_media_driver_status =  FX_PTR_ERROR;
        return;
    }

    /* Protect Thread reentry to this instance.  */
    status = _ux_host_semaphore_get(&storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);

    /* Restore the LUN number from the media instance.  */
    storage -> ux_host_class_storage_lun =  storage_media -> ux_host_class_storage_media_lun;
    
    /* And the sector size.  */
    storage -> ux_host_class_storage_sector_size =  storage_media -> ux_host_class_storage_media_sector_size;
    
    
    /* Look at the request specified by the FileX caller.  */
    switch (media -> fx_media_driver_request)
    {

    case FX_DRIVER_READ:

        /* Read one or more sectors.  */
        status =  _ux_host_class_storage_media_read(storage, media -> fx_media_driver_logical_sector + 
                                        storage_media -> ux_host_class_storage_media_partition_start,
                                        media -> fx_media_driver_sectors, media -> fx_media_driver_buffer);

        /* Check completion status.  */
        if (status == UX_SUCCESS)
            media -> fx_media_driver_status =  FX_SUCCESS;
        else
            media -> fx_media_driver_status =  _ux_host_class_storage_sense_code_translate(storage, status);
        break;
            

    case FX_DRIVER_WRITE:

        /* Write one or more sectors.  */
        status =  _ux_host_class_storage_media_write(storage, media -> fx_media_driver_logical_sector + 
                                            storage_media -> ux_host_class_storage_media_partition_start,
                                        media -> fx_media_driver_sectors, media -> fx_media_driver_buffer);

        /* Check completion status.  */
        if (status == UX_SUCCESS)
            media -> fx_media_driver_status =  FX_SUCCESS;
        else
            media -> fx_media_driver_status =  _ux_host_class_storage_sense_code_translate(storage,status);
        break;


    case FX_DRIVER_FLUSH:

        /* Nothing to do. Just return a good status!  */
        media -> fx_media_driver_status =  FX_SUCCESS;
        break;

    case FX_DRIVER_ABORT:

        /* Nothing to do. Just return a good status!  */
        media -> fx_media_driver_status =  FX_SUCCESS;
        break;

    case FX_DRIVER_INIT:

        /* Check for media protection.  We must do this operation here because FileX clears all the 
           media fields before init.  */
        if (storage -> ux_host_class_storage_write_protected_media ==  UX_TRUE)
        
            /* The media is Write Protected. We tell FileX.  */
            media -> fx_media_driver_write_protect = UX_TRUE;

        /* This function always succeeds.  */
        media -> fx_media_driver_status =  FX_SUCCESS;
        break;


    case FX_DRIVER_UNINIT:

        /* Nothing to do. Just return a good status!  */
        media -> fx_media_driver_status =  FX_SUCCESS;
        break;


    case FX_DRIVER_BOOT_READ:

        /* Read the media boot sector.  */
        status =  _ux_host_class_storage_media_read(storage, storage_media -> ux_host_class_storage_media_partition_start, 1,
                                                            media -> fx_media_driver_buffer);
        
        /* Check completion status.  */
        if (status == UX_SUCCESS)
            media -> fx_media_driver_status =  FX_SUCCESS;
        else
            media -> fx_media_driver_status =  _ux_host_class_storage_sense_code_translate(storage,status);
        break;
            

    case FX_DRIVER_BOOT_WRITE:

        /* Write the boot sector.  */
        status =  _ux_host_class_storage_media_write(storage, storage_media -> ux_host_class_storage_media_partition_start, 1,
                                                            media -> fx_media_driver_buffer);

        /* Check completion status.  */
        if (status == UX_SUCCESS)
            media -> fx_media_driver_status =  FX_SUCCESS;
        else
            media -> fx_media_driver_status =  _ux_host_class_storage_sense_code_translate(storage, status);
        break;

    default:

        /* Invalid request from FileX */
        media -> fx_media_driver_status =  FX_IO_ERROR;
        break;
    }

    /* Unprotect thread reentry to this instance.  */
    _ux_host_semaphore_put(&storage -> ux_host_class_storage_semaphore);
}
#endif /* !defined(UX_HOST_CLASS_STORAGE_NO_FILEX) */
