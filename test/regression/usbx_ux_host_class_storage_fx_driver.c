/* Implements FX driver entry if FX_MEDIA not integrated in UX Host Class Storage.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "fx_api.h"

#include "ux_device_class_storage.h"
#include "ux_device_stack.h"
#include "ux_host_stack.h"
#include "ux_host_class_storage.h"

#if defined(UX_HOST_CLASS_STORAGE_NO_FILEX)


VOID        _ux_host_class_storage_driver_entry(FX_MEDIA *media);

VOID  _ux_host_class_storage_media_insert(UX_HOST_CLASS_STORAGE_MEDIA *storage_media, ULONG format_open);
VOID  _ux_host_class_storage_media_remove(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);

FX_MEDIA    *_ux_host_class_storage_driver_medias(void);
FX_MEDIA    *_ux_host_class_storage_driver_media(INT i);
UCHAR       *_ux_host_class_storage_driver_media_memory(INT i);
INT         _ux_host_class_storage_driver_media_index(FX_MEDIA *media);

INT         _ux_host_class_storage_media_index(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
FX_MEDIA    *_ux_host_class_storage_media_fx_media(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
UCHAR       *_ux_host_class_storage_media_fx_media_memory(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);

static FX_MEDIA medias[UX_HOST_CLASS_STORAGE_MAX_MEDIA];
static UCHAR    medias_memories[UX_HOST_CLASS_STORAGE_MAX_MEDIA][UX_HOST_CLASS_STORAGE_MEMORY_BUFFER_SIZE];
static VOID (*_ux_host_class_storage_media_read_write_notify)(UINT, UINT,
                                UX_HOST_CLASS_STORAGE *, ULONG, ULONG, UCHAR*) = UX_NULL;

VOID _ux_host_class_storage_driver_read_write_notify(
    VOID (*func)(UINT, UINT, UX_HOST_CLASS_STORAGE *, ULONG, ULONG, UCHAR*))
{
    _ux_host_class_storage_media_read_write_notify = func;
}

FX_MEDIA *_ux_host_class_storage_driver_medias(void)
{
    return(medias);
}

FX_MEDIA *_ux_host_class_storage_driver_media(INT i)
{
    return(&medias[i]);
}

UCHAR *_ux_host_class_storage_driver_media_memory(INT i)
{
    return(medias_memories[i]);
}

INT       _ux_host_class_storage_driver_media_index(FX_MEDIA *media)
{

INT         media_index;


    if (media < medias)
        return(-1);
    media_index = (INT)(media - medias);
    if (media_index >= UX_HOST_CLASS_STORAGE_MAX_MEDIA)
        return(-1);
    return(media_index);
}

INT     _ux_host_class_storage_media_index(UX_HOST_CLASS_STORAGE_MEDIA *storage_media)
{
UX_HOST_CLASS_STORAGE           *storage;
UX_HOST_CLASS                   *class_inst;
UX_HOST_CLASS_STORAGE_MEDIA     *storage_medias;
    if (storage_media == UX_NULL)
        return(-1);
    if (storage_media -> ux_host_class_storage_media_storage == UX_NULL)
        return(-1);
    storage = storage_media -> ux_host_class_storage_media_storage;
    if (storage == UX_NULL)
        return(-1);
    class_inst = storage -> ux_host_class_storage_class;
    if (class_inst == UX_NULL)
        return(-1);
    storage_medias = (UX_HOST_CLASS_STORAGE_MEDIA *)class_inst -> ux_host_class_media;
    if (storage_media < storage_medias)
        return(-1);
    return((INT)(storage_media - storage_medias));
}

FX_MEDIA    *_ux_host_class_storage_media_fx_media(UX_HOST_CLASS_STORAGE_MEDIA *storage_media)
{
INT     media_index = _ux_host_class_storage_media_index(storage_media);
    if (media_index < 0)
        return(UX_NULL);
    return(&medias[media_index]);
}
UCHAR       *_ux_host_class_storage_media_fx_media_memory(UX_HOST_CLASS_STORAGE_MEDIA *storage_media)
{
INT     media_index = _ux_host_class_storage_media_index(storage_media);
    if (media_index < 0)
        return(UX_NULL);
    return(medias_memories[media_index]);
}

VOID  _ux_host_class_storage_media_insert(UX_HOST_CLASS_STORAGE_MEDIA *storage_media, ULONG format_open)
{
INT     media_index = _ux_host_class_storage_media_index(storage_media);
UINT    rc = 0;
    if (media_index < 0)
    {
        return;
    }
    medias[media_index].fx_media_driver_info = storage_media;
    /* Format.  */
    if (format_open > 1)
    {
        rc = fx_media_format(&medias[media_index],
                        _ux_host_class_storage_driver_entry,
                        storage_media,
                        medias_memories[media_index], UX_HOST_CLASS_STORAGE_MEMORY_BUFFER_SIZE,
                        "USB DISK", 2, 512, 0,
                        storage_media -> ux_host_class_storage_media_number_sectors,
                        storage_media -> ux_host_class_storage_media_sector_size,
                        4,
                        1, 1);
        if (rc != UX_SUCCESS)
        {
            printf("%s:%d media_format error 0x%x\n", __FILE__, __LINE__, rc);
        }
    }
    /* Open.  */
    if (format_open > 0 && rc == 0)
    {
        /* Open the media.  */
        rc = fx_media_open(&medias[media_index], UX_HOST_CLASS_STORAGE_MEDIA_NAME,
                        _ux_host_class_storage_driver_entry,
                        storage_media,
                        medias_memories[media_index],
                        UX_HOST_CLASS_STORAGE_MEMORY_BUFFER_SIZE);
        if (rc != UX_SUCCESS)
        {
            printf("%s:%d media_open error 0x%x\n", __FILE__, __LINE__, rc);
        }
    }
    if (rc)
    {
        /* Inserted but not ready.  */
        medias[media_index].fx_media_id = 0;
    }
    else
    {
    }
}

VOID  _ux_host_class_storage_media_remove(UX_HOST_CLASS_STORAGE_MEDIA *storage_media)
{
INT     media_index = _ux_host_class_storage_media_index(storage_media);
UINT    rc = 0;

    if (media_index < 0)
    {
        return;
    }
    if (medias[media_index].fx_media_driver_info == storage_media)
    {
        rc = fx_media_close(&medias[media_index]);
        medias[media_index].fx_media_id = 0; /* Keeping compatible with integrated FX_MEDIA.  */
        medias[media_index].fx_media_driver_info = UX_NULL;
        if (rc != UX_SUCCESS)
        {
            printf("%s:%d media_close error %x\n", __FILE__, __LINE__, rc);
        }
    }
}

VOID  _ux_host_class_storage_driver_entry(FX_MEDIA *media)
{

UINT                            status;
UX_HOST_CLASS_STORAGE           *storage;
UX_HOST_CLASS_STORAGE_MEDIA     *storage_media;
    

    /* Get the pointer to the storage media instance.  */
    storage_media =  (UX_HOST_CLASS_STORAGE_MEDIA *) media -> fx_media_driver_info;

    /* Get storage instance.  */
    storage = storage_media -> ux_host_class_storage_media_storage;

    /* Ensure the instance is valid.  */
    if ((storage -> ux_host_class_storage_state !=  UX_HOST_CLASS_INSTANCE_LIVE) &&
        (storage -> ux_host_class_storage_state !=  UX_HOST_CLASS_INSTANCE_MOUNTING))
    {

        /* Class instance is invalid. Return an error!  */
        media -> fx_media_driver_status =  FX_PTR_ERROR;
        return;
    }

    /* Protect Thread reentry to this instance and select media LUN for further access.  */
    status = _ux_host_class_storage_media_lock(storage_media, UX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
    {
        printf("%s:%d lock fail 0x%x!\n", __FILE__, __LINE__, status);
        media -> fx_media_driver_status = FX_IO_ERROR;
        return;
    }

    /* Look at the request specified by the FileX caller.  */
    switch (media -> fx_media_driver_request)
    {

    case FX_DRIVER_READ:

        /* Read one or more sectors.  */
        status =  _ux_host_class_storage_media_read(storage, media -> fx_media_driver_logical_sector,
                                        media -> fx_media_driver_sectors, media -> fx_media_driver_buffer);
        if (_ux_host_class_storage_media_read_write_notify)
            _ux_host_class_storage_media_read_write_notify(FX_DRIVER_READ, status,
                                        storage,
                                        media -> fx_media_driver_logical_sector,
                                        media -> fx_media_driver_sectors,
                                        media -> fx_media_driver_buffer);

        /* Check completion status.  */
        if (status == UX_SUCCESS)
            media -> fx_media_driver_status =  FX_SUCCESS;
        else
            media -> fx_media_driver_status =  _ux_host_class_storage_sense_code_translate(storage, status);
        break;


    case FX_DRIVER_WRITE:

        /* Write one or more sectors.  */
        status =  _ux_host_class_storage_media_write(storage, media -> fx_media_driver_logical_sector,
                                        media -> fx_media_driver_sectors, media -> fx_media_driver_buffer);
        if (_ux_host_class_storage_media_read_write_notify)
            _ux_host_class_storage_media_read_write_notify(FX_DRIVER_WRITE, status,
                                        storage,
                                        media -> fx_media_driver_logical_sector,
                                        media -> fx_media_driver_sectors,
                                        media -> fx_media_driver_buffer);

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
        status =  _ux_host_class_storage_media_read(storage, 0, 1,
                                                media -> fx_media_driver_buffer);
        if (_ux_host_class_storage_media_read_write_notify)
            _ux_host_class_storage_media_read_write_notify(FX_DRIVER_READ, status,
                                        storage, 0, 1,
                                        media -> fx_media_driver_buffer);

        /* Check completion status.  */
        if (status == UX_SUCCESS)
            media -> fx_media_driver_status =  FX_SUCCESS;
        else
            media -> fx_media_driver_status =  _ux_host_class_storage_sense_code_translate(storage,status);
        break;


    case FX_DRIVER_BOOT_WRITE:

        /* Write the boot sector.  */
        status =  _ux_host_class_storage_media_write(storage, 0, 1,
                                                media -> fx_media_driver_buffer);
        if (_ux_host_class_storage_media_read_write_notify)
            _ux_host_class_storage_media_read_write_notify(FX_DRIVER_WRITE, status,
                                        storage, 0, 1,
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
    _ux_host_class_storage_media_unlock(storage_media);
}
#endif
