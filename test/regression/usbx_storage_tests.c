#include <stdio.h>
#include <stdint.h>
#include "fx_api.h"
#include "tx_api.h"
#include "ux_api.h"
#include "ux_hcd_sim_host.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "tx_thread.h"

#include "ux_device_stack.h"
#include "ux_device_class_storage.h"
#include "ux_host_class_storage.h"
#include "ux_dcd_sim_slave.h"

#include "ux_test.h"
#include "ux_test_actions.h"
#include "ux_test_utility_sim.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_dcd_sim_slave.h"

typedef struct device_media_read_write_timeout_data
{
    UX_TRANSFER *transfer_request;
    ULONG       num_timeouts;
} DEVICE_MEDIA_READ_WRITE_TIMEOUT_DATA;

#define _storage_media_is_mounted() (global_storage_media->ux_host_class_storage_media_status == UX_HOST_CLASS_STORAGE_MEDIA_MOUNTED)
#if defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
FX_MEDIA    *_ux_host_class_storage_driver_media(INT i);
VOID        _ux_host_class_storage_driver_entry(FX_MEDIA *media);
VOID        _ux_host_class_storage_media_insert(UX_HOST_CLASS_STORAGE_MEDIA *storage_media, ULONG format_open);
VOID        _ux_host_class_storage_media_remove(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
INT         _ux_host_class_storage_media_index(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
FX_MEDIA    *_ux_host_class_storage_media_fx_media(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
UCHAR       *_ux_host_class_storage_media_fx_media_memory(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
#endif

/* Define constants.  */
#define     UX_TEST_MULTIPLE_TRANSFERS_SECTOR_COUNT (2*UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE/UX_HOST_CLASS_STORAGE_SECTOR_SIZE_FAT)
#define     UX_TEST_SLEEP_STORAGE_THREAD_RUN_ONCE   (3*UX_HOST_CLASS_STORAGE_THREAD_SLEEP_TIME/2)
#define     UX_TEST_DEFAULT_SECTOR_SIZE             UX_HOST_CLASS_STORAGE_SECTOR_SIZE_FAT
#define     UX_TEST_MAX_SECTOR_SIZE                 (2*UX_HOST_CLASS_STORAGE_SECTOR_SIZE_FAT)
#define     UX_TEST_NUM_DIRECTORY_ENTRIES           512
#define     UX_DEMO_STACK_SIZE                      2048
#define     UX_DEMO_BUFFER_SIZE                     2048
#define     UX_DEMO_FILE_BUFFER_SIZE                UX_TEST_MAX_SECTOR_SIZE
#define     UX_DEMO_LARGE_FILE_BUFFER_SIZE          (2*UX_HOST_CLASS_STORAGE_MAX_TRANSFER_SIZE)
#define     UX_DEMO_RECEPTION_BLOCK_SIZE            64
#define     UX_DEMO_MEMORY_SIZE                     (256 * 1024)
#define     UX_DEMO_FILE_SIZE                       (4 * 1024)
#define     UX_RAM_DISK_SIZE                        (200 * 1024)
#define     UX_RAM_DISK_LAST_LBA                    ((UX_RAM_DISK_SIZE / global_sector_size) -1)
#define     BULK_IN                                 1
#define     BULK_OUT                                2
#define     PARTITION_TYPE_UNKNOWN                  0xff
#define     MEDIA_TYPE_UNKNOWN                      0xff
#define     PROTOCOL_UNKNOWN                        0xff
#define     SUB_CLASS_UNKNOWN                       0xff
#define     bInterfaceSubClass_POS                  0x21
#define     bInterfaceProtocol_POS                  0x22
#define     bNumEndpoints_FS_POS                    0x1f
#define     bNumEndpoints_HS_POS                    0x29
#define     Endpoint_bLength                        0
#define     Endpoint_bDescriptorType                1
#define     Endpoint_bEndpointAddress               2
#define     Endpoint_bmAttributes                   3
#define     Endpoint_wMaxPacketSize                 4
#define     Endpoint_bInterval                      6

/* Define local/extern function prototypes.  */
static void                                 demo_thread_entry(ULONG);
static TX_THREAD                            tx_demo_thread_host_simulation;
static TX_THREAD                            tx_demo_thread_slave_simulation;
static void                                 ux_test_thread_host_simulation_entry(ULONG);
UINT                                        _ux_dcd_sim_slave_function(UX_SLAVE_DCD *dcd, UINT function, VOID *parameter);


VOID                                        _fx_ram_driver(FX_MEDIA *media_ptr);
static UINT                                 default_device_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                                 default_device_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                                 default_device_media_flush(VOID *storage, ULONG lun, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                                 default_device_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status);
static UINT                                 ux_test_storage_file_co(FX_MEDIA *media, FX_FILE *file, UCHAR fail_on_error);
static UINT                                 ux_test_storage_file_cow(FX_MEDIA *media, FX_FILE *file, UCHAR *data_pointer, ULONG data_length, UCHAR fail_on_error);
static void                                 ux_test_storage_file_cd(FX_MEDIA *media, FX_FILE *file);
static void                                 ux_test_storage_file_cowcd(FX_MEDIA *media, FX_FILE *file, UCHAR *data_pointer, ULONG data_length);

/* Define global data structures.  */
static UCHAR                                usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UCHAR                                global_buffer[UX_DEMO_LARGE_FILE_BUFFER_SIZE];
static UCHAR                                global_buffer_2x_slave_buffer_size[2*UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE];
static FX_MEDIA                             global_ram_disk;
static CHAR                                 global_ram_disk_memory[UX_RAM_DISK_SIZE];
static UCHAR                                global_ram_disk_working_buffer[UX_TEST_MAX_SECTOR_SIZE];
static UX_SLAVE_CLASS_STORAGE_PARAMETER     global_storage_parameter;
static UX_HOST_CLASS                        *global_host_storage_class;
static UX_HOST_CLASS_STORAGE_EXT            *global_host_class_ext;
static UX_HOST_CLASS_STORAGE                *global_storage;
static UX_HOST_CLASS_STORAGE                *global_storage_change_function;
static TX_THREAD                            *global_storage_thread;
static UX_HOST_CLASS_STORAGE_MEDIA          *global_storage_media;
static UX_HOST_CLASS_STORAGE_MEDIA          *global_storage_medias;
static FX_MEDIA                             *global_media;
static UX_HCD                               *global_hcd;
static UX_SLAVE_DCD                         *global_dcd;
static UX_DEVICE                            *global_host_device;
static UX_SLAVE_DEVICE                      *global_slave_device;
static UX_SLAVE_CLASS                       *global_slave_class_container;
static UX_SLAVE_CLASS_STORAGE               *global_slave_storage;
static UX_SLAVE_ENDPOINT                    *global_slave_storage_bulk_in;
static UX_SLAVE_ENDPOINT                    *global_slave_storage_bulk_out;
static TX_THREAD                            *global_slave_storage_thread;
static UX_SLAVE_CLASS_STORAGE               *global_persistent_slave_storage;
static TX_THREAD                            *global_enum_thread;
static ULONG                                global_sector_size = UX_TEST_DEFAULT_SECTOR_SIZE;
static ULONG                                global_memory_test_no_device_memory_free_amount;
static UCHAR                                global_is_storage_thread_locked_out;

/* Common data we match. */
static UCHAR global_transfer_request_data_request_sense_data_phase[] =  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static UCHAR global_cbw_data_unit_ready_test_sbc[20] =                  { 85, 83, 66, 67, 67, 66, 83, 85, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0 };
static UCHAR global_cbw_data_request_sense[26] =                        { 0x55, 0x53, 0x42, 0x43, 0x43, 0x42, 0x53, 0x55, 0x12, 0x00, 0x00, 0x00, 0x80, 0x00, 0x0c, 0x03, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static UCHAR global_cbw_data_format_capacity_get[] =                    { 0x55, 0x53, 0x42, 0x43, 0x43, 0x42, 0x53, 0x55, 0xfc, 0x00, 0x00, 0x00, 0x80, 0x00, 0x0a, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static UCHAR global_cbw_data_first_sector_read[] =                      { 0x55, 0x53, 0x42, 0x43, 0x43, 0x42, 0x53, 0x55, 0x00, 0x02, 0x00, 0x00, 0x80, 0x00, 0x0a, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static UCHAR global_cbw_data_media_characteristics_get_sbc[20] =        { 0x55, 0x53, 0x42, 0x43, 0x43, 0x42, 0x53, 0x55, 0x24, 0x00, 0x00, 0x00, 0x80, 0x00, 0x06, 0x12, 0x00, 0x00, 0x00, 0x24 };
static UCHAR global_cbw_data_media_capacity_get_sbc[25] =               { 0x55, 0x53, 0x42, 0x43, 0x43, 0x42, 0x53, 0x55, 0x08, 0x00, 0x00, 0x00, 0x80, 0x00, 0x0a, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

/* Opcode matching. */
static UCHAR global_cbw_opcode_read[] =                                 { 'U', 'S', 'B', 'C', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '(' };
static UCHAR global_cbw_opcode_match_mask[] =                           {  1,   1,   1,   1,   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1 };

/* Common errors. */
static UX_TEST_ERROR_CALLBACK_ERROR global_transfer_stall_error = { UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_TRANSFER_STALLED };
static UX_TEST_ERROR_CALLBACK_ERROR global_transfer_timeout_error = { UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_TRANSFER_TIMEOUT };

/* Test resources. */
static TX_SEMAPHORE global_test_semaphore;
static TX_TIMER     global_timer;

static UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
        0x81, 0x07, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Configuration descriptor */
        0x09, 0x02, 0x45, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x03, 0x08, 0x06, 0x50,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Interrupt In) */
        0x07, 0x05, 0x83, 0x03, 0x40, 0x00, 0x01,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x01, 0x03, 0x08, 0x06, 0x50,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Interrupt In) */
        0x07, 0x05, 0x83, 0x03, 0x40, 0x00, 0x01,

    };
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED ARRAY_COUNT(device_framework_full_speed)

static UCHAR *bulk_in_endpoint_descriptor_fs =      &device_framework_full_speed[0x12 + 0x09 + 0x09];
static UCHAR *bulk_out_endpoint_descriptor_fs =     &device_framework_full_speed[0x12 + 0x09 + 0x09 + 0x07];
static UCHAR *interrupt_in_endpoint_descriptor_fs = &device_framework_full_speed[0x12 + 0x09 + 0x09 + 0x07 + 0x07];

static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x81, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Device qualifier descriptor */
        0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x01, 0x00,

    /* Configuration descriptor */
        0x09, 0x02, 0x45, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x03, 0x08, 0x06, 0x50,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x00, 0x01, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x00, 0x01, 0x00,

    /* Endpoint descriptor (Interrupt In) */
        0x07, 0x05, 0x83, 0x03, 0x40, 0x00, 0x01,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x01, 0x03, 0x08, 0x06, 0x50,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Interrupt In) */
        0x07, 0x05, 0x83, 0x03, 0x40, 0x00, 0x01,

    };
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED ARRAY_COUNT(device_framework_high_speed)

static UCHAR *bulk_in_endpoint_descriptor_hs =      &device_framework_high_speed[0x12 + 0x0a + 0x09 + 0x09];
static UCHAR *bulk_out_endpoint_descriptor_hs =     &device_framework_high_speed[0x12 + 0x0a + 0x09 + 0x09 + 0x07];
static UCHAR *interrupt_in_endpoint_descriptor_hs = &device_framework_high_speed[0x12 + 0x0a + 0x09 + 0x09 + 0x07 + 0x07];


    /* String Device Framework :
     Byte 0 and 1 : Word containing the language ID : 0x0904 for US
     Byte 2       : Byte containing the index of the descriptor
     Byte 3       : Byte containing the length of the descriptor string
    */

#define STRING_FRAMEWORK_LENGTH 38
static UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
        0x09, 0x04, 0x02, 0x0a,
        0x46, 0x6c, 0x61, 0x73, 0x68, 0x20, 0x44, 0x69,
        0x73, 0x6b,

    /* Serial Number string descriptor : Index 3 */
        0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31
    };


    /* Multiple languages are supported on the device, to add
       a language besides english, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
#define LANGUAGE_ID_FRAMEWORK_LENGTH 2
static UCHAR language_id_framework[] = {

    /* English. */
        0x09, 0x04
    };


/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}

/* OS wrappers. */

static void ux_test_semaphore_get(TX_SEMAPHORE *sempahore, UINT wait_option)
{

UINT status;


    status =  ux_utility_semaphore_get(sempahore, wait_option);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n",__LINE__,(ULONG)status);
        test_control_return(1);
    }
}

/* General test resources. */

static void lock_out_storage_thread()
{

UINT status;


    status = ux_utility_semaphore_get(&global_storage -> ux_host_class_storage_semaphore,TX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n",__LINE__,(ULONG)status);
        test_control_return(1);
    }

    global_is_storage_thread_locked_out = 1;
}

static void lock_in_storage_thread()
{

UINT status;


    status = ux_utility_semaphore_put(&global_storage -> ux_host_class_storage_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n",__LINE__,(ULONG)status);
        test_control_return(1);
    }

    global_is_storage_thread_locked_out = 0;
}

static UINT ux_test_host_class_storage_media_read(UX_HOST_CLASS_STORAGE *storage, ULONG sector_start, ULONG sector_count, UCHAR *data_pointer)
{

    UX_TEST_ASSERT(global_is_storage_thread_locked_out);
    return(ux_host_class_storage_media_read(storage, sector_start, sector_count, data_pointer));
}

static UINT ux_test_host_class_storage_media_write(UX_HOST_CLASS_STORAGE *storage, ULONG sector_start, ULONG sector_count, UCHAR *data_pointer)
{

    UX_TEST_ASSERT(global_is_storage_thread_locked_out);
    return(ux_host_class_storage_media_write(storage, sector_start, sector_count, data_pointer));
}

/* Does a write - for times when you just want a write to trigger a CBW or something. */
static void do_any_write()
{

    lock_out_storage_thread();
    ux_test_host_class_storage_media_write(global_storage, 10, 10, global_buffer);
    lock_in_storage_thread();
}

/* Storage thread needs to be locked out when this is called. */
UINT _ux_host_stack_endpoint_reset(UX_ENDPOINT *endpoint);
static void receive_device_csw()
{

UX_TRANSFER *transfer_request;


    UX_TEST_ASSERT(global_is_storage_thread_locked_out);

    if (global_slave_storage_bulk_in->ux_slave_endpoint_state == UX_ENDPOINT_HALTED)
    {
        _ux_host_stack_endpoint_reset(global_storage->ux_host_class_storage_bulk_in_endpoint);
    }

    transfer_request =  &global_storage -> ux_host_class_storage_bulk_in_endpoint -> ux_endpoint_transfer_request;
    transfer_request -> ux_transfer_request_data_pointer =      (UCHAR *) &global_storage -> ux_host_class_storage_csw;
    transfer_request -> ux_transfer_request_requested_length =  UX_HOST_CLASS_STORAGE_CSW_LENGTH;
    UX_TEST_CHECK_SUCCESS(ux_host_stack_transfer_request(transfer_request));
    UX_TEST_CHECK_SUCCESS(ux_utility_semaphore_get(&transfer_request->ux_transfer_request_semaphore, TX_WAIT_FOREVER));

    /* Note: we want to do this _after_ because it's possible the device hasn't stalled the OUT endpoint before. */
    if (global_slave_storage_bulk_out->ux_slave_endpoint_state == UX_ENDPOINT_HALTED)
    {
        _ux_host_stack_endpoint_reset(global_storage->ux_host_class_storage_bulk_out_endpoint);
    }
}

/* Sometimes we need to storage instance during enumeration and before our system host change function has been called. */
static UX_HOST_CLASS_STORAGE *get_internal_host_storage_instance()
{
    return _ux_system_host -> ux_system_host_class_array[0].ux_host_class_first_instance;
}

/* Sometimes we need to storage instance during enumeration and before our system host change function has been called. */
static UX_HOST_CLASS_STORAGE_MEDIA *get_internal_host_storage_medias()
{
    return (_ux_system_host -> ux_system_host_class_array[0].ux_host_class_media);
}

static TX_THREAD *get_host_enum_thread()
{
    return &_ux_system_host->ux_system_host_enum_thread;
}

static VOID ux_slave_class_storage_instance_activate(VOID *instance)
{

UX_SLAVE_ENDPOINT *tmp;


    global_slave_storage = (UX_SLAVE_CLASS_STORAGE *)instance;

    global_slave_storage_bulk_in = global_slave_storage->ux_slave_class_storage_interface->ux_slave_interface_first_endpoint;
    global_slave_storage_bulk_out = global_slave_storage_bulk_in->ux_slave_endpoint_next_endpoint;
    if ((global_slave_storage_bulk_in->ux_slave_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) != UX_ENDPOINT_IN)
    {

        tmp = global_slave_storage_bulk_in;
        global_slave_storage_bulk_in = global_slave_storage_bulk_out;
        global_slave_storage_bulk_out = tmp;
    }

    /* As long as we don't unregister the storage class, this _should_ be fine! */
    global_persistent_slave_storage = global_slave_storage;
}

static VOID ux_slave_class_storage_instance_deactivate(VOID *instance)
{

    global_slave_storage = UX_NULL;
    global_slave_storage_bulk_in = UX_NULL;
    global_slave_storage_bulk_out = UX_NULL;
}

static UINT ux_test_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

    switch(event)
    {

        case UX_DEVICE_INSERTION:

            global_storage_change_function = inst;
            break;

        case UX_DEVICE_REMOVAL:

            global_storage_change_function = UX_NULL;
            break;

#if defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
        case UX_STORAGE_MEDIA_INSERTION:
            /* keep using first media.  */
            if (_ux_host_class_storage_media_index((UX_HOST_CLASS_STORAGE_MEDIA*)inst) == 0)
            {
                _ux_host_class_storage_media_insert((UX_HOST_CLASS_STORAGE_MEDIA*)inst, 1);
                global_media = _ux_host_class_storage_media_fx_media((UX_HOST_CLASS_STORAGE_MEDIA*)inst);
            }
            break;

        case UX_STORAGE_MEDIA_REMOVAL:
            if (_ux_host_class_storage_media_index((UX_HOST_CLASS_STORAGE_MEDIA*)inst) == 0)
            {
                _ux_host_class_storage_media_remove((UX_HOST_CLASS_STORAGE_MEDIA*)inst);
            }
            break;
#endif

        default:
            break;
    }

    return 0;
}

/* General worker thread resources. */

typedef struct UX_TEST_WORKER_WORK
{
    ULONG   input;
    void    (*function)(ULONG);
} UX_TEST_WORKER_WORK;

static UX_TEST_WORKER_WORK  *ux_test_worker_current_work;
static TX_SEMAPHORE         ux_test_worker_semaphore;
static TX_THREAD            ux_test_worker_thread;
static UCHAR                ux_test_worker_thread_stack[4096];

static void ux_test_worker_add_work(UX_TEST_WORKER_WORK *work)
{

UINT status;


    /* There shouldn't be more than one work at a time. */
    UX_TEST_ASSERT(ux_test_worker_semaphore.tx_semaphore_suspended_count == 1);

    ux_test_worker_current_work = work;

    status = ux_utility_semaphore_put(&ux_test_worker_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

static void ux_test_worker_thread_entry(ULONG input)
{

UINT status;


    while (1)
    {

        /* Wait for some work. */
        status = ux_utility_semaphore_get(&ux_test_worker_semaphore, TX_WAIT_FOREVER);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
            test_control_return(1);
        }

        /* Do the work. */
        ux_test_worker_current_work->function(ux_test_worker_current_work->input);
    }
}

static void ux_test_worker_initialize()
{

UINT status;


    status = ux_utility_semaphore_create(&ux_test_worker_semaphore, "ux_test_worker_semaphore", 0);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    status = ux_utility_thread_create(&ux_test_worker_thread, "ux_test_worker_thread", ux_test_worker_thread_entry, 0,
                                      ux_test_worker_thread_stack, UX_DEMO_STACK_SIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

/* Simulate CBI resources. */

UINT _ux_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter);
static TX_SEMAPHORE *cbi_interrupt_endpoint_transfer_semaphore;
static TX_SEMAPHORE cbi_transfer_request_csw_semaphore_host;
static UCHAR        *cbi_host_transfer_csw_data_pointer;
static UINT         global_cbi_fail_on_csw;
static UINT         global_cbi_actual_csw = UX_TRUE;

void ux_cbi_simulator_initialize()
{

UINT status;


    status = ux_utility_semaphore_create(&cbi_transfer_request_csw_semaphore_host, "transfer request csw semaphore device", 0);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

static UX_ENDPOINT *get_host_bulk_endpoint(UCHAR get_bulk_out)
{

UX_ENDPOINT *bulk_out;
UINT        direction = (get_bulk_out == UX_TRUE) ? UX_ENDPOINT_OUT : UX_ENDPOINT_IN;


    bulk_out = global_host_device->ux_device_first_configuration->ux_configuration_first_interface->ux_interface_first_endpoint;
    while (bulk_out)
    {

        if ((bulk_out->ux_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) == direction &&
           (bulk_out->ux_endpoint_descriptor.bmAttributes & UX_MASK_ENDPOINT_TYPE) == UX_BULK_ENDPOINT)
        {
            break;
        }

        bulk_out = bulk_out->ux_endpoint_next_endpoint;
    }
    return bulk_out;
}

static UX_ENDPOINT *get_host_bulk_out_endpoint()
{
    return get_host_bulk_endpoint(UX_TRUE);
}

static UX_ENDPOINT *get_host_bulk_in_endpoint()
{
    return get_host_bulk_endpoint(UX_FALSE);
}

static UINT _ux_hcd_sim_host_entry_bo_to_cbi(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT            status;
UINT            entry_status;
UX_TRANSFER     *transfer_request;
UX_ENDPOINT     *bulk_out;
UX_TRANSFER     *bulk_out_transfer_request;
UCHAR           *cbw;
UCHAR           *cb;
UINT            old_threshold;


    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;

        /* Is this a CBW? */
        if (transfer_request->ux_transfer_request_endpoint->ux_endpoint_descriptor.bmAttributes == 0x0 &&
            transfer_request->ux_transfer_request_type == (UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE))
        {

            /* Right now it's being sent on the control endpoint. Need to switch to bulk. */

            bulk_out = get_host_bulk_out_endpoint();
            bulk_out_transfer_request = &bulk_out->ux_endpoint_transfer_request;

            /* Make the following check and assignment atomic. */
            tx_thread_priority_change(tx_thread_identify(), 0, &old_threshold);

            /* We can only transfer when the device is ATTACHED, ADDRESSED OR CONFIGURED.  */
            if ((global_host_device -> ux_device_state == UX_DEVICE_ATTACHED) || (global_host_device -> ux_device_state == UX_DEVICE_ADDRESSED)
                    || (global_host_device -> ux_device_state == UX_DEVICE_CONFIGURED))

                /* Set the pending transfer request.  */
                bulk_out_transfer_request -> ux_transfer_request_completion_code = UX_TRANSFER_STATUS_PENDING;

            tx_thread_priority_change(tx_thread_identify(), old_threshold, &old_threshold);

            if (bulk_out_transfer_request -> ux_transfer_request_completion_code != UX_TRANSFER_STATUS_PENDING)
                return(UX_TRANSFER_NOT_READY);

            /* Through the power of friendship, we know the CBW is located behind the UFI. */
            cb = transfer_request->ux_transfer_request_data_pointer;
            cbw = cb - UX_HOST_CLASS_STORAGE_CBW_CB;

            bulk_out_transfer_request->ux_transfer_request_data_pointer = cbw;
            bulk_out_transfer_request->ux_transfer_request_requested_length = UX_HOST_CLASS_STORAGE_CBW_LENGTH;

            entry_status = _ux_hcd_sim_host_entry(hcd, function, bulk_out_transfer_request);
            if (entry_status == UX_SUCCESS)
            {

                /* The caller expects this to be blocking since it's a control transfer, so wait for it to complete. */
                status = ux_utility_semaphore_get(&bulk_out_transfer_request->ux_transfer_request_semaphore, TX_WAIT_FOREVER);
                if (status != UX_SUCCESS)
                {

                    printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
                    test_control_return(1);
                }
            }

            transfer_request->ux_transfer_request_completion_code = bulk_out_transfer_request->ux_transfer_request_completion_code;
            transfer_request->ux_transfer_request_status = bulk_out_transfer_request->ux_transfer_request_status;

            return entry_status;
        }

        /* Is this a CSW (only thing that could be on the interrupt endpoint)? */
        else if (transfer_request->ux_transfer_request_endpoint->ux_endpoint_descriptor.bmAttributes == 0x03)
        {

            /* Make the following check and assignment atomic. */
            tx_thread_priority_change(tx_thread_identify(), 0, &old_threshold);

            /* We can only transfer when the device is ATTACHED, ADDRESSED OR CONFIGURED.  */
            if ((global_host_device -> ux_device_state == UX_DEVICE_ATTACHED) || (global_host_device -> ux_device_state == UX_DEVICE_ADDRESSED)
                    || (global_host_device -> ux_device_state == UX_DEVICE_CONFIGURED))

                /* Set the pending transfer request.  */
                transfer_request -> ux_transfer_request_completion_code = UX_TRANSFER_STATUS_PENDING;

            tx_thread_priority_change(tx_thread_identify(), old_threshold, &old_threshold);

            if (transfer_request -> ux_transfer_request_completion_code != UX_TRANSFER_STATUS_PENDING)
                return(UX_TRANSFER_NOT_READY);

            cbi_host_transfer_csw_data_pointer = transfer_request->ux_transfer_request_data_pointer;
            cbi_interrupt_endpoint_transfer_semaphore = &transfer_request->ux_transfer_request_semaphore;

            if (global_cbi_actual_csw == UX_TRUE)
            {

                /* Tell device we've received csw request. */
                status = ux_utility_semaphore_put(&cbi_transfer_request_csw_semaphore_host);
                if (status != UX_SUCCESS)
                {

                    printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
                    test_control_return(1);
                }

                /* Wait for device to copy data. */
                status = ux_utility_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, TX_WAIT_FOREVER);
                if (status != UX_SUCCESS)
                {

                    printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
                    test_control_return(1);
                }

                /* Release semaphore for caller. */
                status = ux_utility_semaphore_put(&transfer_request->ux_transfer_request_semaphore);
                if (status != UX_SUCCESS)
                {

                    printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
                    test_control_return(1);
                }
            }
            else
            {

                global_cbi_actual_csw = UX_TRUE;
            }

            transfer_request->ux_transfer_request_completion_code = UX_SUCCESS;
            transfer_request->ux_transfer_request_status = UX_TRANSFER_STATUS_COMPLETED;

            if (global_cbi_fail_on_csw == UX_TRUE)
            {

                global_cbi_fail_on_csw = UX_FALSE;
                return(UX_ERROR);
            }

            return UX_SUCCESS;
        }
    }

    return _ux_hcd_sim_host_entry(hcd, function, parameter);
}

static UINT _ux_dcd_sim_slave_function_bo_to_cbi(UX_SLAVE_DCD *dcd, UINT function, VOID *parameter)
{

UINT                status;
ULONG               mask;
UX_SLAVE_TRANSFER   *transfer_request;


    if (function == UX_DCD_TRANSFER_REQUEST)
    {

#if 0
        /* See matching #if block in host bo_to_cbi. */
#else
        transfer_request = parameter;

        mask = ux_utility_long_get(&transfer_request->ux_slave_transfer_request_data_pointer[UX_SLAVE_CLASS_STORAGE_CSW_SIGNATURE]);

        /* Is this a CSW? */
        if (mask == UX_SLAVE_CLASS_STORAGE_CSW_SIGNATURE_MASK)
        {

            /* Wait for host to send CSW request. */
            status = ux_utility_semaphore_get(&cbi_transfer_request_csw_semaphore_host, TX_WAIT_FOREVER);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
                test_control_return(1);
            }

            /* Copy CSW to host. */
            ux_utility_memory_copy(cbi_host_transfer_csw_data_pointer,
                transfer_request->ux_slave_transfer_request_current_data_pointer, UX_SLAVE_CLASS_STORAGE_CSW_LENGTH);

            /* Tell host we've copied data */
            status = ux_utility_semaphore_put(cbi_interrupt_endpoint_transfer_semaphore);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
                test_control_return(1);
            }

            transfer_request->ux_slave_transfer_request_completion_code = UX_SUCCESS;
            transfer_request->ux_slave_transfer_request_status = UX_TRANSFER_STATUS_COMPLETED;

            return UX_SUCCESS;
        }
#endif
    }

    return _ux_dcd_sim_slave_function(dcd, function, parameter);
}

/* Simulate CB resources. */

UINT _ux_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter);
static UINT _ux_hcd_sim_host_entry_bo_to_cb(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT            status;
UINT            entry_status;
UX_TRANSFER     *transfer_request;
UX_ENDPOINT     *bulk_out;
UX_TRANSFER     *bulk_out_transfer_request;
UCHAR           *cbw;
UCHAR           *cb;
UINT            old_threshold;


    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;

        /* Is this a CBW? */
        if (transfer_request->ux_transfer_request_type == (UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE))
        {

            /* Right now it's being sent on the control endpoint. Need to switch to bulk. */

            bulk_out = global_host_device->ux_device_first_configuration->ux_configuration_first_interface->ux_interface_first_endpoint->ux_endpoint_next_endpoint;
            bulk_out_transfer_request = &bulk_out->ux_endpoint_transfer_request;

            /* Make the following check and assignment atomic. */
            tx_thread_priority_change(tx_thread_identify(), 0, &old_threshold);

            /* We can only transfer when the device is ATTACHED, ADDRESSED OR CONFIGURED.  */
            if ((global_host_device -> ux_device_state == UX_DEVICE_ATTACHED) || (global_host_device -> ux_device_state == UX_DEVICE_ADDRESSED)
                    || (global_host_device -> ux_device_state == UX_DEVICE_CONFIGURED))

                /* Set the pending transfer request.  */
                bulk_out_transfer_request -> ux_transfer_request_completion_code = UX_TRANSFER_STATUS_PENDING;

            tx_thread_priority_change(tx_thread_identify(), old_threshold, &old_threshold);

            if (transfer_request -> ux_transfer_request_completion_code != UX_TRANSFER_STATUS_PENDING)
                return(UX_TRANSFER_NOT_READY);

            /* Through the power of friendship, we know the CBW is located behind the UFI. */
            cb = transfer_request->ux_transfer_request_data_pointer;
            cbw = cb - UX_HOST_CLASS_STORAGE_CBW_CB;

            bulk_out_transfer_request->ux_transfer_request_data_pointer = cbw;
            bulk_out_transfer_request->ux_transfer_request_requested_length = UX_HOST_CLASS_STORAGE_CBW_LENGTH;

            entry_status = _ux_hcd_sim_host_entry(hcd, function, bulk_out_transfer_request);

            if (entry_status == UX_SUCCESS)
            {

                /* The caller expects this to be blocking since it's a control transfer, so wait for it to complete. */
                status = ux_utility_semaphore_get(&bulk_out_transfer_request->ux_transfer_request_semaphore, TX_WAIT_FOREVER);
                if (status != UX_SUCCESS)
                {

                    printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
                    test_control_return(1);
                }
            }

            transfer_request->ux_transfer_request_completion_code = bulk_out_transfer_request->ux_transfer_request_completion_code;
            transfer_request->ux_transfer_request_status = bulk_out_transfer_request->ux_transfer_request_status;

            return entry_status;
        }
    }

    return _ux_hcd_sim_host_entry(hcd, function, parameter);
}

static UINT _ux_dcd_sim_slave_function_bo_to_cb(UX_SLAVE_DCD *dcd, UINT function, VOID *parameter)
{

UX_SLAVE_TRANSFER   *transfer_request;
ULONG               mask;


    if (function == UX_DCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;

        mask = ux_utility_long_get(&transfer_request->ux_slave_transfer_request_data_pointer[UX_SLAVE_CLASS_STORAGE_CSW_SIGNATURE]);

        /* Is this a CSW? */
        if (mask == UX_SLAVE_CLASS_STORAGE_CSW_SIGNATURE_MASK)
        {

            /* Sike, that's the wrong number! */
            /* Ignore cause there is no CSW in CB protocol. */

            transfer_request->ux_slave_transfer_request_completion_code = UX_SUCCESS;
            transfer_request->ux_slave_transfer_request_status = UX_TRANSFER_STATUS_COMPLETED;

            return UX_SUCCESS;
        }
    }

    return _ux_dcd_sim_slave_function(dcd, function, parameter);
}

static void format_ram_disk()
{

UINT status;


    /* We need to close media before formatting. */
    status = fx_media_close(&global_ram_disk);
    if (status != FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    status =  fx_media_format(&global_ram_disk, _fx_ram_driver, global_ram_disk_memory, global_ram_disk_working_buffer, global_sector_size, "RAM DISK", 2, UX_TEST_NUM_DIRECTORY_ENTRIES, 0, UX_RAM_DISK_SIZE/global_sector_size, global_sector_size, 4, 1, 1);
    if (status != FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    status =  fx_media_open(&global_ram_disk, "RAM DISK", _fx_ram_driver, global_ram_disk_memory, global_ram_disk_working_buffer, sizeof(global_ram_disk_working_buffer));
    if (status != FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

/* Disconnection and connection functions. Note that these should be the only ways you disconnect and connect, for the
   sake of 'funneling'. */

/* This function should not be called from inside USBX because we wait for the deactivation to finish. If we have a
   semaphore the deactivation routine requires, then we have deadlock.
   Note that we format the ram disk during connection because global_sector_size uses changes after we call
   disconnect(). */
static void disconnect_host_and_slave()
{

    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect_no_wait();

    /* Wait for disconnect to finish. */
    while (global_hcd->ux_hcd_nb_devices != 0)
        tx_thread_sleep(10);

    /* Do a memory check. */

    /* Has the value not been initialized yet? */
    if (global_memory_test_no_device_memory_free_amount == 0)
        return;

    if (_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available != global_memory_test_no_device_memory_free_amount)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
}

static void disconnect_host_and_slave_work_func(ULONG input)
{

    disconnect_host_and_slave();
}

static UX_TEST_WORKER_WORK global_disconnect_host_and_slave_work = { 0, disconnect_host_and_slave_work_func };
static void disconnect_host_and_slave_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;


    ux_test_worker_add_work(&global_disconnect_host_and_slave_work);

    /* Allow enum thread to run. This is what we want to have happen, like, all of the time. */
    tx_thread_relinquish();
}

/* Note: This assumes the enum thread is already running AKA enum thread's semaphore
   was put() before. */
static UINT wait_for_enum_completion_and_get_global_storage_values()
{

UINT status;


    /* Should already be running! */
    UX_TEST_ASSERT(_ux_system_host -> ux_system_host_enum_semaphore.tx_semaphore_suspended_count == 0);

    /* Wait for enum thread to complete. */
    while (_ux_system_host -> ux_system_host_enum_semaphore.tx_semaphore_suspended_count == 0)
        tx_thread_sleep(10);

    status = ux_host_stack_class_get(_ux_system_host_class_storage_name, &global_host_storage_class);
    UX_TEST_ASSERT(status == UX_SUCCESS);

    status = ux_host_stack_class_instance_get(global_host_storage_class, 0, (void **) &global_storage);
    if (status)
        return status;

    global_host_class_ext = global_host_storage_class -> ux_host_class_ext;

    if (global_storage -> ux_host_class_storage_state != UX_HOST_CLASS_INSTANCE_LIVE ||
        global_host_class_ext == UX_NULL ||
        global_host_storage_class -> ux_host_class_media == UX_NULL)
        return UX_ERROR;

    global_storage_medias = global_host_storage_class->ux_host_class_media;
    global_storage_media = &global_storage_medias[0];
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    global_media =  &global_storage_media->ux_host_class_storage_media;
#endif
    global_storage_thread = &global_host_class_ext->ux_host_class_thread;

    return status;
}

/* Returns whether or not the enumeration succeeded. */
static UINT connect_host_and_slave()
{

UINT status;


    /* The worker thread might be in the middle of waiting for the disconnect
       to complete. Don't confuse it! */
    while (ux_test_worker_semaphore.tx_semaphore_suspended_count == 0)
        tx_thread_sleep(10);

    /* During the basic test, although we delete the file, it seems to stay
       on the disk. Clear everything and reformat the disk (reformatting doesn't
       clear everything). */
    memset(global_ram_disk_memory, 0, sizeof(global_ram_disk_memory));
    format_ram_disk();

    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);

    /* No wait because wait_for_enum...() expects the enum thread to be running
       when it's called. */
    ux_test_hcd_sim_host_connect_no_wait(UX_FULL_SPEED_DEVICE);

    status = wait_for_enum_completion_and_get_global_storage_values();

    return status;
}

static void switch_to_protocol(UCHAR protocol)
{
    switch (protocol)
    {

    case UX_HOST_CLASS_STORAGE_PROTOCOL_BO:
            device_framework_full_speed[bInterfaceProtocol_POS] = UX_HOST_CLASS_STORAGE_PROTOCOL_BO;
            device_framework_high_speed[bInterfaceProtocol_POS] = UX_HOST_CLASS_STORAGE_PROTOCOL_BO;
            device_framework_full_speed[bInterfaceSubClass_POS] = UX_HOST_CLASS_STORAGE_SUBCLASS_SCSI;
            device_framework_high_speed[bInterfaceSubClass_POS] = UX_HOST_CLASS_STORAGE_SUBCLASS_SCSI;
            global_hcd->ux_hcd_entry_function = _ux_test_hcd_sim_host_entry;
            global_dcd->ux_slave_dcd_function = _ux_test_dcd_sim_slave_function;
        break;

    case UX_HOST_CLASS_STORAGE_PROTOCOL_CBI:
            device_framework_full_speed[bInterfaceProtocol_POS] = UX_HOST_CLASS_STORAGE_PROTOCOL_CBI;
            device_framework_high_speed[bInterfaceProtocol_POS] = UX_HOST_CLASS_STORAGE_PROTOCOL_CBI;
            device_framework_full_speed[bInterfaceSubClass_POS] = UX_HOST_CLASS_STORAGE_SUBCLASS_UFI;
            device_framework_high_speed[bInterfaceSubClass_POS] = UX_HOST_CLASS_STORAGE_SUBCLASS_UFI;
            global_hcd->ux_hcd_entry_function = _ux_hcd_sim_host_entry_bo_to_cbi;
            global_dcd->ux_slave_dcd_function = _ux_dcd_sim_slave_function_bo_to_cbi;
        break;

    case UX_HOST_CLASS_STORAGE_PROTOCOL_CB:
            device_framework_full_speed[bInterfaceProtocol_POS] = UX_HOST_CLASS_STORAGE_PROTOCOL_CB;
            device_framework_high_speed[bInterfaceProtocol_POS] = UX_HOST_CLASS_STORAGE_PROTOCOL_CB;
            device_framework_full_speed[bInterfaceSubClass_POS] = UX_HOST_CLASS_STORAGE_SUBCLASS_UFI;
            device_framework_high_speed[bInterfaceSubClass_POS] = UX_HOST_CLASS_STORAGE_SUBCLASS_UFI;
            global_hcd->ux_hcd_entry_function = _ux_hcd_sim_host_entry_bo_to_cb;
            global_dcd->ux_slave_dcd_function = _ux_dcd_sim_slave_function_bo_to_cb;
        break;
    }
}

/* For restoring state for next test. */
static void reset_to_bo()
{

    disconnect_host_and_slave();

    global_sector_size = UX_HOST_CLASS_STORAGE_SECTOR_SIZE_FAT;

    switch_to_protocol(UX_HOST_CLASS_STORAGE_PROTOCOL_BO);

    bulk_in_endpoint_descriptor_fs[Endpoint_bEndpointAddress] = 0x81;
    bulk_in_endpoint_descriptor_hs[Endpoint_bEndpointAddress] = 0x81;
    bulk_in_endpoint_descriptor_fs[Endpoint_bmAttributes] = 0x02;
    bulk_in_endpoint_descriptor_hs[Endpoint_bmAttributes] = 0x02;

    bulk_out_endpoint_descriptor_fs[Endpoint_bEndpointAddress] = 0x02;
    bulk_out_endpoint_descriptor_hs[Endpoint_bEndpointAddress] = 0x02;
    bulk_out_endpoint_descriptor_fs[Endpoint_bmAttributes] = 0x02;
    bulk_out_endpoint_descriptor_hs[Endpoint_bmAttributes] = 0x02;

    interrupt_in_endpoint_descriptor_fs[Endpoint_bEndpointAddress] = 0x83;
    interrupt_in_endpoint_descriptor_hs[Endpoint_bEndpointAddress] = 0x83;
    interrupt_in_endpoint_descriptor_fs[Endpoint_bmAttributes] = 0x03;
    interrupt_in_endpoint_descriptor_hs[Endpoint_bmAttributes] = 0x03;

    global_persistent_slave_storage->ux_slave_class_storage_number_lun = 1;
    global_persistent_slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_type = UX_HOST_CLASS_STORAGE_MEDIA_FAT_DISK;
    global_persistent_slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_removable_flag = 0x80;
    global_persistent_slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_block_length = global_sector_size;

    connect_host_and_slave();
}

/* This function is called when the device tries to read/write to the device. We return an error so that transfer
   succeeds, but the CSW contains an error (this is for the multiple read retries test). */
static UINT fx_media_read_write_test_error_action_func(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

    *media_status = (ULONG)-1;
    return UX_ERROR;
}

UX_TEST_SETUP global_setup_endpoint_reset_in1 =
{
    UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_ENDPOINT,
    UX_CLEAR_FEATURE,
    UX_ENDPOINT_HALT,
    0x81
};

UX_TEST_SETUP global_setup_endpoint_reset_out2 =
{
    UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_ENDPOINT,
    UX_CLEAR_FEATURE,
    UX_ENDPOINT_HALT,
    0x02
};

UX_TEST_SETUP global_setup_mass_storage_reset =
{
    UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE,
    UX_HOST_CLASS_STORAGE_RESET,
    0,
    0
};

/* Action creation functions. */

static void ux_test_memory_allocate_all_memory_action_func(UX_TEST_ACTION *action, VOID *_params)
{

    ux_test_utility_sim_mem_allocate_until_align_flagged(0, 0, action->memory_cache_flag);
}

static UX_TEST_ACTION create_disconnect_on_thread_preemption_change(TX_THREAD *thread_to_match, TX_THREAD *preemption_change_param_thread_ptr, UINT preemption_change_param_new_threshold)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_TX_THREAD_PREEMPTION_CHANGE;
    action.thread_ptr = preemption_change_param_thread_ptr;
    action.new_threshold = preemption_change_param_new_threshold;
    action.action_func = disconnect_host_and_slave_action_func;
    action.thread_to_match = thread_to_match;
    action.do_after = 1;

    return action;
}

static VOID device_media_write_block_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS *params = (UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS *)_params;


    /* 100% of the time, we're doing this block so we can hit a transfer timeout branch. The problem is that the
       timeouts can take a while, so we abort the waits prematurely from here since the test thread (and therefore host)
       are blocked. */

DEVICE_MEDIA_READ_WRITE_TIMEOUT_DATA    *timeout_data = (DEVICE_MEDIA_READ_WRITE_TIMEOUT_DATA *)action->user_data;
TX_SEMAPHORE                            *semaphore = &timeout_data->transfer_request->ux_transfer_request_semaphore;


    while (timeout_data->num_timeouts--)
    {

        /* Wait for host thread to wait on the semaphore. */
        while (semaphore->tx_semaphore_suspended_count == 0)
            tx_thread_sleep(10);

        UX_TEST_ASSERT(semaphore->tx_semaphore_suspension_list != UX_NULL);

        /* Now simulate a timeout. */
        UX_TEST_CHECK_SUCCESS(tx_thread_wait_abort(semaphore->tx_semaphore_suspension_list));
    }

    /* It's unrealistic for us the complete the write _right_ after the host timeouts. Therefore, we sleep a little
       longer. It's up to the user to unblock the CSW we do after.
       Note: I got this sleep value from ux_hcd_sim_host_transfer_abort.c: it's 10x the 1 ms it does. */
    ux_utility_delay_ms(10*1);
}

static UX_TEST_ACTION create_device_media_write_block_action(DEVICE_MEDIA_READ_WRITE_TIMEOUT_DATA *timeout_data)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_WRITE;
    action.ignore_params = 1;
    action.no_return = 0;
    action.status = UX_ERROR;
    action.action_func = device_media_write_block_action_func;
    action.user_data = (ALIGN_TYPE)timeout_data;

    return(action);
}

static VOID device_media_read_write_media_status_error_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS *params = (UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS *)_params;


    *params->media_status = ~0;
}

static VOID device_media_status_error_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_STATUS_PARAMS *params = (UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_STATUS_PARAMS *)_params;


    *params->media_status = ~0;
}

static UX_TEST_ACTION create_device_media_read_fail_action()
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ;
    action.ignore_params = 1;
    action.action_func = device_media_read_write_media_status_error_action_func;
    action.no_return = 0;
    action.status = UX_ERROR;

    return action;
}

static UX_TEST_ACTION create_device_media_write_fail_action()
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_WRITE;
    action.ignore_params = 1;
    action.action_func = device_media_read_write_media_status_error_action_func;
    action.no_return = 0;
    action.status = UX_ERROR;

    return action;
}

static void configuration_reset_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UINT                                status;


    status = ux_host_stack_device_configuration_reset(global_host_device);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

static UX_TEST_ACTION create_disconnect_on_media_characteristics_get_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_data = global_cbw_data_media_characteristics_get_sbc;
    action.req_actual_len = sizeof(global_cbw_data_media_characteristics_get_sbc);
    action.action_func = disconnect_host_and_slave_action_func;
    action.thread_to_match = thread_to_match;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_format_capacity_get_match_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = UX_HOST_CLASS_STORAGE_READ_FORMAT_RESPONSE_LENGTH;
    action.thread_to_match = thread_to_match;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_disconnect_on_transfer_data_match_action(TX_THREAD *thread_to_match, UCHAR *data, UINT data_size)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_data = data;
    action.req_actual_len = data_size;
    action.action_func = disconnect_host_and_slave_action_func;
    action.thread_to_match = thread_to_match;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_allocate_all_memory_on_transfer_data_match_action(TX_THREAD *thread_to_match, ULONG memory_cache_flag, UCHAR *data, UINT data_size)
{

UX_TEST_ACTION action = { 0 };


    action = create_disconnect_on_transfer_data_match_action(thread_to_match, data, data_size);
    action.memory_cache_flag = memory_cache_flag;
    action.action_func = ux_test_memory_allocate_all_memory_action_func;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_setup_match_action(TX_THREAD *thread_to_match, UX_TEST_SETUP *req_setup)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_SETUP_MATCH_REQ_V_I;
    action.req_setup = req_setup;
    action.thread_to_match = thread_to_match;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_mass_storage_reset_match_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action = create_setup_match_action(thread_to_match, &global_setup_mass_storage_reset);

    return action;
}

static UX_TEST_ACTION create_disconnect_on_mass_storage_reset_match_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action = create_setup_match_action(thread_to_match, &global_setup_mass_storage_reset);
    action.action_func = disconnect_host_and_slave_action_func;

    return action;
}

static UX_TEST_ACTION create_error_match_action_from_error(UX_TEST_ERROR_CALLBACK_ERROR error)
{

UX_TEST_ACTION action = create_error_match_action(error.system_level, error.system_context, error.error_code);


    return action;
}

static UX_TEST_ACTION create_memory_allocation_fail_error_match_action()
{

UX_TEST_ACTION action = create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_UTILITY, UX_MEMORY_INSUFFICIENT);


    return action;
}

/* Note: do_after and do_before refer to when the action func is called relative to the HCD function. */
/* This is to be used as an action func. It simply sets the completion code of a transfer to UX_ERROR */
static void async_transfer_completion_code_error_action_func_do_after(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UINT                                status;
UX_TRANSFER                         *transfer = params->parameter;


    status = ux_utility_semaphore_get(&transfer->ux_transfer_request_semaphore, TX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    transfer->ux_transfer_request_completion_code = UX_ERROR;

    status = ux_utility_semaphore_put(&transfer->ux_transfer_request_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

/* This changes the transfer completion code before the HCD is called. */
static void async_transfer_completion_code_success_action_func_do_before(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UINT                                status;
UX_TRANSFER                         *transfer_request = params->parameter;


    transfer_request->ux_transfer_request_completion_code = UX_SUCCESS;

    /* Since we're not calling the HCD, we don't wait for the transfer to complete via a sempahore_get(). However,
       the caller is still going to wait on the semaphore, so make sure we put() it. */
    status = ux_utility_semaphore_put(&transfer_request -> ux_transfer_request_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n",__LINE__,(ULONG)status);
        test_control_return(1);
    }
}

static UX_TEST_ACTION create_semaphore_get_match_action(TX_THREAD *thread, TX_SEMAPHORE *semaphore, ULONG semaphore_signal)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_TX_SEMAPHORE_GET;
    action.semaphore_ptr = semaphore;
    action.wait_option = semaphore_signal;
    action.thread_to_match = thread;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_semaphore_get_disconnect_action(TX_THREAD *thread, TX_SEMAPHORE *semaphore, ULONG semaphore_signal)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_TX_SEMAPHORE_GET;
    action.semaphore_ptr = semaphore;
    action.wait_option = semaphore_signal;
    action.thread_to_match = thread;
    action.action_func = disconnect_host_and_slave_action_func;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_test_unit_ready_match_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_data = global_cbw_data_unit_ready_test_sbc;
    action.req_actual_len = sizeof(global_cbw_data_unit_ready_test_sbc);
    action.no_return = 1;
    action.thread_to_match = thread_to_match;

    return action;
}

static UX_TEST_ACTION create_semaphore_get_fail_with_check_override_action(TX_THREAD *thread, TX_SEMAPHORE *semaphore, ULONG semaphore_signal, UCHAR (*check)())
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_TX_SEMAPHORE_GET;
    action.semaphore_ptr = semaphore;
    action.wait_option = semaphore_signal;
    action.thread_to_match = thread;
    action.check_func = check;
    action.no_return = 1;

    return action;
}

static void async_transport_stall_test_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UINT                                status;
UX_TRANSFER                         *transfer_request = params->parameter;


    transfer_request->ux_transfer_request_completion_code = UX_TRANSFER_STALLED;

    /* Caller is waiting for transfer completion. */
    status = ux_utility_semaphore_put(&transfer_request->ux_transfer_request_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
}

static void create_request_sense_error_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UINT                                status;
UX_TRANSFER                         *transfer_request = params->parameter;


    /* Wait for transfer to complete. */
    status = ux_utility_semaphore_get(&transfer_request->ux_transfer_request_semaphore, TX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    transfer_request->ux_transfer_request_data_pointer[UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_SENSE_KEY] = (UCHAR)action->user_data;
    transfer_request->ux_transfer_request_completion_code = UX_SUCCESS;

    /* Signal caller that transfer is complete. */
    status = ux_utility_semaphore_put(&transfer_request->ux_transfer_request_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

static UX_TEST_ACTION create_request_sense_error_action(TX_THREAD *thread_to_match, UINT request_sense_code)
{

UX_TEST_ACTION action = { 0 };


    /* For making Request Sense contain error. */
    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH;
    action.action_func = create_request_sense_error_action_func;
    action.do_after = 1;
    action.no_return = 1;
    action.thread_to_match = thread_to_match;
    action.user_data = request_sense_code;

    return action;
}

/* Action function for making the CSW contain an error. */
static void csw_error_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UINT                                status;
UX_TRANSFER                         *transfer_request = params->parameter;


    /* Wait for transfer to complete. */
    status = ux_utility_semaphore_get(&transfer_request->ux_transfer_request_semaphore, TX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    transfer_request->ux_transfer_request_data_pointer[UX_HOST_CLASS_STORAGE_CSW_STATUS] = (UCHAR)action->user_data;
    transfer_request->ux_transfer_request_completion_code = UX_SUCCESS;

    /* Signal caller that transfer is complete. */
    status = ux_utility_semaphore_put(&transfer_request->ux_transfer_request_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

/* Action function for skipping transfer. */
static void skip_transfer_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UINT                                status;
UX_TRANSFER                         *transfer_request = params->parameter;


    /* Caller is gonna wait! */
    status = ux_utility_semaphore_put(&transfer_request->ux_transfer_request_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    transfer_request->ux_transfer_request_completion_code = UX_SUCCESS;
}

static UX_TEST_ACTION create_endpoint_reset_match_action(TX_THREAD *thread_to_match, UX_TEST_SETUP *setup)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_SETUP_MATCH_REQ_V_I;
    action.no_return = 0; /* Host side tests, the actual request is not sent.  */
                          /* TODO: modify testing to make changes in device side.  */
    action.req_setup = setup;
    action.thread_to_match = thread_to_match;

    return action;
}

/* Sets the CSW status to the value specific ('error'). Note: the reason we add the thread to match is because of the
   pesky little background storage thread. */
static UX_TEST_ACTION create_csw_error_action(TX_THREAD *thread_to_match, UINT error)
{

UX_TEST_ACTION action = { 0 };


    /* For making CSW contain error. */
    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = UX_HOST_CLASS_STORAGE_CSW_LENGTH;
    action.action_func = csw_error_action_func;
    action.do_after = 1;
    action.no_return = 1;
    action.thread_to_match = thread_to_match;
    action.user_data = error;

    return action;
}

static UX_TEST_ACTION create_data_match_action(TX_THREAD *thread_to_match, UCHAR *data, ULONG data_length)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_data = data;
    action.req_actual_len = data_length;
    action.thread_to_match = thread_to_match;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_csw_match_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = UX_HOST_CLASS_STORAGE_CSW_LENGTH;
    action.thread_to_match = thread_to_match;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_csw_skip_hcd_no_put_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = UX_HOST_CLASS_STORAGE_CSW_LENGTH;
    action.thread_to_match = thread_to_match;
    action.no_return = 0;
    action.status = UX_SUCCESS;

    return action;
}

static UX_TEST_ACTION create_csw_skip_hcd_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = UX_HOST_CLASS_STORAGE_CSW_LENGTH;
    action.thread_to_match = thread_to_match;
    action.action_func = skip_transfer_action_func;
    action.no_return = 0;
    action.status = UX_SUCCESS;

    return action;
}

static UX_TEST_ACTION create_transfer_requested_length_match_action(TX_THREAD *thread_to_match, ULONG length_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = length_to_match;
    action.thread_to_match = thread_to_match;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_data_phase_sector_size_match_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action = create_transfer_requested_length_match_action(thread_to_match, UX_TEST_DEFAULT_SECTOR_SIZE);

    return action;
}

static UX_TEST_ACTION create_data_phase_sector_size_stall_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action = create_data_phase_sector_size_match_action(thread_to_match);
    action.action_func = async_transport_stall_test_action_func;
    action.do_after = 0;
    action.no_return = 0;
    action.status = UX_SUCCESS;
    action.thread_to_match = thread_to_match;

    return action;
}

static UX_TEST_ACTION create_disconnect_on_sector_size_transfer_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action = create_data_phase_sector_size_match_action(thread_to_match);
    action.action_func = disconnect_host_and_slave_action_func;

    return action;
}

static UX_TEST_ACTION create_cbw_match_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = UX_HOST_CLASS_STORAGE_CBW_LENGTH;
    action.thread_to_match = thread_to_match;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_timeout_on_cbw_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    /* We do this by just not sending the request to the device. */

    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = UX_HOST_CLASS_STORAGE_CBW_LENGTH;
    action.thread_to_match = thread_to_match;
    action.no_return = 0;

    return action;
}

static UX_TEST_ACTION create_timeout_on_transfer_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    /* We do this by just not sending the request to the device. */

    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.thread_to_match = thread_to_match;
    action.no_return = 0;

    return action;
}

static UX_TEST_ACTION create_cbw_opcode_match_action(TX_THREAD *thread_to_match, UCHAR opcode)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = 0;
    /* Opcode is right after CBW. */
    action.req_actual_len = UX_HOST_CLASS_STORAGE_CBW_CB + 1;
    action.thread_to_match = thread_to_match;
    action.no_return = 1;

    action.req_data_match_mask = global_cbw_opcode_match_mask;

    switch (opcode)
    {
        case UX_HOST_CLASS_STORAGE_SCSI_READ16:
            action.req_data = global_cbw_opcode_read;
            break;

        default:
            UX_TEST_ASSERT(0);
    }

    return action;
}

static  UX_TEST_ACTION create_disconnect_on_requested_length_action(TX_THREAD *thread_to_match, UINT requested_length)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = requested_length;
    action.action_func = disconnect_host_and_slave_action_func;
    action.thread_to_match = thread_to_match;
    action.no_return = 1;

    return action;
}

static  UX_TEST_ACTION create_config_reset_on_requested_length_action(TX_THREAD *thread_to_match, UINT requested_length)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = requested_length;
    action.action_func = configuration_reset_action_func;
    action.thread_to_match = thread_to_match;
    action.no_return = 1;

    return action;
}

static UX_TEST_ACTION create_cbw_disconnect_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = create_disconnect_on_requested_length_action(thread_to_match, UX_HOST_CLASS_STORAGE_CBW_LENGTH);


    return action;
}

static UX_TEST_ACTION create_cbw_config_reset_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = create_config_reset_on_requested_length_action(thread_to_match, UX_HOST_CLASS_STORAGE_CBW_LENGTH);


    return action;
}

static UX_TEST_ACTION create_csw_disconnect_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = create_disconnect_on_requested_length_action(thread_to_match, UX_HOST_CLASS_STORAGE_CSW_LENGTH);


    return action;
}

/* This will match a CSW and call the supplied action function. */
static UX_TEST_ACTION create_csw_match_action_with_func(VOID (*entry_func_action)(UX_TEST_ACTION *action, VOID *params), TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = UX_HOST_CLASS_STORAGE_CSW_LENGTH;
    action.action_func = entry_func_action;
    action.do_after = 1;
    action.thread_to_match = thread_to_match;

    return action;
}

/* Note: This should be a last resort.

   This will match a CSW and force the transfer completion to stall. Note that this returns before the HCD is called
   This because if the transfer makes it to the device, the device will send the CSW, host will see the stall, ask for
   another, but won't get it. */
static UX_TEST_ACTION create_csw_stall_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = UX_HOST_CLASS_STORAGE_CSW_LENGTH;
    action.action_func = async_transport_stall_test_action_func;
    action.do_after = 0;
    action.thread_to_match = thread_to_match;

    return action;
}

static UX_TEST_ACTION create_cbw_stall_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_REQ_LEN;
    action.req_requested_len = UX_HOST_CLASS_STORAGE_CBW_LENGTH;
    action.action_func = async_transport_stall_test_action_func;
    action.do_after = 0;
    action.thread_to_match = thread_to_match;

    return action;
}

static UX_TEST_ACTION create_device_media_status_fail_action()
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_STATUS;
    action.ignore_params = 1;
    action.action_func = device_media_status_error_action_func;
    action.no_return = 0;
    action.status = UX_ERROR;

    return action;
}

static VOID add_multiple_read_retries_fails_actions(UX_TEST_ACTION *user_list, TX_THREAD *thread_to_match)
{

UINT    i;


    for (i = 0; i < UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RETRY; i++)
    {
        /* Remember, entire read has to succeed to storage -> ux_host_class_storage_data_phase_length is valid. */

        /* For safety, match the read CBW. */
        ux_test_add_action_to_user_list(user_list, create_cbw_opcode_match_action(thread_to_match, UX_HOST_CLASS_STORAGE_SCSI_READ16));

        /* We only send the request sense if the CSW contains an error.  */
        ux_test_add_action_to_user_list(user_list, create_csw_error_action(thread_to_match, UX_HOST_CLASS_STORAGE_CSW_FAILED));

        ux_test_add_action_to_user_list(user_list, create_request_sense_error_action(thread_to_match, UX_ERROR));
    }
}

static VOID add_multiple_write_retries_fails_actions(UX_TEST_ACTION *user_list)
{

UINT    i;


    for (i = 0; i < UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RETRY; i++)
    {
        ux_test_add_action_to_user_list(user_list, create_device_media_write_fail_action());
        ux_test_add_action_to_user_list(user_list, create_error_match_action_from_error(global_transfer_stall_error));
    }
}

static VOID add_multiple_status_retries_fails_actions(UX_TEST_ACTION *user_list)
{

UINT    i;


    for (i = 0; i < UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RETRY; i++)
    {
        ux_test_add_action_to_user_list(user_list, create_device_media_status_fail_action());
        ux_test_add_action_to_user_list(user_list, create_error_match_action_from_error(global_transfer_stall_error));
    }
}

/* Action creation functions END. */

/* Action addition utilities. */

static void add_unit_ready_test_not_ready_actions(UX_TEST_ACTION *actions)
{

    ux_test_add_action_to_user_list(actions, create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED));
    ux_test_add_action_to_user_list(actions, create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_NOT_READY));
}

static void initialize_test_resources()
{

UINT status;

#if defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    /* Reset media IDs.  */
    for (int i = 0; i < UX_HOST_CLASS_STORAGE_MAX_MEDIA; i ++)
        _ux_host_class_storage_driver_media(i)->fx_media_id = 0;
#endif

    status = ux_utility_semaphore_create(&global_test_semaphore, "test_semaphore", 0);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    ux_cbi_simulator_initialize();
    ux_test_worker_initialize();
}

static void basic_read_write_test(FX_FILE *file)
{

UINT    status;
ULONG   total_length;
UCHAR   buffer_pattern;
ULONG   bytes_read;
UINT    i;


    /* Set the file length.  */
    total_length = UX_DEMO_FILE_SIZE;

    /* Set pattern first letter.  */
    buffer_pattern = 'a';

    /* Seek to the beginning to copy over an existing file.  */
    status = fx_file_seek(file, 0);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    while (total_length !=0)
    {

        /* Set the buffer with pattern.  */
        ux_utility_memory_set(global_buffer, buffer_pattern, UX_DEMO_FILE_BUFFER_SIZE);

        /* Copy the file in blocks */
        status = fx_file_write(file, global_buffer, UX_DEMO_FILE_BUFFER_SIZE);

        /* Check if status OK.  */
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
            test_control_return(1);
        }

        /* Decrement the length remaining. */
        total_length -= UX_DEMO_FILE_BUFFER_SIZE;

        /* Next pattern.  */
        buffer_pattern++;

        /* Check pattern end.  */
        if (buffer_pattern > 'z')

            /* Back to beginning.  */
            buffer_pattern = 'a';
    }

    /* Seek to the beginning to read.  */
    status = fx_file_seek(file, 0);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Set the file length.  */
    total_length = UX_DEMO_FILE_SIZE;

    /* Set pattern first letter.  */
    buffer_pattern = 'a';

    while(total_length !=0)
    {

        /* Read the file in blocks */
        status = fx_file_read(file, global_buffer, UX_DEMO_FILE_BUFFER_SIZE, &bytes_read);

        /* Check if status OK.  */
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
            test_control_return(1);
        }

        if (bytes_read != UX_DEMO_FILE_BUFFER_SIZE)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        /* Do compare. */
        for (i = 0; i < UX_DEMO_FILE_BUFFER_SIZE; i++)
        {

            if (global_buffer[i] != buffer_pattern)
            {

                printf("Error on line %d\n", __LINE__);
                test_control_return(1);
            }
        }

        /* Decrement the length remaining. */
        total_length -= UX_DEMO_FILE_BUFFER_SIZE;

        /* Next pattern.  */
        buffer_pattern++;

        /* Check pattern end.  */
        if (buffer_pattern > 'z')

            /* Back to beginning.  */
            buffer_pattern = 'a';
    }
}

/* Performs basic test including deletion, creation, open, writing, and reading.  */
static void basic_test_pre_existing_file(UINT iterations, char *message, FX_FILE *file)
{

    if (message != UX_NULL)
    {
        stepinfo("%s", message);
    }

    while (iterations--)
        basic_read_write_test(file);
}

/* Performs basic test including deletion, creation, open, writing, and reading.  */
static void basic_test(FX_MEDIA *media, UINT iterations, char *message)
{

UINT                    status;
FX_FILE                 file;


    if (message != UX_NULL)
    {
        stepinfo("%s", message);
    }

    while (iterations--)
    {

        /* Try to delete the non-existent target file.  */
        status =  fx_file_delete(media, "FILE.CPY");
        if (status != FX_NOT_FOUND)
        {

            printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
            test_control_return(1);
        }

        /* Create and open file. */
        ux_test_storage_file_co(media, &file, UX_TRUE);

        basic_read_write_test(&file);

        /* Close and delete file. */
        ux_test_storage_file_cd(media, &file);
    }
}

/* This is for use in the tests array. */
static void basic_test_with_globals()
{

    stepinfo("basic_test_with_globals\n");
    basic_test(global_media, 1, UX_NULL);
}

UINT ux_test_device_class_storage_entry(UX_SLAVE_CLASS_COMMAND *command)
{

UINT            status;
ULONG           old_time_slice;
UX_SLAVE_CLASS  *storage_class;
TX_THREAD       *storage_thread;


    status = _ux_device_class_storage_entry(command);

    if (command -> ux_slave_class_command_request == UX_SLAVE_CLASS_COMMAND_INITIALIZE)
    {

        storage_class = command->ux_slave_class_command_class_ptr;

        /* Currently, the slave storage thread has no time slice. This is a problem
           in the following case:
            1) Device write to storage device fails
            2) Device stalls bulk OUT endpoint
            3) Device sends CSW, waits for transaction scheduler
            4) Host tries to receive CSW, waits for transaction scheduler
            5) Transaction scheduler completes transfer, resumes device
            6) Device storage thread loops forever (with no sleep) waiting for host
               to clear OUT endpoint, but host thread can't run because slave
               storage thread runs forever.

            This is valid behavior on the device's part since, obviously,
            the host isn't running on the same processor.

            The fix is to give the device storage thread a very large time slice;
            if it ever reaches the end of the time slice, it means it's waiting
            for the host to clear the stall. */
        storage_thread = &storage_class->ux_slave_class_thread;
        status = tx_thread_time_slice_change(storage_thread, 100, &old_time_slice);
        if (status != TX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
            test_control_return(1);
        }
    }

    return(status);
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_storage_tests_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;


    /* Inform user.  */
    printf("Running Storage Basic Functionality Test............................ ");
#if !(UX_TEST_MULTI_IFC_ON) || !(UX_MAX_SLAVE_LUN > 1)
    printf("SKIP!");
    test_control_return(0);
    return;
#endif

    stepinfo("\n");

    /* Initialize testing-related resources. */
    initialize_test_resources();

    /* Initialize FileX.  */
    fx_system_initialize();

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Reset ram disk memory.  */
    ux_utility_memory_set(global_ram_disk_memory, 0, UX_RAM_DISK_SIZE);

    /* The code below is required for installing the device portion of USBX.
       In this demo, DFU is possible and we have a call back for state change. */
    status = ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                        device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                        string_framework, STRING_FRAMEWORK_LENGTH,
                                        language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH, UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Store the number of LUN in this device storage instance.  */
    global_storage_parameter.ux_slave_class_storage_parameter_number_lun = 1;

    /* Set the instance activate and deactivate callbacks. */
    global_storage_parameter.ux_slave_class_storage_instance_activate = ux_slave_class_storage_instance_activate;
    global_storage_parameter.ux_slave_class_storage_instance_deactivate = ux_slave_class_storage_instance_deactivate;

    /* Initialize the storage class parameters for reading/writing to the Flash Disk.  */
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_last_lba        =  UX_RAM_DISK_LAST_LBA;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_block_length    =  global_sector_size;
    /* Note: The multiple_and_different_lun_types_test tests the other two types (OPTICAL DISK and IOMEGA CLICK). */
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_type            =  UX_HOST_CLASS_STORAGE_MEDIA_FAT_DISK;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_removable_flag  =  0x80;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read            =  default_device_media_read;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write           =  default_device_media_write;
#ifdef BUGFIX /* USBX_200 */
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_flush           =  default_device_media_flush;
#endif
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status          =  default_device_media_status;

    /* Initilize the device storage class. The class is connected with interface 0 on configuration 1. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_test_device_class_storage_entry,
                                                1, 0, (VOID *)&global_storage_parameter);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    global_dcd = &_ux_system_slave->ux_system_slave_dcd;
    global_slave_class_container = &_ux_system_slave->ux_system_slave_class_array[0];
    global_slave_device = &_ux_system_slave->ux_system_slave_device;
    global_slave_storage_thread = &global_slave_class_container->ux_slave_class_thread;

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(ux_test_system_host_change_function);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    global_host_device = &_ux_system_host->ux_system_host_device_array[0];
    global_enum_thread = &_ux_system_host->ux_system_host_enum_thread;

    /* Register the error callback. */
    ux_utility_error_callback_register(ux_test_error_callback);

    /* Register storage class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_storage_name, ux_host_class_storage_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    global_hcd = &_ux_system_host->ux_system_host_hcd_array[0];

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", ux_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

/* CO: Create and Open */
static UINT ux_test_storage_file_co(FX_MEDIA *media, FX_FILE *file, UCHAR fail_on_error)
{

UINT status;


    /* Create the file.  */
    status = fx_file_create(media, "FILE.CPY");
    if (fail_on_error == UX_TRUE && status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    memset(file,0,sizeof(FX_FILE));

    /* Open file for copying.  */
    status = fx_file_open(media,file,"FILE.CPY",FX_OPEN_FOR_WRITE);
    if (fail_on_error == UX_TRUE && status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Seek to the beginning to copy over an existing file.  */
    status = fx_file_seek(file,0);
    if (fail_on_error == UX_TRUE && status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    return status;
}

/* COW: Create, Open, and Write */
static UINT ux_test_storage_file_cow(FX_MEDIA *media, FX_FILE *file, UCHAR *data_pointer, ULONG data_length, UCHAR fail_on_error)
{

UINT status;


    status = ux_test_storage_file_co(media, file, fail_on_error);
    if (fail_on_error == UX_TRUE && status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    status = fx_file_write(file, data_pointer, data_length);
    if (fail_on_error == UX_TRUE && status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Ensure it's actually written to. */
    UX_TEST_CHECK_SUCCESS(fx_media_flush(media));

    /* Most of the time, we want to do a read right after. */
    status = fx_file_seek(file, 0);
    if (fail_on_error == UX_TRUE && status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    return status;
}

static void ux_test_storage_file_cd(FX_MEDIA *media, FX_FILE *file)
{

UINT status;


    /* Close the file. */
    status = fx_file_close(file);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Delete the target file.  */
    status = fx_file_delete(media, "FILE.CPY");
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

static void ux_test_storage_file_cowcd(FX_MEDIA *media, FX_FILE *file, UCHAR *data_pointer, ULONG data_length)
{

    ux_test_storage_file_cow(media, file, data_pointer, data_length, UX_TRUE);
    ux_test_storage_file_cd(media, file);
}

/* Device reset test resources. */

static void device_reset_test(UINT cb_length, UCHAR *message)
{

UCHAR                   transfer_request_data_unit_test_cbw[] = { 0x55, 0x53, 0x42, 0x43, 0x43, 0x42, 0x53, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, cb_length, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UX_TEST_ACTION          actions[] = {
    create_cbw_match_action(tx_thread_identify()),
    create_csw_error_action(tx_thread_identify(), UX_HOST_CLASS_STORAGE_CSW_PHASE_ERROR),
    { 0 }
};


    /* Test ux_host_class_storage_device_reset. */
    if (message)
    {
        stepinfo("%s", message);
    }

    /* Set our amazing action. */
    ux_test_set_main_action_list_from_array(actions);

    lock_out_storage_thread();
    UX_TEST_CHECK_NOT_SUCCESS(ux_test_host_class_storage_media_write(global_storage, 10, 1, global_buffer));
    lock_in_storage_thread();
}

static void let_storage_thread_run()
{

    /* Get the semaphore so we can keep track of the storage thread. */
    lock_out_storage_thread();

    /* Wait for storage thread to wait on the semaphore. */
    while (global_storage->ux_host_class_storage_semaphore.tx_semaphore_suspended_count == 0)
        tx_thread_sleep(10);

    /* Now let storage thread run. */
    lock_in_storage_thread();
    tx_thread_sleep(100);

    /* TODO: match a 2 second sleep via test action engine (trademark)? */

    /* Wait for storage thread to complete a single cycle. We know it has when it does the 2 second sleep (hopefullly
       there aren't any other sleeps it does though...). */
    UINT global_storage_thread_state;
    do
    {

        tx_thread_sleep(10);
        UX_TEST_CHECK_SUCCESS(tx_thread_info_get(global_storage_thread, UX_NULL, &global_storage_thread_state, UX_NULL, UX_NULL, UX_NULL, UX_NULL, UX_NULL, UX_NULL));
    } while (global_storage_thread_state != TX_SLEEP);
}

/* Test unit ready test resourcs. */

UCHAR turt_unit_attention_first_semaphore_get_fails_check_action_function()
{

    /* Have we closed the media yet? */
    return global_media->fx_media_id == 0 ? UX_TRUE : UX_FALSE;
}

static void test_unit_ready_test()
{

UINT                        i;
UINT                        sense_code_values[] = { UX_HOST_CLASS_STORAGE_SENSE_KEY_NOT_READY, UX_HOST_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION };
UX_HOST_CLASS_STORAGE_MEDIA *storage_media;
UCHAR                       *stringies[] = { "UX_HOST_CLASS_STORAGE_SENSE_KEY_NOT_READY", "UX_HOST_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION" };
UX_HOST_CLASS_STORAGE       *local_init_storage = global_storage;
UX_TEST_ACTION              *action_item;

    UX_TEST_ACTION not_ready_actions[] = {
        create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED),
        create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_NOT_READY),
        { 0 }
    };

    UX_TEST_ACTION unit_attention_actions[] = {
        create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED),
        create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION),
        { 0 }
    };

    UX_TEST_ACTION not_ready_first_semaphore_get_disconnect_actions[] = {
        create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED),
        create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_NOT_READY),
        /* The first semaphore_get() will be from the fx_media_close to flush. */
        create_semaphore_get_match_action(global_storage_thread, &global_storage->ux_host_class_storage_semaphore, UX_WAIT_FOREVER),
        /* The second semaphore_get() will be from the fx_media_close to unittialize driver. */
        create_semaphore_get_match_action(global_storage_thread, &global_storage->ux_host_class_storage_semaphore, UX_WAIT_FOREVER),
        create_semaphore_get_disconnect_action(global_storage_thread, &global_storage->ux_host_class_storage_semaphore, UX_WAIT_FOREVER),
        { 0 }
    };

    UX_TEST_ACTION first_class_instance_semaphore_get_disconnect_actions[] = {
        create_semaphore_get_disconnect_action(global_storage_thread, &global_storage->ux_host_class_storage_semaphore, UX_WAIT_FOREVER),
        { 0 }
    };

    UX_TEST_ACTION unit_attention_unit_ready_test_disconnect_actions[] = {
        create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED),
        create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION),
        create_disconnect_on_transfer_data_match_action(global_storage_thread, global_cbw_data_unit_ready_test_sbc, sizeof(global_cbw_data_unit_ready_test_sbc)),
        { 0 }
    };

    UX_TEST_ACTION unit_attention_media_characteristics_get_disconnect_actions[] = {
        create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED),
        create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION),
        create_disconnect_on_media_characteristics_get_action(global_storage_thread),
        { 0 }
    };

    UX_TEST_ACTION unit_attention_unit_ready_test_request_sense_disconnect_actions[] = {
        create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED),
        create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION),
        create_disconnect_on_transfer_data_match_action(global_storage_thread, global_cbw_data_unit_ready_test_sbc, sizeof(global_cbw_data_unit_ready_test_sbc)),
        { 0 }
    };

    UX_TEST_ACTION unit_attention_format_capacity_get_fails_via_memory_alloc_failure_actions[] = {
        create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED),
        create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION),
        create_test_unit_ready_match_action(global_storage_thread),
        /* We do two format_capacity_get()s. Make the second one fail. */
        create_format_capacity_get_match_action(global_storage_thread),
        /* The only way format_capacity_get fails is via memory allocation failure. */
        create_allocate_all_memory_on_transfer_data_match_action(global_storage_thread, UX_CACHE_SAFE_MEMORY, global_cbw_data_media_capacity_get_sbc, sizeof(global_cbw_data_media_capacity_get_sbc)),
        create_memory_allocation_fail_error_match_action(global_storage_thread),
        { 0 }
    };

    UX_TEST_ACTION unit_attention_first_semaphore_get_disconnect_actions[] = {
        create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED),
        create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION),
        /* The first semaphore_get() will be from the fx_media_close to flush. */
        create_semaphore_get_match_action(global_storage_thread, &global_storage->ux_host_class_storage_semaphore, UX_WAIT_FOREVER),
        /* The second semaphore_get() will be from the fx_media_close to unittialize driver. */
        create_semaphore_get_match_action(global_storage_thread, &global_storage->ux_host_class_storage_semaphore, UX_WAIT_FOREVER),
        create_semaphore_get_disconnect_action(global_storage_thread, &global_storage->ux_host_class_storage_semaphore, UX_WAIT_FOREVER),
        { 0 }
    };

    UX_TEST_ACTION *all_actions[] = {
        unit_attention_format_capacity_get_fails_via_memory_alloc_failure_actions,
        unit_attention_first_semaphore_get_disconnect_actions,
        not_ready_first_semaphore_get_disconnect_actions,
        unit_attention_media_characteristics_get_disconnect_actions,
        first_class_instance_semaphore_get_disconnect_actions,
        not_ready_actions,
        unit_attention_actions,
        unit_attention_unit_ready_test_disconnect_actions,
        unit_attention_unit_ready_test_request_sense_disconnect_actions,
    };


    stepinfo("unit_ready_test_test\n");

    stepinfo("    single lun\n");

    for (i = 0; i < ARRAY_COUNT(all_actions); i++)
    {

        stepinfo("        doing %d\n", i);

        if (global_storage != local_init_storage)
        {
            action_item = all_actions[i];
            while(action_item->function != 0 || action_item->usbx_function != 0)
            {
                if (action_item->semaphore_ptr == &local_init_storage->ux_host_class_storage_semaphore)
                {
                    action_item->semaphore_ptr = &global_storage->ux_host_class_storage_semaphore;
                }
                action_item ++;
            }
        }

        ux_test_set_main_action_list_from_array(all_actions[i]);

        let_storage_thread_run();

        /* Ensure expected result and clean up. */

        if (all_actions[i] == not_ready_actions)
        {

            /* Make sure storage media is unmounted. */
            if (!!_storage_media_is_mounted())
            {

                printf("Error on line %d\n", __LINE__);
                test_control_return(1);
            }
        }
        else if (all_actions[i] == unit_attention_actions)
        {

            /* Make sure storage media is mounted. */
            if (!_storage_media_is_mounted())
            {

                printf("Error on line %d\n", __LINE__);
                test_control_return(1);
            }

            /* In thierry, we should've remounted the media, so run a basic test. */
            basic_test(global_media, 1, "            running basic test\n");
        }
        else if (all_actions[i] == unit_attention_first_semaphore_get_disconnect_actions ||
                 all_actions[i] == first_class_instance_semaphore_get_disconnect_actions ||
                 all_actions[i] == unit_attention_unit_ready_test_disconnect_actions ||
                 all_actions[i] == unit_attention_unit_ready_test_request_sense_disconnect_actions ||
                 all_actions[i] == unit_attention_media_characteristics_get_disconnect_actions ||
                 all_actions[i] == not_ready_first_semaphore_get_disconnect_actions)
        {

            /* Note: the disconnection method we use ensures disconnection is complete. */

            if (global_media->fx_media_id != 0)
            {

                printf("Error on line %d\n", __LINE__);
                test_control_return(1);
            }

            /* Setup for next test. */
            connect_host_and_slave();
        }
        else if (all_actions[i] == unit_attention_format_capacity_get_fails_via_memory_alloc_failure_actions)
        {

            UX_TEST_ASSERT(ux_test_check_actions_empty());

            if (global_media->fx_media_id != 0)
            {

                printf("Error on line %d\n", __LINE__);
                test_control_return(1);
            }

            /* This test consisted of a memory allocation failure. Free the memory. */
            ux_test_utility_sim_mem_free_all_flagged(UX_CACHE_SAFE_MEMORY);

            /* The media is in an unstable state. Reset. */
            disconnect_host_and_slave();
            connect_host_and_slave();
        }
        else
        {

            /* We should handle them individually. */
            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        UX_TEST_ASSERT(ux_test_check_actions_empty());
    }

    /* For this test, there two LUNs and the second one reports the errors. */
    stepinfo("    Host receives NOT_READY and UNIT_ATTENTION sense keys from second LUN\n");

    /* Add another LUN. */
    global_persistent_slave_storage->ux_slave_class_storage_number_lun = 2;
    global_persistent_slave_storage->ux_slave_class_storage_lun[1] = global_persistent_slave_storage->ux_slave_class_storage_lun[0];

    for (i = 0;; i++)
    {

        stepinfo("        doing %s\n", stringies[i]);

        disconnect_host_and_slave();
        connect_host_and_slave();

        UX_TEST_ACTION multi_lun_not_ready_actions[] = {
            create_csw_match_action(global_storage_thread),
            create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED),
            create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_NOT_READY),
            { 0 }
        };

        UX_TEST_ACTION multi_lun_unit_attention_actions[] = {
            create_csw_match_action(global_storage_thread),
            create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED),
            create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION),
            { 0 }
        };

        UX_TEST_ACTION *multi_lun_actions[] = {
            multi_lun_not_ready_actions,
            multi_lun_unit_attention_actions,
        };

        ux_test_set_main_action_list_from_array(multi_lun_actions[i]);

        /* Let storage thread run at least once. */
        ux_utility_delay_ms(UX_TEST_SLEEP_STORAGE_THREAD_RUN_ONCE);

        /* First LUN should still be mounted. */
        storage_media = global_host_storage_class->ux_host_class_media;
        if (!_storage_media_is_mounted())
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        storage_media++;
        if (multi_lun_actions[i] == multi_lun_not_ready_actions)
        {

            /* Make sure storage media is unmounted. */
            if (!_storage_media_is_mounted())
            {

                printf("Error on line %d\n", __LINE__);
                test_control_return(1);
            }
        }
        else if (multi_lun_actions[i] == multi_lun_unit_attention_actions)
        {

            /* Make sure storage media is mounted. */
            if (!_storage_media_is_mounted())
            {

                printf("Error on line %d\n", __LINE__);
                test_control_return(1);
            }

            /* In thierry, we should've remounted the media, so run a basic test */
            basic_test(global_media, 1, "    running basic test\n");
        }

        if (i == ARRAY_COUNT(multi_lun_actions) - 1)
        {
            break;
        }
    }
}

/* Abort media test resources. */

static void abort_media_test()
{

UINT    status;
FX_FILE file;


    /* Test aborting the media. */
    stepinfo("Abort media test\n");

    /* Create file and write something. */
    ux_test_storage_file_cow(global_media, &file, global_buffer, 1, UX_TRUE);

    /* Abort media. Media must be reopened if it is to be used again. */
    status = fx_media_abort(global_media);
    if (status != FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Reopen media. */
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    status = fx_media_open(global_media, UX_HOST_CLASS_STORAGE_MEDIA_NAME, _ux_host_class_storage_driver_entry,
                           global_storage, global_storage_media->ux_host_class_storage_media_memory,
                           UX_HOST_CLASS_STORAGE_MEMORY_BUFFER_SIZE);
#else
    status = fx_media_open(global_media, UX_HOST_CLASS_STORAGE_MEDIA_NAME, _ux_host_class_storage_driver_entry,
                           global_storage_media, _ux_host_class_storage_media_fx_media_memory(global_storage_media),
                           UX_HOST_CLASS_STORAGE_MEMORY_BUFFER_SIZE);
#endif
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Delete the target file.  */
    status = fx_file_delete(global_media, "FILE.CPY");
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Do basic test to ensure everything still works. */
    ux_test_storage_file_cowcd(global_media, &file, global_buffer, 1);
}

/* boot_sector_write_fail_test resources */

static void boot_sector_write_fail_test()
{

UINT    status;


    /* Test formatting the media. */
    stepinfo("boot_sector_write_fail_test\n");

    /* This test forces the boot write in ux_host_class_storage_driver_entry to fail. Writing to the boot sector occurs
       during fx_media_volume_set.
       Specific test case: ux_host_class_storage_driver_entry.c case FX_DRIVER_BOOT_WRITE */
    stepinfo("    fx_media_volume_set fails due to write error (also boot write fails test)\n");

    UX_TEST_ACTION actions = { 0 };
    add_multiple_read_retries_fails_actions(&actions, tx_thread_identify());
    ux_test_set_main_action_list_from_list(&actions);

    status =  fx_media_volume_set(global_media, "C");
    if (status == UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    stepinfo("    fx_media_volume_set succeeds\n");

    /* Do it again without the broken media write function. */
    status =  fx_media_volume_set(global_media, "C");
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Do basic test to ensure everything still works. */
    basic_test(global_media, 1, "    basic testerino\n");
}

/* Flush media test resources. */

static void flush_media_test()
{

UINT    status;
FX_FILE file;


    /* Test flushing the media. */
    stepinfo("Flush media test\n");

    /* Create, open, and write file. */
    ux_test_storage_file_cow(global_media, &file, global_buffer, 1, UX_TRUE);

    status = fx_media_flush(global_media);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Close and delete file */
    ux_test_storage_file_cd(global_media, &file);
}

/* Close media test resources. */

static void close_media_test()
{

UINT status;


    /* Test closing the media. */
    stepinfo("Close media test\n");

    status = fx_media_close(global_media);
    if (status != FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

/* Entry command test resources. */

static void entry_command_test()
{

UINT                    status;
UX_HOST_CLASS_COMMAND   command;
UX_TEST_ACTION          actions[] = {
    create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED),
    {0}
};


    /* Test _ux_host_class_storage_entry unknown command */
    stepinfo("_ux_host_class_storage_entry unknown command\n");

    ux_test_set_main_action_list_from_array(actions);

    command.ux_host_class_command_request = 0xdeadbeef;
    status = ux_host_class_storage_entry(&command);
    if (status == UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

static UINT get_sector_start()
{
UINT sector_start = global_media->fx_media_driver_logical_sector
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
         + global_storage_media->ux_host_class_storage_media_partition_start
#endif
         ;


    return sector_start;
}

static void ux_test_host_class_storage_media_read_write_basic_test(UINT iterations, char *message, UINT lun)
{

UCHAR   buffer_pattern;
UINT    sector_count;
UINT    size;
UINT    i;
UINT    j;


    /* Basic ux_test_host_class_storage_media_write & ux_test_host_class_storage_media_write tests. */
    if (message != UX_NULL)
    {
        stepinfo("%s", message);
    }

    if (iterations > 2)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    buffer_pattern = 'G';

#if 0
    for (i = 0; i < iterations; i++)
#else
    for (i = 0; i < 1; i++)
#endif
    {

        if (i == 0)
            size = UX_DEMO_FILE_BUFFER_SIZE;
        else
            size = UX_DEMO_LARGE_FILE_BUFFER_SIZE;

        sector_count = ((size - 1) + global_storage_media->ux_host_class_storage_media_sector_size) / global_storage_media->ux_host_class_storage_media_sector_size;

        if (i == 0)
        {

            /* Set buffer we're going to use to write. */
            ux_utility_memory_set(global_buffer, buffer_pattern, size);
        }
        else
        {

            /* Set one half. */
            ux_utility_memory_set(global_buffer, buffer_pattern, size / 2);

            /* Set the other half. */
            ux_utility_memory_set(global_buffer + size / 2, buffer_pattern + 1, size / 2);
        }

        lock_out_storage_thread();
        global_storage->ux_host_class_storage_lun = lun;
        ux_test_host_class_storage_media_write(global_storage,
            global_media->fx_media_driver_logical_sector
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
             + global_storage_media->ux_host_class_storage_media_partition_start
#endif
             ,
            sector_count, global_buffer);
        lock_in_storage_thread();

        /* Clear buffer to make sure read() really works. */
        ux_utility_memory_set(global_buffer, 0, UX_DEMO_FILE_BUFFER_SIZE);

        lock_out_storage_thread();
        global_storage->ux_host_class_storage_lun = lun;
        ux_test_host_class_storage_media_read(global_storage,
            global_media->fx_media_driver_logical_sector
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
             + global_storage_media->ux_host_class_storage_media_partition_start
#endif
             ,
            sector_count, global_buffer);
        lock_in_storage_thread();

        /* Check result of read(). */
        if (i == 0)
        {

            for (j = 0; j < size; j++)
            {

                if (global_buffer[j] != buffer_pattern)
                {

                    printf("Error on line %d\n", __LINE__);
                    test_control_return(1);
                }
            }
        }
        else
        {

            for (j = 0; j < size / 2; j++)
            {

                if ((global_buffer[j] != buffer_pattern) || (global_buffer[size / 2 + j] != (buffer_pattern + 1)))
                {

                    printf("Error on line %d\n", __LINE__);
                    test_control_return(1);
                }
            }
        }
    }
}

/* fx_media_write test resources */

/* This is used to match our action with cbw transfer (CBW has a length field, we try to match it). */
#define WRITE_SIZE (2*UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE)

static void fx_media_write_test()
{

UINT    status;
FX_FILE file;


    stepinfo("Test fx_media_write API\n");

    /* Specific test case: in ux_host_class_storage_driver_entry, the media_write() fails. */
    stepinfo("    transfer fails\n");

    ux_test_storage_file_co(global_media, &file, UX_TRUE);

    UX_TEST_ACTION cbw_transfer_fail_actions[] = {
        create_cbw_disconnect_action(tx_thread_identify()),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(cbw_transfer_fail_actions);

    status = fx_file_write(&file, global_buffer, WRITE_SIZE);
    if (status == FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    UX_TEST_ASSERT(ux_test_check_actions_empty());
    connect_host_and_slave();

    stepinfo("    data phase fails due to timeout\n");

    DEVICE_MEDIA_READ_WRITE_TIMEOUT_DATA timeout_data;
    timeout_data.num_timeouts = 1;
    timeout_data.transfer_request = &global_storage->ux_host_class_storage_bulk_out_endpoint->ux_endpoint_transfer_request;

    UX_TEST_ACTION data_phase_timeout_actions[] = {
        create_device_media_write_block_action(&timeout_data),
        create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_TRANSFER_TIMEOUT),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(data_phase_timeout_actions);

    lock_out_storage_thread();
    UX_TEST_CHECK_NOT_SUCCESS(ux_test_host_class_storage_media_write(global_storage, 10, UX_TEST_MULTIPLE_TRANSFERS_SECTOR_COUNT, global_buffer));
    receive_device_csw();
    lock_in_storage_thread();
    UX_TEST_ASSERT(ux_test_check_actions_empty());
    basic_test(global_media, 1, "        basic test\n");

    /* Specific test case: while (media_retry-- != 0) fails in ux_test_host_class_storage_media_write */
    /* How: Having the device write fail, causing the device to stall the endpoint and send a CSW with error. */
    stepinfo("    multiple retries fails\n");

    ux_test_storage_file_co(global_media, &file, UX_TRUE);

    UX_TEST_ACTION actions = { 0 };
    add_multiple_write_retries_fails_actions(&actions);
    ux_test_set_main_action_list_from_list(&actions);

    status = fx_file_write(&file, global_buffer, WRITE_SIZE);
    if (status == FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    basic_test_pre_existing_file(1, "    running basic test\n", &file);

    UX_TEST_ASSERT(ux_test_check_actions_empty());
}

#undef WRITE_SIZE

/* fx_media_read test resources */

static UCHAR boot_read_fails_action_check_function()
{

UCHAR result;

#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    /* Is this from FileX? */
    result = (get_internal_host_storage_medias()->ux_host_class_storage_media_memory != UX_NULL);
#else
    result = UX_TRUE;
#endif
    return result ? UX_TRUE : UX_FALSE;
}

static UX_TEST_ACTION create_boot_read_from_filex_fails_action(TX_THREAD *thread_to_match)
{

UX_TEST_ACTION action;


    action = create_device_media_read_fail_action(thread_to_match);
    action.check_func = boot_read_fails_action_check_function;

    return action;
}

/* BUG_ID_19 - tldr; original sim is broken, so this size only works with fixed sim. */
#define READ_SIZE (2*UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE)

static void fx_media_read_test()
{

UINT                            status;
UX_TEST_ERROR_CALLBACK_ERROR    transfer_stall_error = { UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_TRANSFER_STALLED };
FX_FILE                         file;
ULONG                           actual_size;
UX_HOST_CLASS_STORAGE_MEDIA     *storage_media;
UINT                            i;


    stepinfo("Test fx_media_read API\n");

    stepinfo("    large read\n");

    /* Set first half of buffer to 'a', second half to 'b'. */
    ux_utility_memory_set(global_buffer_2x_slave_buffer_size, 'a', sizeof(global_buffer_2x_slave_buffer_size)/2);
    ux_utility_memory_set(global_buffer_2x_slave_buffer_size + sizeof(global_buffer_2x_slave_buffer_size)/2, 'b', sizeof(global_buffer_2x_slave_buffer_size)/2);

    /* We need to create a file and write to it, since there actually needs to be something to read. */
    ux_test_storage_file_cow(global_media, &file, global_buffer_2x_slave_buffer_size, READ_SIZE, UX_TRUE);

    status = fx_file_read(&file, global_buffer_2x_slave_buffer_size, READ_SIZE, &actual_size);
    if (status != FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Ensure data is correct. */
    for (i = 0; i < sizeof(global_buffer_2x_slave_buffer_size)/2; i++)
    {

        if (global_buffer_2x_slave_buffer_size[i] != 'a' ||
            global_buffer_2x_slave_buffer_size[sizeof(global_buffer_2x_slave_buffer_size)/2 + i] != 'b')
        {

            printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
            test_control_return(1);
        }
    }

    /* Specific test case: in ux_host_class_storage_driver_entry, the media_read() fails. */
    stepinfo("    transfer fails\n");

    status = fx_file_seek(&file, 0);
    if (status != FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    UX_TEST_ACTION cbw_disconnect_actions[] = {
        create_cbw_disconnect_action(tx_thread_identify()),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(cbw_disconnect_actions);

    UX_TEST_CHECK_NOT_SUCCESS(fx_file_read(&file, global_buffer_2x_slave_buffer_size, READ_SIZE, &actual_size));
    connect_host_and_slave();

#if 0 /* NOBUGFIX: BUG_ID_25 */
    stepinfo("    multiple BO data phase transfers, first one fails\n");

    /* Action for making read fail. */
    read_fail_actions[0].is_valid = UX_TRUE;
    read_fail_actions[0].func = ux_test_host_class_storage_media_read_test_media_read_action_func;

    /* Set media read action. */
    set_device_media_read_actions(read_fail_actions);

    /* We should get one stall error. */
    ux_test_error_callback_add_error_to_ignore(stall_error);

    status = ux_test_media_read(storage, media->fx_media_driver_logical_sector +
                                 storage_media->ux_host_class_storage_media_partition_start,
                                 8, media->fx_media_driver_buffer, UX_FALSE);
    if (status == UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
#endif

    /* Specific test case: while (media_retry-- != 0) fails */
    /* Specific test case: if (*(cbw + UX_HOST_CLASS_STORAGE_CBW_FLAGS) == UX_HOST_CLASS_STORAGE_DATA_IN) in
       ux_host_class_transport_bo */
    /* Note: this one is a pain in the ass, since the sense code check is after
       the data phase length check. This means the data phase has to "succeed",
       but the sense code contains an error. Is this even possible? I think so:
       if the device has less data to send than the host requests, but pads out
       the data, then according to the spec (thirteen cases), the device will
       stall the endpoint, and most likely, the request sense will have an error. */
    stepinfo("    multiple retries fails\n");

    ux_test_storage_file_cow(global_media, &file, global_buffer, 512, UX_TRUE);
    UX_TEST_ACTION actions = { 0 };
    add_multiple_read_retries_fails_actions(&actions, tx_thread_identify());
    ux_test_set_main_action_list_from_list(&actions);
    UX_TEST_CHECK_NOT_SUCCESS(fx_file_read(&file, global_buffer, 512, &actual_size));
    UX_TEST_ASSERT(ux_test_check_actions_empty());
    basic_test_pre_existing_file(1, "    running basic test\n", &file);

#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX) /* Boot read is available only when FX_MEDIA is integrated.  */
    /* This test occurs during enumeration. The boot sector read needs to fail in the USBX storage driver for FileX.
       This means we can't just fail during the first boot sector read; we need to fail only when it's a boot sector
       read from FileX. How do we determine if it's from FileX? Before USBX calls fx_media_open() (which does the boot
       sector read), it initializes a UX_HOST_CLASS_STORAGE_MEDIA instance. We check if that instance is initialized
       and if it is, we know FileX is doing the read... The problem is that upon deinitialization, USBX doesn't clear
       reset the instance's members, so we do it ourselves.

       Specific test case: ux_host_class_storage_driver_entry.c case FX_DRIVER_BOOT_READ
                           and
                           / Ask FileX to mount the partition.  /
                           ux_host_class_storage_media_open.c fx_media_open fails
    */
    stepinfo("    boot read fails\n");

    disconnect_host_and_slave();

    /* Point the media structure to the first media in the container.  */
    storage_media = (UX_HOST_CLASS_STORAGE_MEDIA *) _ux_system_host -> ux_system_host_class_array[0].ux_host_class_media;
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    storage_media -> ux_host_class_storage_media_memory = UX_NULL;
#endif

    for (i = 0; i < UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RETRY; i++)
    {
        ux_test_add_action_to_main_list(create_boot_read_from_filex_fails_action(tx_thread_identify()));
        ux_test_add_action_to_main_list(create_error_match_action_from_error(transfer_stall_error));
    }

    connect_host_and_slave();

    /* Since boot read should've failed, the storage instance shouldn't be valid. */
    if (global_media -> fx_media_id != 0)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
#endif
}

/* first_sector_non_boot_test_ram_disk_memory test resources */

static CHAR     first_sector_non_boot_test_ram_disk_memory[1*1024];

static VOID first_sector_non_boot_test_media_read_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS *params = (UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS *)_params;


    while (params->number_blocks--)
    {
        ux_utility_memory_copy(params->data_pointer, first_sector_non_boot_test_ram_disk_memory + global_sector_size * params->lba, global_sector_size);
        params->lba++;
    }
}

static UX_TEST_ACTION create_first_sector_non_boot_test_media_read_action()
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ;
    action.ignore_params = 1;
    action.no_return = 0;
    action.status = UX_SUCCESS;
    action.action_func = first_sector_non_boot_test_media_read_action_func;

    return action;
}

/* Tests the cases where the first sector is not the boot sector, but instead a partition table. */
static void first_sector_non_boot_test()
{

UINT                    i;
UX_SLAVE_CLASS_STORAGE  *device_storage;
UCHAR                   *extended_partition;
UCHAR                   *partition_table;
UCHAR                   partition_types[] = {
    /* Extended partitions. */
    UX_HOST_CLASS_STORAGE_PARTITION_EXTENDED,
    UX_HOST_CLASS_STORAGE_PARTITION_EXTENDED_LBA_MAPPED,

    /* Regular partitions. */
    UX_HOST_CLASS_STORAGE_PARTITION_FAT_12,
    UX_HOST_CLASS_STORAGE_PARTITION_FAT_16,
    UX_HOST_CLASS_STORAGE_PARTITION_FAT_16L,
    UX_HOST_CLASS_STORAGE_PARTITION_FAT_16_LBA_MAPPED,
    UX_HOST_CLASS_STORAGE_PARTITION_FAT_32_1,
    UX_HOST_CLASS_STORAGE_PARTITION_FAT_32_2,

    /* Unknown partition. */
    0xff
};
UX_TEST_ACTION          first_sector_non_boot_test_extended_media_actions[] = {
    create_first_sector_non_boot_test_media_read_action(),
    create_first_sector_non_boot_test_media_read_action(),
    { 0 }
};
UX_TEST_ACTION          first_sector_non_boot_test_media_read_actions[] = {
    create_first_sector_non_boot_test_media_read_action(),
    { 0 }
};


    stepinfo("first sector is not boot test\n");

    /* Get device storage instance. */
    device_storage = _ux_system_slave->ux_system_slave_class_array[0].ux_slave_class_instance;

    for (i = 0; i < ARRAY_COUNT(partition_types); i++)
    {

        stepinfo("    doing %d\n", i);

        disconnect_host_and_slave();

        /* Set disk. */

        partition_table = &first_sector_non_boot_test_ram_disk_memory[UX_HOST_CLASS_STORAGE_PARTITION_TABLE_START];

        /* Set partition signature. */
        ux_utility_short_put(first_sector_non_boot_test_ram_disk_memory + 510, UX_HOST_CLASS_STORAGE_PARTITION_SIGNATURE);

        /* Make sure there's no boot signature. */
        first_sector_non_boot_test_ram_disk_memory[0] = 0x00;
        first_sector_non_boot_test_ram_disk_memory[1] = 0x00;
        first_sector_non_boot_test_ram_disk_memory[2] = 0x00;

        /* Set the partition table entry type. */
        partition_table[UX_HOST_CLASS_STORAGE_PARTITION_TYPE] = partition_types[i];

        /* Is this an extended entry? An extended entry means it points to another partition table. */
        if (partition_types[i] == UX_HOST_CLASS_STORAGE_PARTITION_EXTENDED ||
            partition_types[i] == UX_HOST_CLASS_STORAGE_PARTITION_EXTENDED_LBA_MAPPED)
        {

            /* The extended partition is located one sector away. */
            partition_table[UX_HOST_CLASS_STORAGE_PARTITION_SECTORS_BEFORE] = 1;

            /* Set disk. */

            extended_partition = &first_sector_non_boot_test_ram_disk_memory[UX_HOST_CLASS_STORAGE_SECTOR_SIZE_FAT];
            partition_table = &extended_partition[UX_HOST_CLASS_STORAGE_PARTITION_TABLE_START];

            /* Set partition signature. */
            ux_utility_short_put(extended_partition + 510, UX_HOST_CLASS_STORAGE_PARTITION_SIGNATURE);

            /* Make sure there's no boot signature. */
            extended_partition[0] = 0x00;
            extended_partition[1] = 0x00;
            extended_partition[2] = 0x00;

            /* Set the type to something non-extended so we don't recurse. */
            partition_table[UX_HOST_CLASS_STORAGE_PARTITION_TYPE] = UX_HOST_CLASS_STORAGE_PARTITION_FAT_12;

            /* We want subsequent boot sector reads to read the beginning of the ram disk (where FileX formatted it).
               We need to overflow to get the SECTORS_BEFORE to equal zero. */
            ux_utility_long_put(&partition_table[UX_HOST_CLASS_STORAGE_PARTITION_SECTORS_BEFORE], ~0);

            /* Set action for media read. Since it's an extended entry, it will read twice. */
            ux_test_set_main_action_list_from_array(first_sector_non_boot_test_extended_media_actions);
        }
        else
        {

            /* We want subsequent boot sector reads to read the beginning of the ram disk (where FileX formatted it). */
            partition_table[UX_HOST_CLASS_STORAGE_PARTITION_SECTORS_BEFORE] = 0;

            /* Set action for media read. */
            ux_test_set_main_action_list_from_array(first_sector_non_boot_test_media_read_actions);
        }

        /* Start enumeration. */
        connect_host_and_slave();

        if (partition_types[i] != PARTITION_TYPE_UNKNOWN)
        {

            if (global_storage == UX_NULL)
            {

                printf("Error on line %d\n", __LINE__);
                test_control_return(1);
            }

            basic_test(global_media, 1, "    running basic test\n");
        }
        /* Ensure unknown partition wasn't mounted. */
        else if (global_media->fx_media_id != 0)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }
    }

    /* Since during the last test the media was never opened, we need to disconnect and reconnect. */
    disconnect_host_and_slave();
    connect_host_and_slave();
}

/* multiple_and_different_lun_types_test resources */

static UINT                         madltt_media_types[] = { UX_HOST_CLASS_STORAGE_MEDIA_OPTICAL_DISK, UX_HOST_CLASS_STORAGE_MEDIA_IOMEGA_CLICK, UX_HOST_CLASS_STORAGE_MEDIA_CDROM, MEDIA_TYPE_UNKNOWN };
static UX_TEST_ERROR_CALLBACK_ERROR dltt_media_not_supported_error = { UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_MEDIA_NOT_SUPPORTED };
static UCHAR                        dltt_unit_test_called_actions_data[UX_HOST_CLASS_STORAGE_MAX_MEDIA][sizeof(global_cbw_data_unit_ready_test_sbc)];
static UX_TEST_ACTION               dltt_unit_test_called_actions[UX_HOST_CLASS_STORAGE_MAX_MEDIA + 1];

/* This tests every possible permutation of LUN types within the allowable number of medias
   (UX_HOST_CLASS_STORAGE_MAX_MEDIA). We don't use the max LUNs because it's possible for us to run out of medias
   (USBX-93). */
static void dltt_test_every_lun_type_permutation(UINT luns_remaining)
{

UINT            lun_idx;
UINT            media_idx;
UINT            num_medias;
UINT            expected_medias;
UINT            expected_unit_ready_tests;
UX_TEST_ACTION  media_not_supported_match_action = create_error_match_action_from_error(dltt_media_not_supported_error);


    if (luns_remaining > 0)
    {

        for (lun_idx = 0; lun_idx < ARRAY_COUNT(madltt_media_types); lun_idx++)
        {

            global_persistent_slave_storage->ux_slave_class_storage_lun[luns_remaining - 1].ux_slave_class_storage_media_type = madltt_media_types[lun_idx];
            global_persistent_slave_storage->ux_slave_class_storage_lun[luns_remaining - 1].ux_slave_class_storage_media_removable_flag = (lun_idx % 2) ? 0x80 : 0x00;
            dltt_test_every_lun_type_permutation(luns_remaining - 1);
        }
    }
    else
    {

        /* All the LUNs are set. Let's do it! */

        stepinfo("        disconnecting...\n");

        disconnect_host_and_slave();

        global_persistent_slave_storage->ux_slave_class_storage_number_lun = UX_HOST_CLASS_STORAGE_MAX_MEDIA;

        /* Initialize actions. */
        ux_utility_memory_set(dltt_unit_test_called_actions, 0, sizeof(dltt_unit_test_called_actions));

        /* Get the number of medias we expect the LUNs should have, and add errors to ignore. */
        expected_medias = 0;
        expected_unit_ready_tests = 0;
        for (lun_idx = 0; lun_idx < ARRAY_COUNT(global_persistent_slave_storage->ux_slave_class_storage_lun); lun_idx++)
        {

            if (global_persistent_slave_storage->ux_slave_class_storage_lun[lun_idx].ux_slave_class_storage_media_type == UX_HOST_CLASS_STORAGE_MEDIA_OPTICAL_DISK ||
                global_persistent_slave_storage->ux_slave_class_storage_lun[lun_idx].ux_slave_class_storage_media_type == UX_HOST_CLASS_STORAGE_MEDIA_IOMEGA_CLICK ||
                global_persistent_slave_storage->ux_slave_class_storage_lun[lun_idx].ux_slave_class_storage_media_type == UX_HOST_CLASS_STORAGE_MEDIA_FAT_DISK)
            {
                expected_medias++;

#if 0 /* NOBUGFIX: USBX_100 */
                if (global_persistent_slave_storage->ux_slave_class_storage_lun[lun_idx].ux_slave_class_storage_media_removable_flag == 0x80)
                {

                    ux_utility_memory_copy(dltt_unit_test_called_actions_data[expected_unit_ready_tests], global_transfer_request_data_unit_test_sbc_cbw, sizeof(global_transfer_request_data_unit_test_sbc_cbw));
                    dltt_unit_test_called_actions_data[expected_unit_ready_tests][UX_HOST_CLASS_STORAGE_CBW_LUN] = lun_idx;
                    dltt_unit_test_called_actions[expected_unit_ready_tests].function = UX_HCD_TRANSFER_REQUEST;
                    dltt_unit_test_called_actions[expected_unit_ready_tests].req_actual_len = sizeof(global_transfer_request_data_unit_test_sbc_cbw);
                    dltt_unit_test_called_actions[expected_unit_ready_tests].no_return = 1;
                    dltt_unit_test_called_actions[expected_unit_ready_tests].req_data = dltt_unit_test_called_actions_data[expected_unit_ready_tests];
                    expected_unit_ready_tests++;
                }
#endif
            }
            else if (global_persistent_slave_storage->ux_slave_class_storage_lun[lun_idx].ux_slave_class_storage_media_type == MEDIA_TYPE_UNKNOWN)
            {

                ux_test_add_action_to_main_list(media_not_supported_match_action);
            }
        }

        for (lun_idx = 0; lun_idx < UX_HOST_CLASS_STORAGE_MAX_MEDIA; lun_idx++)
            stepinfo( "            media type: %lu\n", global_persistent_slave_storage->ux_slave_class_storage_lun[lun_idx].ux_slave_class_storage_media_type);

        stepinfo("        connecting...\n");
        connect_host_and_slave();

        stepinfo("            waiting 2 seconds for storage thread to run...\n");

#if 0 /* NOBUGFIX: USBX_100 */
        /* Add actions. */
        if (dltt_unit_test_called_actions[0].function != 0)
            ux_test_set_actions(dltt_unit_test_called_actions);
#endif

        /* Let storage thread run least once. */
        ux_utility_delay_ms(UX_TEST_SLEEP_STORAGE_THREAD_RUN_ONCE);

#if 0 /* NOBUGFIX USBX_100 */
        if (ux_test_actions_empty_check() != UX_TRUE)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }
#endif

        /* Go through each LUN and run a test if applicable. */
        num_medias = 0;
        media_idx = 0;
        for (lun_idx = 0; lun_idx < UX_HOST_CLASS_STORAGE_MAX_MEDIA; lun_idx++)
        {

            if (global_storage->ux_host_class_storage_lun_types[lun_idx] == UX_HOST_CLASS_STORAGE_MEDIA_OPTICAL_DISK ||
                global_storage->ux_host_class_storage_lun_types[lun_idx] == UX_HOST_CLASS_STORAGE_MEDIA_IOMEGA_CLICK ||
                global_storage->ux_host_class_storage_lun_types[lun_idx] == UX_HOST_CLASS_STORAGE_MEDIA_FAT_DISK)
            {

                /* Only optical disks, fat disks, and iomega clicks have FX_MEDIAs instances, which is why we only
                   increment media_idx if the type is one of those two. */
                basic_test(
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
                    &global_storage_medias[media_idx++].ux_host_class_storage_media,
#else
                    _ux_host_class_storage_media_fx_media(&global_storage_medias[media_idx++]),
#endif
                    1, "            running basic test...\n");

                /* We need to remove the file entirely for the next test. Usually disconnect_host_and_slave() does this
                   but we're not calling that in between basic_test() calls, now are we?...!...? */
                format_ram_disk();

                num_medias++;
            }
            else if (global_storage->ux_host_class_storage_lun_types[lun_idx] == UX_HOST_CLASS_STORAGE_MEDIA_CDROM)
            {

                /* For CDROMs, we don't mount the partition, so we can't use FileX. Also, we have to manually set which
                   LUN we want USBX to write to. */
                ux_test_host_class_storage_media_read_write_basic_test(1, "            running basic test...\n", lun_idx);
            }
            else// if(MEDIA_TYPE_UNKNOWN)
            {
                /* Do nothing. */
            }
        }

        if (num_medias != expected_medias)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        if (ux_test_check_actions_empty() == UX_FALSE)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }
    }
}

static void multiple_and_different_lun_types_test()
{

UINT            i;
UX_TEST_ACTION  unknown_media_error_actions[] = {
    create_error_match_action_from_error(dltt_media_not_supported_error),
    { 0 }
};


    /* Specific test cases: (storage -> ux_host_class_storage_removable_media == UX_HOST_CLASS_STORAGE_MEDIA_REMOVABLE)
       passes and fails in ux_host_class_storage_thread_entry.c */

    stepinfo("multiple_and_different_lun_types_test\n");

    stepinfo("    max lun test\n");

    for (i = 1; i < UX_HOST_CLASS_STORAGE_MAX_MEDIA; i++)
    {
        global_persistent_slave_storage->ux_slave_class_storage_lun[i] = global_persistent_slave_storage->ux_slave_class_storage_lun[0];
    }

    dltt_test_every_lun_type_permutation(UX_HOST_CLASS_STORAGE_MAX_MEDIA);

    /* Note: if there's a test after this, we should reset_to_bo(). */
    reset_to_bo();

    /* When we disconnect the slave, it sets the global device storage pointer to null. Save it! */
    global_persistent_slave_storage = global_slave_storage;
}

/* Transport failures cause device reset test resources. */

static void transport_failures_cause_device_reset_test()
{

    UX_TEST_ACTION actions[] = {
        /* This will cause the CSW to contain an error. */
        create_device_media_write_fail_action(),
        /* This is how we make the REQUEST SENSE fail. Every other option is completely ridiculous (i.e. transmission
           error, which never ever happens, and if they do happen, then the controller is broken). */
        create_disconnect_on_transfer_data_match_action(tx_thread_identify(), global_cbw_data_request_sense, sizeof(global_cbw_data_request_sense)),
        /* Since we disconnect, the request is never sent to the HCD, so we can't check it. */
        //create_mass_storage_reset_match_action(tx_thread_identify()),
        { 0 }
    };


    stepinfo("transport_failures_cause_device_reset_test\n");

    ux_test_set_main_action_list_from_array(actions);

    lock_out_storage_thread();
    UX_TEST_CHECK_NOT_SUCCESS(ux_test_host_class_storage_media_write(global_storage, 10, 1, global_buffer));
    lock_in_storage_thread();

    connect_host_and_slave();
}

/* Resources for failure of starting the UFI device test. */

static void ufi_start_fails_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UX_TRANSFER                         *transfer_request = params->parameter;


    transfer_request->ux_transfer_request_completion_code = UX_ERROR;
}

/* Specific test case: _ux_host_class_storage_start_stop fails in _ux_host_class_storage_media_mount */
static void ufi_start_fails()
{

UCHAR             transfer_data_ufi_start_cb[] = { 0x55, 0x53, 0x42, 0x43, 0x43, 0x42, 0x53, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UX_TEST_ACTION    actions[2] = { 0 };


    stepinfo("ufi_start_fails test\n");

    /* Action for making transfer of start command fail. */
    actions[0].usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    actions[0].function = UX_HCD_TRANSFER_REQUEST;
    actions[0].action_func = ufi_start_fails_action_func;
    actions[0].req_data = transfer_data_ufi_start_cb;
    actions[0].req_actual_len = sizeof(transfer_data_ufi_start_cb);
    actions[0].no_return = 0;
    actions[0].status = UX_ERROR;

    disconnect_host_and_slave();

    /* Switch to CBI (only CBI requires start command). */

    device_framework_full_speed[bInterfaceProtocol_POS] = UX_HOST_CLASS_STORAGE_PROTOCOL_CBI;
    device_framework_high_speed[bInterfaceProtocol_POS] = UX_HOST_CLASS_STORAGE_PROTOCOL_CBI;
    device_framework_full_speed[bInterfaceSubClass_POS] = UX_HOST_CLASS_STORAGE_SUBCLASS_UFI;
    device_framework_high_speed[bInterfaceSubClass_POS] = UX_HOST_CLASS_STORAGE_SUBCLASS_UFI;

    global_hcd->ux_hcd_entry_function = _ux_hcd_sim_host_entry_bo_to_cbi;
    global_dcd->ux_slave_dcd_function = _ux_dcd_sim_slave_function_bo_to_cbi;

    /* Set actions */
    ux_test_set_main_action_list_from_array(actions);

    connect_host_and_slave();

    if (global_media->fx_media_id == FX_MEDIA_ID)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
}

/* First sector read fails test resources. */

/* Specific test case: case UX_HOST_CLASS_STORAGE_SENSE_ERROR: and default: in ux_host_class_storage_media_mount */
static void media_mount_first_sector_read_fails_test()
{

UINT errors[] = { UX_ERROR, UX_HOST_CLASS_STORAGE_SENSE_ERROR };


    stepinfo("media_mount_first_sector_read_fail test\n");

    /* How: Read keeps failing. Note: this assumes the very first read the one we're looking for. */
    stepinfo("    case UX_HOST_CLASS_STORAGE_SENSE_ERROR: \n");

    disconnect_host_and_slave();
    UX_TEST_ACTION actions = { 0 };
    add_multiple_read_retries_fails_actions(&actions, tx_thread_identify());
    ux_test_set_main_action_list_from_list(&actions);
    connect_host_and_slave();

    /* How: we disconnect during the read. */
    stepinfo("    default :\n");

    disconnect_host_and_slave();
    ux_test_add_action_to_main_list(create_disconnect_on_transfer_data_match_action(global_enum_thread, global_cbw_data_first_sector_read, sizeof(global_cbw_data_first_sector_read)));
    UX_TEST_ASSERT(connect_host_and_slave() != UX_SUCCESS);
}

/* First sector not partition test resources. */

static UINT first_sector_different_signatures_test_num;
static ULONG first_sector_different_signatures_sig_values[5][6] =
{
       /* 510 */                                  /* 0 */   /* 2 */ /* 0x16 */  /* 0x24 */
    /* _ux_utility_short_get(sector_memory + 510) == UX_HOST_CLASS_STORAGE_PARTITION_SIGNATURE fails */
    { ~UX_HOST_CLASS_STORAGE_PARTITION_SIGNATURE, 0,        0,      0,          0,          UX_ERROR  },
    /* (*sector_memory == 0xe9) fails */
    {  UX_HOST_CLASS_STORAGE_PARTITION_SIGNATURE, 0xeb,     0x90,   0xff,       0,          UX_SUCCESS },
    /* *(sector_memory + 2) == 0x90 fails */
    {  UX_HOST_CLASS_STORAGE_PARTITION_SIGNATURE, 0xeb,     0,      0,          0,          UX_ERROR },
    /* _ux_utility_short_get(sector_memory + 0x16) != 0x0 fails and _ux_utility_long_get(sector_memory + 0x24) != 0x0 succeeds */
    {  UX_HOST_CLASS_STORAGE_PARTITION_SIGNATURE, 0xe9,     0,      0x00,       0xff,       UX_SUCCESS },
    /* _ux_utility_long_get(sector_memory + 0x24) != 0x0 fails */
    {  UX_HOST_CLASS_STORAGE_PARTITION_SIGNATURE, 0xe9,     0,      0x00,       0x00,       UX_ERROR },
};

static VOID first_sector_different_signatures_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS *params = (UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS *)_params;


    params->data_pointer[510] =                         (UCHAR) first_sector_different_signatures_sig_values[first_sector_different_signatures_test_num][0];
    params->data_pointer[0] =                           (UCHAR) first_sector_different_signatures_sig_values[first_sector_different_signatures_test_num][1];
    params->data_pointer[2] =                           (UCHAR) first_sector_different_signatures_sig_values[first_sector_different_signatures_test_num][2];
    ux_utility_short_put(&params->data_pointer[0x16],   (USHORT)first_sector_different_signatures_sig_values[first_sector_different_signatures_test_num][3]);
    ux_utility_long_put(&params->data_pointer[0x24],            first_sector_different_signatures_sig_values[first_sector_different_signatures_test_num][4]);
}

static UX_TEST_ACTION create_first_sector_different_signature_action()
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ;
    action.action_func = first_sector_different_signatures_action_func;
    action.do_after = 1;
    action.no_return = 1;
    action.ignore_params = 1;

    return action;
}

/* Specific test case: if (_ux_utility_short_get(sector_memory + 510) == UX_HOST_CLASS_STORAGE_PARTITION_SIGNATURE) fails in ux_host_class_storage_media_mount */
static void first_sector_different_signatures_test()
{

UX_TEST_ACTION actions[] = {
    create_first_sector_different_signature_action(),
    { 0 }
};


    stepinfo("first_sector_different_signatures\n");

    for (; first_sector_different_signatures_test_num < ARRAY_COUNT(first_sector_different_signatures_sig_values); first_sector_different_signatures_test_num++)
    {

        disconnect_host_and_slave();
        ux_test_set_main_action_list_from_array(actions);
        connect_host_and_slave();

        if ((first_sector_different_signatures_sig_values[first_sector_different_signatures_test_num][5] == UX_SUCCESS &&   global_media->fx_media_id != FX_MEDIA_ID) ||
            (first_sector_different_signatures_sig_values[first_sector_different_signatures_test_num][5] == UX_ERROR &&     global_media->fx_media_id == FX_MEDIA_ID))
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        if (first_sector_different_signatures_sig_values[first_sector_different_signatures_test_num][5] == UX_SUCCESS)
            basic_test(global_media, 1, "    basic test\n");
    }
}

/* General disconnection test resources. */

static VOID disconnection_tests_transfer_action_func(UX_TEST_ACTION *action, VOID *parameter)
{

    disconnect_host_and_slave();
}

static VOID disconnection_tests_transfer_with_relinquish_action_func(UX_TEST_ACTION *action, VOID *parameter)
{

    ux_test_hcd_sim_host_disconnect_no_wait();

    /* Simulate time slice running out. */
    tx_thread_relinquish();
}

/* Storage class related device disconnect test resources. */

static TX_SEMAPHORE transfer_initiated_semaphore;

static VOID device_disconnect_test_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UINT                                status;


    /* Tell test thread to start disconnection. */
    status = ux_utility_semaphore_put(&transfer_initiated_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

/* system_host_change_function_test resources */

static void system_host_change_function_test()
{

    stepinfo("system_host_change_function_test\n");

    disconnect_host_and_slave();

    /* Set change function to null. */
    _ux_system_host -> ux_system_host_change_function = UX_NULL;

    connect_host_and_slave();

    /* Since there's no change function registered, our instance should remain null. */
    if (global_storage_change_function != UX_NULL)
    {

        printf("Error on line %d, %p\n", __LINE__, global_storage_change_function);
        test_control_return(1);
    }

    /* Disconnect with null change function. */
    disconnect_host_and_slave();

    /* Restore change function. */
    _ux_system_host -> ux_system_host_change_function = ux_test_system_host_change_function;

    stepinfo("    connecting...\n");

    /* Connect with non-null change function. */
    connect_host_and_slave();

    /* Since our change function was restored, our storage function should've been invoked. */
    if (global_storage_change_function == UX_NULL)
    {

        printf("Error on line %d, %p\n", __LINE__, global_storage_change_function);
        test_control_return(1);
    }
}

static void direct_calls_test()
{

FX_MEDIA                        media;
UX_TEST_ERROR_CALLBACK_ERROR    error = { UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_ENDPOINT_HANDLE_UNKNOWN };
UX_TEST_ACTION                  actions[] = {
    create_error_match_action_from_error(error),
    { 0 }
};


    stepinfo("direct calls test\n");

    /* These cases have no way of being triggered via normal circumstances, and therefore direct calls are required. An
       explanation should be provided with each.  */

    /* The FileX storage driver handles every request FileX sends to it. */
    stepinfo("    ux_host_class_storage_driver_entry unknown command\n");

    media.fx_media_driver_info = global_storage;
    media.fx_media_reserved_for_user = (ALIGN_TYPE)global_host_storage_class->ux_host_class_media;

    /* Set to unknown driver request. */
    media.fx_media_driver_request = 0xffffffff;

    _ux_host_class_storage_driver_entry(&media);
    if (media.fx_media_driver_status == FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)media.fx_media_driver_status);
        test_control_return(1);
    }

#ifdef BUGFIX /* USBX_89 */
    /* Make interrupt in search fail by setting it's type to non-interrupt. */
    tmp = global_storage->ux_host_class_storage_interface->ux_interface_descriptor.bInterfaceProtocol;
    global_storage->ux_host_class_storage_interface->ux_interface_descriptor.bInterfaceProtocol = UX_HOST_CLASS_STORAGE_PROTOCOL_CBI;
    global_storage->ux_host_class_storage_interface->ux_interface_first_endpoint->ux_endpoint_next_endpoint->ux_endpoint_next_endpoint->ux_endpoint_descriptor.bmAttributes = 0x00;
    ux_test_set_main_action_list_from_array(actions);
    status = _ux_host_class_storage_endpoints_get(global_storage);
    if (status == UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)media.fx_media_driver_status);
        test_control_return(1);
    }
    global_storage->ux_host_class_storage_interface->ux_interface_first_endpoint->ux_endpoint_next_endpoint->ux_endpoint_next_endpoint->ux_endpoint_descriptor.bmAttributes = UX_INTERRUPT_ENDPOINT;
    global_storage->ux_host_class_storage_interface->ux_interface_descriptor.bInterfaceProtocol = tmp;

    global_storage->ux_host_class_storage_interface->ux_interface_descriptor.bNumEndpoints = original_num_endpoints;
#endif
}

#define ENDPOINT_TYPE_BULK          0x02
#define ENDPOINT_TYPE_INTERRUPT     0x03

static void storage_endpoints_get_test()
{

UINT  endpoint_addresses[] = { 0x81, 0x02 };
UCHAR *first_endpoint_fs, *first_endpoint_hs;
UCHAR *second_endpoint_fs, *second_endpoint_hs;
UCHAR *third_endpoint_fs, *third_endpoint_hs;
UX_TEST_ERROR_CALLBACK_ERROR error = { UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_ENDPOINT_HANDLE_UNKNOWN };


    stepinfo("_ux_host_class_storage_endpoints_get\n");

    first_endpoint_fs = bulk_in_endpoint_descriptor_fs;
    first_endpoint_hs = bulk_in_endpoint_descriptor_hs;
    second_endpoint_fs = bulk_out_endpoint_descriptor_fs;
    second_endpoint_hs = bulk_out_endpoint_descriptor_hs;
    third_endpoint_fs = interrupt_in_endpoint_descriptor_hs;
    third_endpoint_hs = interrupt_in_endpoint_descriptor_fs;

    /* None of these should pass enumeration. It's impossible to hit the test cases without breaking everything (device
       assumes bulk endpoints are always first, and one of the test cases is that one of the endpoints before a bulk is
       non-bulk). */

    /** For bulk out: **/

    /* 1) An endpoint before bulk out must be IN. Set them all to IN. */

    disconnect_host_and_slave();

    first_endpoint_fs[Endpoint_bEndpointAddress] = 0x81;
    first_endpoint_hs[Endpoint_bEndpointAddress] = 0x81;

    second_endpoint_fs[Endpoint_bEndpointAddress] = 0x82;
    second_endpoint_hs[Endpoint_bEndpointAddress] = 0x82;

    third_endpoint_fs[Endpoint_bEndpointAddress] = 0x83;
    third_endpoint_hs[Endpoint_bEndpointAddress] = 0x83;

    /* Ignore error. */
    ux_test_add_action_to_main_list(create_error_match_action_from_error(error));

    connect_host_and_slave();

    if (global_media->fx_media_id != 0)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, global_media->fx_media_id);
        test_control_return(1);
    }

    /* 2) An endpoint before bulk out must be OUT and non-bulk. Set the first to OUT and non-bulk. The rest must be IN
       so it fails. */

    disconnect_host_and_slave();

    first_endpoint_fs[Endpoint_bEndpointAddress] = 0x01;
    first_endpoint_hs[Endpoint_bEndpointAddress] = 0x01;
    first_endpoint_fs[Endpoint_bmAttributes] = ENDPOINT_TYPE_INTERRUPT;
    first_endpoint_hs[Endpoint_bmAttributes] = ENDPOINT_TYPE_INTERRUPT;
    first_endpoint_fs[Endpoint_bInterval] = 0x01;
    first_endpoint_hs[Endpoint_bInterval] = 0x01;

    second_endpoint_fs[Endpoint_bEndpointAddress] = 0x82;
    second_endpoint_hs[Endpoint_bEndpointAddress] = 0x82;

    third_endpoint_fs[Endpoint_bEndpointAddress] = 0x83;
    third_endpoint_hs[Endpoint_bEndpointAddress] = 0x83;

    /* This third endpoint is Interrupt, but we must change since if we have multiple Interrupt endpoints, USBX breaks. */
    third_endpoint_fs[Endpoint_bmAttributes] = ENDPOINT_TYPE_BULK;
    third_endpoint_hs[Endpoint_bmAttributes] = ENDPOINT_TYPE_BULK;

    /* Ignore error. */
    ux_test_add_action_to_main_list(create_error_match_action_from_error(error));

    connect_host_and_slave();

    if (global_media->fx_media_id != 0)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, global_media->fx_media_id);
        test_control_return(1);
    }

    /** For bulk in (Note: in order to even search for bulk in, bulk out must be valid, so the first should be bulk
        out): **/

    /* 1) An endpoint before bulk in must be OUT. Set them all to out so it fails. */

    disconnect_host_and_slave();

    first_endpoint_fs[Endpoint_bEndpointAddress] = 0x01;
    first_endpoint_hs[Endpoint_bEndpointAddress] = 0x01;
    first_endpoint_fs[Endpoint_bmAttributes] = ENDPOINT_TYPE_BULK;
    first_endpoint_hs[Endpoint_bmAttributes] = ENDPOINT_TYPE_BULK;

    second_endpoint_fs[Endpoint_bEndpointAddress] = 0x02;
    second_endpoint_hs[Endpoint_bEndpointAddress] = 0x02;

    third_endpoint_fs[Endpoint_bEndpointAddress] = 0x03;
    third_endpoint_hs[Endpoint_bEndpointAddress] = 0x03;

    /* Ignore error. */
    ux_test_add_action_to_main_list(create_error_match_action_from_error(error));

    connect_host_and_slave();

    if (global_media->fx_media_id != 0)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, global_media->fx_media_id);
        test_control_return(1);
    }

    /* 2) An endpoint before bulk in must be IN and non-bulk. Set the second to IN and non-bulk. The rest must be OUT
       so it fails. */

    disconnect_host_and_slave();

    second_endpoint_fs[Endpoint_bEndpointAddress] = 0x82;
    second_endpoint_hs[Endpoint_bEndpointAddress] = 0x82;
    second_endpoint_fs[Endpoint_bmAttributes] = ENDPOINT_TYPE_INTERRUPT;
    second_endpoint_hs[Endpoint_bmAttributes] = ENDPOINT_TYPE_INTERRUPT;
    second_endpoint_fs[Endpoint_bInterval] = 0x01;
    second_endpoint_hs[Endpoint_bInterval] = 0x01;

    third_endpoint_fs[Endpoint_bEndpointAddress] = 0x03;
    third_endpoint_hs[Endpoint_bEndpointAddress] = 0x03;

    /* Ignore error. */
    ux_test_add_action_to_main_list(create_error_match_action_from_error(error));

    connect_host_and_slave();

    if (global_media->fx_media_id != 0)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, global_media->fx_media_id);
        test_control_return(1);
    }

#ifdef BUGFIX
    /** For interrupt in: **/

    /* 1) CBI but no interrupt endpoint */

    disconnect_host_and_slave();

    device_framework_full_speed[bInterfaceProtocol_POS] = UX_HOST_CLASS_STORAGE_PROTOCOL_CBI;
    device_framework_high_speed[bInterfaceProtocol_POS] = UX_HOST_CLASS_STORAGE_PROTOCOL_CBI;
    device_framework_full_speed[bInterfaceSubClass_POS] = UX_HOST_CLASS_STORAGE_SUBCLASS_UFI;
    device_framework_high_speed[bInterfaceSubClass_POS] = UX_HOST_CLASS_STORAGE_SUBCLASS_UFI;

    first_endpoint_fs[Endpoint_bEndpointAddress] = 0x81;
    first_endpoint_hs[Endpoint_bEndpointAddress] = 0x81;
    first_endpoint_fs[Endpoint_bmAttributes] = 0x02;
    first_endpoint_hs[Endpoint_bmAttributes] = 0x02;

    second_endpoint_fs[Endpoint_bEndpointAddress] = 0x01;
    second_endpoint_hs[Endpoint_bEndpointAddress] = 0x01;
    second_endpoint_fs[Endpoint_bmAttributes] = 0x02;
    second_endpoint_hs[Endpoint_bmAttributes] = 0x02;

    third_endpoint_fs[Endpoint_bEndpointAddress] = 0x81;
    third_endpoint_hs[Endpoint_bEndpointAddress] = 0x81;
    third_endpoint_fs[Endpoint_bmAttributes] = 0x02;
    third_endpoint_hs[Endpoint_bmAttributes] = 0x02;

    connect_host_and_slave();

    if (global_media->fx_media_id != 0)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, global_media->fx_media_id);
        test_control_return(1);
    }
#endif

    /* reset_to_bo() resets the endpoint addresses, so we don't have to. */
}

#undef ENDPOINT_TYPE_BULK
#undef ENDPOINT_TYPE_INTERRUPT

/* max_lun_get_test resources */

static VOID max_lun_get_test_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UX_TRANSFER                         *transfer = params->parameter;


    /* Host expects a length of 1. Set it to not 1. */
    transfer->ux_transfer_request_actual_length = 0xdeadbeef;
}

static void max_lun_get_test()
{

UX_TEST_SETUP     setup = { 0 };
UX_TEST_ACTION    actions[2] = { 0 };


    /* We still expect USBX to mount the LUN because 1) max_lun_get always returns UX_SUCCESS 2) when it fails, the
       storage instance's max_lun count is 0 which means 1 LUN, so USBX continues as normal.
       Also, the real world example of when this would happen is the device not being ready yet, so it stalls the
       endpoint. */

    stepinfo("max_lun_get_test\n");

    stepinfo("    transfer_request fails\n");

    disconnect_host_and_slave();

    setup.ux_test_setup_type = UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    setup.ux_test_setup_request = UX_HOST_CLASS_STORAGE_GET_MAX_LUN;

    actions[0].usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    actions[0].function = UX_HCD_TRANSFER_REQUEST;
    actions[0].req_setup = &setup;
    actions[0].req_action = UX_TEST_SETUP_MATCH_REQUEST;
    actions[0].no_return = 0;
    actions[0].status = UX_ERROR;

    ux_test_set_main_action_list_from_array(actions);

    connect_host_and_slave();

    if (global_media->fx_media_id == 0)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, global_media->fx_media_id);
        test_control_return(1);
    }

    stepinfo("    invalid actual length\n");

    disconnect_host_and_slave();

    setup.ux_test_setup_type = UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    setup.ux_test_setup_request = UX_HOST_CLASS_STORAGE_GET_MAX_LUN;

    actions[0].usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    actions[0].function = UX_HCD_TRANSFER_REQUEST;
    actions[0].req_setup = &setup;
    actions[0].req_action = UX_TEST_SETUP_MATCH_REQUEST;
    actions[0].no_return = 0;
    actions[0].action_func = max_lun_get_test_action_func;
    actions[0].status = UX_SUCCESS;

    ux_test_set_main_action_list_from_array(actions);

    connect_host_and_slave();

    if (global_media->fx_media_id == 0)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, global_media->fx_media_id);
        test_control_return(1);
    }
}

/* media_capacity_get_test resources */

static void media_capacity_get_test()
{

UINT                            i;
UX_TEST_ERROR_CALLBACK_ERROR    error = { UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_TRANSFER_STALLED };
ULONG                           test_sector_size = 2*UX_HOST_CLASS_STORAGE_SECTOR_SIZE_FAT;
UX_TEST_ACTION                  actions[2] = { 0 };


    stepinfo("media_capacity_get_test\n");

    /* How: We do this by overriding the device's media_status function to return an error. Also, this assumes that
       media_capacity_get is the first BO transfer to the device. */
    stepinfo("    transport succeeds, error sense code, fails 10 times\n");

    disconnect_host_and_slave();
    for (i = 0; i < UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RETRY; i++)
    {
        ux_test_add_action_to_main_list(create_data_match_action(global_enum_thread, global_cbw_data_media_capacity_get_sbc, sizeof(global_cbw_data_media_capacity_get_sbc)));
        ux_test_add_action_to_main_list(create_device_media_status_fail_action());
        ux_test_add_action_to_main_list(create_error_match_action_from_error(global_transfer_stall_error));
    }
    connect_host_and_slave();

    if (ux_test_check_actions_empty() == UX_FALSE)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, global_media->fx_media_id);
        test_control_return(1);
    }

    stepinfo("    non-default sector size\n");

    disconnect_host_and_slave();

    /* Change the sector size. */
    global_sector_size = test_sector_size;

    /* USBX has a default sector size, therefore in order to test this, we change the sector size to the non-default. */
    global_persistent_slave_storage -> ux_slave_class_storage_lun[0].ux_slave_class_storage_media_block_length = test_sector_size;

    connect_host_and_slave();

    /* Ensure media_capacity_get() received the correctomundo sector size. */
    if (global_storage -> ux_host_class_storage_sector_size != test_sector_size)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, global_storage -> ux_host_class_storage_sector_size);
        test_control_return(1);
    }

    basic_test(global_media, 1, "        basic_test\n");

    /* Set the sector size back to normal. */
    reset_to_bo();

    stepinfo("    transport fails\n");

    disconnect_host_and_slave();

    UX_TEST_ACTION media_capacity_get_cbw_disconnect_actions[] = {
        create_disconnect_on_transfer_data_match_action(global_enum_thread, global_cbw_data_media_capacity_get_sbc, sizeof(global_cbw_data_media_capacity_get_sbc)),
        create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_TRANSFER_TIMEOUT),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(media_capacity_get_cbw_disconnect_actions);
    UX_TEST_ASSERT(connect_host_and_slave() != UX_SUCCESS);
}

/* media_characteristics_get_test resources */

static void media_characteristics_get_test()
{

    stepinfo("media_characteristics_get_test\n");

    stepinfo("    correct characteristics received\n");

    disconnect_host_and_slave();
    global_persistent_slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_removable_flag = 0x00;
    connect_host_and_slave();

    if (global_storage->ux_host_class_storage_lun_removable_media_flags[0] != 0x00)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, global_media->fx_media_id);
        test_control_return(1);
    }

    basic_test(global_media, 1, "    basic test\n");

    /* Reset removable flag. */
    reset_to_bo();

    stepinfo("    fail on transfer\n");

    disconnect_host_and_slave();
    UX_TEST_ACTION media_characteristics_get_disconnect_actions[] = {
        create_disconnect_on_transfer_data_match_action(global_enum_thread, global_cbw_data_media_characteristics_get_sbc, sizeof(global_cbw_data_media_characteristics_get_sbc)),
        create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_TRANSFER_TIMEOUT),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(media_characteristics_get_disconnect_actions);;
    UX_TEST_ASSERT(connect_host_and_slave() != UX_SUCCESS);
}

/* ux_test_host_class_storage_media_read_test resources */

static void uhcsmrt_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UX_TRANSFER                         *transfer = params->parameter;


    /* Make it NOT correct. */
    global_storage -> ux_host_class_storage_data_phase_length = ~(global_storage -> ux_host_class_storage_data_phase_length);
}

/* ux_host_class_storage_request_sense_test resources */

static void ux_host_class_storage_request_sense_test()
{

UINT                         status;
UX_TEST_ACTION     actions[3] = { 0 };


    stepinfo("ux_host_class_storage_request_sense_test\n");

    /* We accomplish this by forcing a REQUEST_SENSE command to fail via error in CSW. */
    stepinfo("    recursive REQUEST_SENSE command\n");

    lock_out_storage_thread();

    /* In order to get USBX to send the REQUEST_SENSE comand, a CSW must contain an error. Therefore, we need to error
       CSWs. */
    actions[0] = create_csw_error_action(tx_thread_identify(), UX_HOST_CLASS_STORAGE_CSW_FAILED);
    actions[1] = create_csw_error_action(tx_thread_identify(), UX_HOST_CLASS_STORAGE_CSW_FAILED);
    ux_test_set_main_action_list_from_array(actions);

    /* This should fail because the transport itself fails, not just the CSW. */
    status = ux_test_host_class_storage_media_read(global_storage, 10, 1, global_buffer);
    if (status == UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    lock_in_storage_thread();
}

/* ux_host_class_storage_start_stop_test resources */

static void uhcsst_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UX_TRANSFER                         *transfer = params->parameter;
UX_HOST_CLASS_STORAGE               *storage = get_internal_host_storage_instance();


    storage->ux_host_class_storage_sense_code = 0xff;
}

static void ux_host_class_storage_start_stop_test()
{

UCHAR                       cbw_data_start_stop[] = { 0x55, 0x53, 0x42, 0x43, 0x43, 0x42, 0x53, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UX_TEST_ACTION              actions[51] = { 0 };
UINT                        i;


    /* The host sends the START_STOP command only to UFI devices during enumeration. */
    stepinfo("ux_host_class_storage_start_stop_test\n");

    /* We do this by detecting the CBW and setting to sense code to error. */
    stepinfo("    50 retries fail test\n");

    disconnect_host_and_slave();

    /* Set this to UFI. */
    switch_to_protocol(UX_HOST_CLASS_STORAGE_PROTOCOL_CBI);

    /* Match CBW and set the sense code to error. */
    actions[0].usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    actions[0].function = UX_HCD_TRANSFER_REQUEST;
    actions[0].req_data = cbw_data_start_stop;
    actions[0].req_actual_len = sizeof(cbw_data_start_stop);
    actions[0].action_func = uhcsst_action_func;
    actions[0].no_return = 1;

    /* Host will try to send a REQUEST_SENSE command to device. Make sure it contains an error. */
    for (i = 1; i < (ARRAY_COUNT(actions) - 1); i++)
    {
        actions[i] = actions[0];
    }

    ux_test_set_main_action_list_from_array(actions);

    connect_host_and_slave();

    if (global_media->fx_media_id != 0)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
}

/* ux_host_class_storage_transport_test resources */

/* This tests transport, transport_bo, transport_cb, and transport_cbi. */
static void ux_host_class_storage_transport_test()
{

UX_TEST_ERROR_CALLBACK_ERROR    transfer_timeout_error = { UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_TRANSFER_TIMEOUT };


    stepinfo("ux_host_class_storage_transport_test\n");

    stepinfo("    CSW 2 timeouts\n");

    DEVICE_MEDIA_READ_WRITE_TIMEOUT_DATA timeout_data;
    timeout_data.transfer_request = &global_storage->ux_host_class_storage_bulk_in_endpoint->ux_endpoint_transfer_request;
    timeout_data.num_timeouts = 2;

    UX_TEST_ACTION csw_multiple_retries_fails[] = {
        create_device_media_write_block_action(&timeout_data),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(csw_multiple_retries_fails);

    lock_out_storage_thread();
    UX_TEST_CHECK_NOT_SUCCESS(ux_test_host_class_storage_media_write(global_storage, 10, 1, global_buffer));
    receive_device_csw();
    lock_in_storage_thread();
    basic_test(global_media, 1, "        basic test\n");

    stepinfo("    CBW transfer fail\n");

    /* Set action for making CBW transfer fail. */
    UX_TEST_ACTION cbw_disconnect_actions[] = {
        create_cbw_disconnect_action(tx_thread_identify()),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(cbw_disconnect_actions);

    lock_out_storage_thread();
    UX_TEST_CHECK_NOT_SUCCESS(ux_test_host_class_storage_media_write(global_storage, 0, 1, global_buffer));
    lock_in_storage_thread();
    connect_host_and_slave();

#if 0 /* USBX_54 */
    stepinfo("    CBW stalls\n");

    actions[0] = create_cbw_stall_action(tx_thread_identify());
    ux_test_set_actions(actions);

    /* Do anything that triggers a CBW. */
    status = ux_test_media_write(global_storage, 0, 1, global_buffer, 0, UX_FALSE);
    if (status == UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
#endif

    /* Note: the device never sends a PHASE ERROR, since it's optional. Therefore, we do the next best thing. */
    stepinfo("    CSW status is Phase Error\n");

    /* Set action for making CSW contain Phase Error. */
    UX_TEST_ACTION csw_phase_error[] = {
        create_csw_error_action(tx_thread_identify(), UX_HOST_CLASS_STORAGE_CSW_PHASE_ERROR),
        create_endpoint_reset_match_action(tx_thread_identify(), &global_setup_endpoint_reset_out2),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(csw_phase_error);

    lock_out_storage_thread();
    UX_TEST_CHECK_SUCCESS(ux_test_host_class_storage_media_write(global_storage, 10, 1, global_buffer));
    lock_in_storage_thread();
    UX_TEST_ASSERT(ux_test_check_actions_empty());

    stepinfo("    CSW transfer fail\n");

    /* Set action for making CSW transfer fail. */
    UX_TEST_ACTION csw_disconnect_actions[] = {
        create_csw_disconnect_action(tx_thread_identify()),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(csw_disconnect_actions);

    lock_out_storage_thread();
    UX_TEST_CHECK_NOT_SUCCESS(ux_test_host_class_storage_media_write(global_storage, 10, 1, global_buffer));
    lock_in_storage_thread();
    connect_host_and_slave();

    stepinfo("    CSW semaphore_get() fails due to disconnect\n");

    UX_TEST_ACTION csw_semaphore_get_disconnect_actions[] = {
        create_csw_match_action(tx_thread_identify()),
        create_semaphore_get_disconnect_action(tx_thread_identify(), &global_storage->ux_host_class_storage_bulk_in_endpoint->ux_endpoint_transfer_request.ux_transfer_request_semaphore, UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(csw_semaphore_get_disconnect_actions);

    lock_out_storage_thread();
    UX_TEST_CHECK_NOT_SUCCESS(ux_test_host_class_storage_media_write(global_storage, 10, 1, global_buffer));
    lock_in_storage_thread();
    connect_host_and_slave();
}

/* thirteen_cases_test resources
   These tests ensure the host acts conformant to the USB MSC BO spec regarding the thirteen cases specified in section
   6.7. */

static UX_TEST_ACTION create_match_unit_test_ready_cbw(TX_THREAD *thread)
{

UX_TEST_ACTION  action = {0};


    /* Action for matching CBW of unit ready test. */
    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_data = global_cbw_data_unit_ready_test_sbc;
    action.req_actual_len = sizeof(global_cbw_data_unit_ready_test_sbc);
    action.no_return = 1;
    action.thread_to_match = thread;

    return action;
}

static UINT tcht_media_read_test_error_action_func_calls;
static UINT tcht_media_read_test_error_action_func(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

    if (!tcht_media_read_test_error_action_func_calls++)
    {

        *media_status = (ULONG)-1;
        return UX_ERROR;
    }
    else
        return default_device_media_read(storage, lun, data_pointer, number_blocks, lba, media_status);
}

static UINT tcht_media_write_test_error_action_func_calls;
static UINT tcht_media_write_test_error_action_func(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

    if (!tcht_media_write_test_error_action_func_calls++)
    {

        *media_status = (ULONG)-1;
        return UX_ERROR;
    }
    else
        return default_device_media_write(storage, lun, data_pointer, number_blocks, lba, media_status);
}

static void stall_while_receiving_csw_test()
{

    /* Case: On a STALL condition receiving the CSW, then:
        -The host shall clear the Bulk-In pipe.
        -The host shall attempt to receive the CSW again.

        We "hardcode" it because there's no reason AFAIK to trigger this case. */
    stepinfo("stall_while_receiving_csw_test\n");

    UX_TEST_ACTION actions[] = {
        /* Action for matching CBW of unit ready test. */
        create_cbw_match_action(tx_thread_identify()),
        /* Force the CSW to stall. */
        create_csw_stall_action(tx_thread_identify()), /* Not actually stalled on device side.  */
        /* Match bulk-in endpoint reset. */
        create_endpoint_reset_match_action(tx_thread_identify(), &global_setup_endpoint_reset_in1),
        /* Match CSW. */
        create_csw_match_action(tx_thread_identify()),
        { 0 }
    };

    lock_out_storage_thread();

    ux_test_set_expedient(UX_TRUE);
    ux_test_set_main_action_list_from_array(actions);
    UX_TEST_CHECK_SUCCESS(ux_test_host_class_storage_media_read(global_storage, 10, sizeof(global_buffer_2x_slave_buffer_size)/UX_HOST_CLASS_STORAGE_SECTOR_SIZE_FAT, global_buffer_2x_slave_buffer_size));
    ux_test_set_expedient(UX_FALSE);

    lock_in_storage_thread();

    /* Ensure we "attempt to receive the CSW again" and "clear the Bulk-In pipe" */
    if (ux_test_check_actions_empty() != UX_TRUE)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
}

static void csw_contains_phase_error_test()
{

    stepinfo("csw_contains_phase_error_test\n");

    lock_out_storage_thread();

    UX_TEST_ACTION actions[] = {
        create_cbw_match_action(tx_thread_identify()),
        create_csw_error_action(tx_thread_identify(), UX_HOST_CLASS_STORAGE_CSW_PHASE_ERROR),
        create_mass_storage_reset_match_action(tx_thread_identify()),
        create_request_sense_error_action(tx_thread_identify(), 0xff),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(actions);

    UX_TEST_CHECK_SUCCESS(ux_test_host_class_storage_media_read(global_storage, 10, sizeof(global_buffer_2x_slave_buffer_size)/UX_HOST_CLASS_STORAGE_SECTOR_SIZE_FAT, global_buffer_2x_slave_buffer_size));

    lock_in_storage_thread();

    /* Ensure we "attempt to receive the CSW again" and "clear the Bulk-In pipe" */
    UX_TEST_ASSERT(ux_test_check_actions_empty());
}

static void thirteen_cases_4_5_test_host()
{

UINT              status;
UX_TEST_ACTION    actions[] = {
    create_device_media_read_fail_action(),
    create_error_match_action_from_error(ux_error_hcd_transfer_stalled),
    create_endpoint_reset_match_action(tx_thread_identify(), &global_setup_endpoint_reset_in1),
    { 0 }
};


    /* For case 4 and 5, the device should stall during data stage. This is what
       we do during this test. */
    stepinfo("thirteen_cases_4_5_test_host\n");

    lock_out_storage_thread();

    ux_test_set_main_action_list_from_array(actions);

    /* Make sure it requires multiple transfers (large data size). */
    status = ux_test_host_class_storage_media_read(global_storage, 10, sizeof(global_buffer_2x_slave_buffer_size)/UX_HOST_CLASS_STORAGE_SECTOR_SIZE_FAT, global_buffer_2x_slave_buffer_size);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    lock_in_storage_thread();

    if (global_media->fx_media_id == 0)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, global_media->fx_media_id);
        test_control_return(1);
    }

    /* Ensure we "clear the Bulk-In pipe." */
    if (ux_test_check_actions_empty() != UX_TRUE)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
}

static void thirteen_cases_9_11_test_host()
{

UINT                        status;
UINT                        num_transfers;
/* Note that we use the DEVICE's max buffer size because we need the device to process it (for example, if it's less than
   the host's and we use the host's, then it would be the same as only sending one transfer). */
ULONG                       write_sector_counts[] = { UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE/UX_HOST_CLASS_STORAGE_SECTOR_SIZE_FAT, UX_TEST_MULTIPLE_TRANSFERS_SECTOR_COUNT };

/* We don't expect any errors since the device doesn't stall until after we've sent all our data, we don't actually
   detect the stall. Instead, we see that the CSW contains an error then clear the endpoint. */
UX_TEST_ACTION              one_transfer_actions[] = {
    create_device_media_write_fail_action(),
    create_endpoint_reset_match_action(tx_thread_identify(), &global_setup_endpoint_reset_out2),
    { 0 }
};

/* This time, we will detect the stall in transport_bo.c. */
UX_TEST_ACTION              two_transfers_actions[] = {
    create_device_media_write_fail_action(),
    create_error_match_action_from_error(ux_error_hcd_transfer_stalled),
    create_endpoint_reset_match_action(tx_thread_identify(), &global_setup_endpoint_reset_out2),
    { 0 }
};

    /* For these cases, the device should stall the bulk out during the data
       phase. That is what we do during this test.

       We do this test twice for good measuer: one that requires one slave transfer,
       and one that requires two slave transfers. */
    stepinfo("thirteen_cases_9_11_test_host\n");

    for (num_transfers = 1; num_transfers < 3; num_transfers++)
    {

        stepinfo("    num_transfers: %d\n", num_transfers);

        if (num_transfers == 1)
        {
            ux_test_set_main_action_list_from_array(one_transfer_actions);
        }
        else if (num_transfers == 2)
        {
            ux_test_set_main_action_list_from_array(two_transfers_actions);
        }

        lock_out_storage_thread();

        /* Make sure it requires multiple transfers (large data size). */
        status = ux_test_host_class_storage_media_write(global_storage, 10, write_sector_counts[num_transfers - 1], global_buffer_2x_slave_buffer_size);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
            test_control_return(1);
        }

        /* Ensure we "clear the Bulk-Out pipe." Note that we actually do it twice, once in transport_bo() and once in
           transport(). See the note in transport() as to why this is indeed the case. */
        if (ux_test_check_actions_empty() != UX_TRUE)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        if (global_storage->ux_host_class_storage_bulk_out_endpoint->ux_endpoint_transfer_request.ux_transfer_request_semaphore.tx_semaphore_count != 0)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        lock_in_storage_thread();
    }
}

/* get_no_device_memory_free_amount resources. */

static void get_no_device_free_memory_amount_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
ULONG                                           *no_device_memory_free_amount = (ULONG *)action->user_data;


    /* Log number of memory allocations right before storage class initialization. */
    *no_device_memory_free_amount = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available;
}

/* device_not_ready_during_enumeration_test resources */

static void device_not_ready_during_enumeration_test()
{

    stepinfo("device_not_ready_during_enumeration_test\n");

    disconnect_host_and_slave();

    UX_TEST_ACTION actions[] = {
        /* This causes the UNIT READY TEST in _ux_host_class_storage_media_mount to fail. */
        create_test_unit_ready_match_action(global_enum_thread),
        create_device_media_status_fail_action(),
        /* These two causes the UNIT READY TEST to receive the UNIT_ATTENTION sense key, which will mount us! */
        create_csw_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_CSW_FAILED),
        create_request_sense_error_action(global_storage_thread, UX_HOST_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION),
        { 0 }
    };
    ux_test_set_main_action_list_from_array(actions);

    connect_host_and_slave();

    /* Wait for storage thread to mount us. */
    while (!_storage_media_is_mounted())
        tx_thread_sleep(10);

    /* We should be successfully mounted. Run a basic test. */
    basic_test(global_media, 1, "    basic test\n");
}

static void disconnect_during_storage_thread_unmounting_media_test()
{

    stepinfo("disconnect_during_storage_thread_unmounting_media_test\n");

    UX_TEST_ACTION not_ready_actions = { 0 };
    add_unit_ready_test_not_ready_actions(&not_ready_actions);
    ux_test_add_action_to_user_list(&not_ready_actions, create_disconnect_on_thread_preemption_change(global_storage_thread, global_storage_thread, UX_HOST_CLASS_STORAGE_THREAD_PRIORITY_CLASS));
    ux_test_set_main_action_list_from_list(&not_ready_actions);
    let_storage_thread_run();
}

#ifdef UX_TEST_RACE_CONDITION_TESTS_ON

/* host_stack_simultaneous_control_transfer_test resources */

static void hssctt_thread2_entry(ULONG input)
{

UINT            status;


    /** This is Thread2 **/

    /* ux_host_stack_interface_setting_select() with alternate setting 0. Should override Thread1's transfer. */
    status = ux_host_stack_interface_setting_select(global_host_device->ux_device_first_configuration->ux_configuration_first_interface);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n",__LINE__,(ULONG)status);
        test_control_return(1);
    }

    /* Resume Thread1. */
    status = ux_utility_semaphore_put(&global_test_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

static void hssctt_action_func(UX_TEST_ACTION *action, VOID *_params)
{

UX_TEST_HCD_SIM_HOST_ENTRY_PARAMS   *params = (UX_TEST_HCD_SIM_HOST_ENTRY_PARAMS *)_params;
UINT                                status;
UX_TEST_WORKER_WORK                 work;


    work.function = hssctt_thread2_entry;

    /* Remember we are being called from ux_host_stack_interface_set.c */

    /* Run Thread2 */
    ux_test_worker_add_work(&work);

    /* Wait for Thread2 to finish. */
    status = ux_utility_semaphore_get(&global_test_semaphore, TX_WAIT_FOREVER);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

static void host_stack_simultaneous_control_transfers_cause_override_test()
{

UINT                    status;
UX_INTERFACE            *interface_alternate_setting1;
UX_TRANSFER             *transfer_request;
UCHAR                   buffer;


    /* Proof of concept for USBX_77 */

    /* Here is a REALISTIC example of what could POSSIBLY happen:

        -Background storage thread does a UNIT_READY_TEST.
        -It fails on the device side, so device stalls endpoint.
        -Host just sees that it failed and tries to reset the device (via MSC command on _control endpoint)).
        -After setting up transfer request but _before_ calling ux_host_stack_transfer_request, storage thread gets
         preempted by ThreadA.
        -ThreadA decides it's time to select another interface setting or change the configuration (via
         _control endpoint_).
        -ThreadA _overrides_ storage thread's transfer request.
        -Once storage thread runs again, it wlil send the same request as ThreadA.

        Not only that, but the reverse could happen: storage thread overrides ThreadA's transfer!

       See? this test isn't a waste of time! */

    stepinfo("host_stack_simultaneous_control_transfer_test\n");

    UX_TEST_ACTION actions[2] = { 0 };
    actions[0]._usbx_function = UX_TEST_HOST_STACK_INTERFACE_SET;
    actions[0].bInterfaceNumber = 0;
    actions[0].bAlternateSetting = 1;
    actions[0].action_func = hssctt_action_func;

    /** This is Thread1. **/

    lock_out_storage_thread();

    /* The device stack calls the class to handle SET_INTERFACE requests. Problem is that device classes don't actually
       support it and throw an error. In case this is a bug, I've documented it as BUG_ID_48. */
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED));

    /* Set action so that right before transfer_request(), we get preempted by Thread2. */
    ux_test_set_main_action_list_from_array(actions);

    interface_alternate_setting1 = global_storage->ux_host_class_storage_interface->ux_interface_next_interface;

    /* Call ux_host_stack_interface_setting_select() with alternate setting 1. Should be the interface right after the
       current one. */
    /* When we resume, our transfer should've been overridden. */
    status = ux_host_stack_interface_setting_select(interface_alternate_setting1);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n",__LINE__,(ULONG)status);
        test_control_return(1);
    }

    /* Get the current alternate setting from the device (there's no API for this, we'll have to send the transfer
       manually). */
    transfer_request = &global_host_device->ux_device_control_endpoint.ux_endpoint_transfer_request;
    transfer_request -> ux_transfer_request_data_pointer =      &buffer;
    transfer_request -> ux_transfer_request_requested_length =  1;
    transfer_request -> ux_transfer_request_function =          UX_GET_INTERFACE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n",__LINE__,(ULONG)status);
        test_control_return(1);
    }

    if (buffer != 1)
    {

        printf("Error on line %d, error code: 0x%lx\n",__LINE__,(ULONG)status);
        test_control_return(1);
    }

    lock_in_storage_thread();
}

/* host_stack_simultaneous_control_transfers_cause_extra_sem_count_test resources */

static void hssctcesmt_do_configuration_reset_function(ULONG input)
{

UINT status;


    /* We just need to do A control transfer, doesn't matter which one. */
    status = ux_host_stack_device_configuration_reset(global_host_device);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

static void hssctcesmt_reactivate_hcd_timer(ULONG input)
{

UINT            status;
UX_HCD_SIM_HOST *hcd_sim_host = global_hcd -> ux_hcd_controller_hardware;


    /* Reactivate HCD timer. */
    status = tx_timer_activate(&hcd_sim_host -> ux_hcd_sim_host_timer);
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}

static void host_stack_simultaneous_control_transfers_cause_extra_sem_count_test()
{

UINT                        status;
UX_TEST_WORKER_WORK         work_transfer = { 0 };
UX_HCD_SIM_HOST             *hcd_sim_host = global_hcd -> ux_hcd_controller_hardware;


    /* Proof of concept for USBX_78. */

    stepinfo("    simulataneous control transfers test\n");

    work_transfer.function = hssctcesmt_do_configuration_reset_function;

    /* Deactivate HCD timer. No transfers are processed during this time. */
    status = tx_timer_deactivate(&hcd_sim_host -> ux_hcd_sim_host_timer);
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Start our timer. */
    status = tx_timer_create(&global_timer, "timer", hssctcesmt_reactivate_hcd_timer, 0, 100, 0, TX_AUTO_ACTIVATE);
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Place work for doing transfer at the same we are in worker thread. */
    ux_test_worker_add_work(&work_transfer);

    /* Do our control transfer (remember that this is blocking). */
    hssctcesmt_do_configuration_reset_function(0);

    /* Our timer should reactivate HCD timer. */

    /* If there are no bugs, the device protection semaphore's count is 1 (but with bugs, it should be 2). */
    if (global_host_device->ux_device_protection_semaphore.tx_semaphore_count != 1)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    status = tx_timer_delete(&global_timer);
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }
}
#endif

/* synchronize_cache_test resources. */

/* Opcodes.  */
#define UX_SLAVE_CLASS_STORAGE_SCSI_SYNCHRONIZE_CACHE 0x35

/* Define Storage Class read format command constants.  */
#define UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_OPERATION                       0
#define UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_FLAGS                           1
#define UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_LBA                             2

#define UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_NUMBER_OF_BLOCKS                7
#define UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_COMMAND_LENGTH_SBC              10

#define UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_FLAGS_IMMED                     0x02

static void synchronize_cache_test()
{

UCHAR   *cbw;
UINT    number_of_blocks;
UINT    i;


    stepinfo("synchronize cache test\n");

    /* For this test, the device will reply with an error in the CSW. This will
       cause us to send a REQUEST SENSE. We ensure the REQUEST SENSE is valid.  */
    stepinfo("    synchronize cache test - callback NULL\n");

    /* Set callback to NULL.  */
    global_slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_flush = 0;

    number_of_blocks = 0xffff;

    /* Initialize the CBW for this command.  */
    _ux_host_class_storage_cbw_initialize(global_storage, 0, 0, UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_COMMAND_LENGTH_SBC);

    /* Prepare the SYNCHRONIZE CACHE command block.  */
    cbw =  (UCHAR *) global_storage -> ux_host_class_storage_cbw;
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_OPERATION) =  UX_SLAVE_CLASS_STORAGE_SCSI_SYNCHRONIZE_CACHE;
    _ux_utility_long_put_big_endian((cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_LBA), 0);
    _ux_utility_short_put_big_endian((cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_NUMBER_OF_BLOCKS), number_of_blocks);

    UX_TEST_CHECK_SUCCESS(_ux_host_class_storage_transport(global_storage, UX_NULL));
    UX_TEST_ASSERT(global_storage->ux_host_class_storage_cbw[UX_HOST_CLASS_STORAGE_CSW_STATUS] == UX_SUCCESS);
    UX_TEST_ASSERT(ux_test_check_actions_empty());

    /* Restore callback.  */
    global_slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_flush = default_device_media_flush;

    stepinfo("    synchronize cache test - different LBAs and number of blocks\n");

    /* Try with different blocks and LBAs. */
    UINT lbas[] = {0, 1, 2, 3, 4, 5};
    USHORT number_blocks[] = {0xffff, 10, 9, 8, 7, 6};
    for (i = 0; i < ARRAY_COUNT(lbas); i++)
    {

        stepinfo("        synchronize cache test - different LBAs and number of blocks - %d\n", i);

        /* Initialize the CBW for this command.  */
        _ux_host_class_storage_cbw_initialize(global_storage, 0, 0, UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_COMMAND_LENGTH_SBC);

        /* Prepare the SYNCHRONIZE CACHE command block.  */
        cbw =  (UCHAR *) global_storage -> ux_host_class_storage_cbw;
        *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_OPERATION) =  UX_SLAVE_CLASS_STORAGE_SCSI_SYNCHRONIZE_CACHE;
        _ux_utility_long_put_big_endian((cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_LBA), lbas[i]);
        _ux_utility_short_put_big_endian((cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_NUMBER_OF_BLOCKS), number_blocks[i]);

        /* Action for matching media flush to make sure device receives it. */
        UX_TEST_ACTION action = { 0 };
        action.usbx_function = UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_FLUSH;
        action.lun = 0;
        action.lba = lbas[i];
        action.number_blocks = number_blocks[i];
        ux_test_add_action_to_main_list(action);

        UX_TEST_CHECK_SUCCESS(_ux_host_class_storage_transport(global_storage, UX_NULL));
        UX_TEST_ASSERT(global_storage->ux_host_class_storage_cbw[UX_HOST_CLASS_STORAGE_CSW_STATUS] == UX_SUCCESS);
        UX_TEST_ASSERT(ux_test_check_actions_empty());
    }

    stepinfo("    synchronize cache test - immediate bit set\n");

    number_of_blocks = 0xffff;

    /* Initialize the CBW for this command.  */
    _ux_host_class_storage_cbw_initialize(global_storage, 0, 0, UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_COMMAND_LENGTH_SBC);

    /* Prepare the SYNCHRONIZE CACHE command block.  */
    cbw =  (UCHAR *) global_storage -> ux_host_class_storage_cbw;
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_OPERATION) =  UX_SLAVE_CLASS_STORAGE_SCSI_SYNCHRONIZE_CACHE;
    _ux_utility_long_put_big_endian((cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_LBA), 0);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_FLAGS) |=  UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_FLAGS_IMMED;
    _ux_utility_short_put_big_endian((cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_SYNCHRONIZE_CACHE_NUMBER_OF_BLOCKS), number_of_blocks);

    /* Action for matching media flush to make sure device receives it. */
    UX_TEST_ACTION action_immediate_bit_set = { 0 };
    action_immediate_bit_set.usbx_function = UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_FLUSH;
    action_immediate_bit_set.lun = 0;
    action_immediate_bit_set.lba = 0;
    action_immediate_bit_set.number_blocks = number_of_blocks;
    ux_test_add_action_to_main_list(action_immediate_bit_set);

    UX_TEST_CHECK_SUCCESS(_ux_host_class_storage_transport(global_storage, UX_NULL));
    UX_TEST_ASSERT(global_storage->ux_host_class_storage_cbw[UX_HOST_CLASS_STORAGE_CSW_STATUS] == UX_SUCCESS);
    UX_TEST_ASSERT(ux_test_check_actions_empty());

    basic_test(global_media, 1, "        running basic test\n");
}

/** This tests the case where the media is unremovable. **/

static void unremovable_media_test()
{

    stepinfo("unremovable_media_test\n");

    ux_test_disconnect_slave_and_host_wait_for_enum_completion();

    /* Change the device's removable media flag to unremovable. */
    global_persistent_slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_removable_flag = 0x00;

    /* Now connect. */
    ux_test_connect_slave_and_host_wait_for_enum_completion();

    /* Wait for storage thread to run. */
    tx_thread_sleep(2*UX_HOST_CLASS_STORAGE_THREAD_SLEEP_TIME);
}

/* transfer_timeout_test resources */

static TX_THREAD ttt_asynchronous_transfer_thread;
static UCHAR ttt_asynchronous_transfer_thread_stack[4096];

static void my_completion_function(UX_TRANSFER *transfer_request)
{

    /* transfer_request_abort() will call this. */
    UX_TEST_ASSERT(transfer_request->ux_transfer_request_completion_code == UX_TRANSFER_STATUS_ABORT);
}

static VOID ttt_asynchronous_transfer_thread_entry(ULONG input)
{

UX_TRANSFER *transfer_request;


    UX_THREAD_EXTENSION_PTR_GET(transfer_request, UX_TRANSFER, input);

    /* Make sure the transfer never completes. */
    ux_test_add_action_to_main_list(create_timeout_on_transfer_action(tx_thread_identify()));

    transfer_request->ux_transfer_request_completion_function = my_completion_function;
    transfer_request->ux_transfer_request_data_pointer = global_buffer;
    transfer_request->ux_transfer_request_requested_length = UX_HOST_CLASS_STORAGE_CBW_LENGTH;
    UX_TEST_CHECK_SUCCESS(ux_host_stack_transfer_request(transfer_request));
    UX_TEST_ASSERT(ux_test_check_actions_empty());

    tx_thread_suspend(tx_thread_identify());
}

static void transfer_timeout_test()
{

UCHAR       original_expedient_value;
UX_TRANSFER *transfer_request = &global_storage->ux_host_class_storage_bulk_in_endpoint->ux_endpoint_transfer_request;


    /* This is to test USBX-115. In summary, the bug is that if the thread timeouts
       waiting on a semaphore, then it usually calls transfer_abort(), but abort()
       does a semaphore_put() so there is extra count.  */
    stepinfo("transfer_timeout_test\n");

    /* Synchronous means no completion function is set. */
    stepinfo("    transfer_timeout_test - synchronous\n");

    /* Since we return from the transfer immediately, we need to turn on
       expedient mode.  */
    ux_test_turn_on_expedient(&original_expedient_value);

    /* Have the transfer timeout. */
    ux_test_add_action_to_main_list(create_timeout_on_cbw_action(tx_thread_identify()));
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_TRANSFER_TIMEOUT));

    /* Do a transfer - the first transfer should be a CBW. */
    do_any_write();
    UX_TEST_ASSERT(ux_test_check_actions_empty());

    /* Check the bulk-out's transfer semaphore count. */
    UX_TEST_ASSERT(global_storage->ux_host_class_storage_bulk_out_endpoint->ux_endpoint_transfer_request.ux_transfer_request_semaphore.tx_semaphore_count == 0);

    /* Asynchronous means a completion function is set. */
    stepinfo("    transfer_timeout_test - asynchronous\n");

    /* We use the bulk out endpoint cause we have no others to use. */
    lock_out_storage_thread();

    UX_TEST_CHECK_SUCCESS(tx_thread_create(&ttt_asynchronous_transfer_thread, "ttt_asynchronous_transfer_thread",
                                           ttt_asynchronous_transfer_thread_entry, (ULONG)(ALIGN_TYPE)transfer_request,
                                           ttt_asynchronous_transfer_thread_stack, sizeof(ttt_asynchronous_transfer_thread_stack),
                                           20, 20, 10, TX_DONT_START));
    UX_THREAD_EXTENSION_PTR_SET(&ttt_asynchronous_transfer_thread, transfer_request);
    UX_TEST_CHECK_SUCCESS(tx_thread_resume(&ttt_asynchronous_transfer_thread));

    /* Wait for thread to do transfer. */
    int thread_state;
    do
    {

        UX_TEST_CHECK_SUCCESS(tx_thread_info_get(&ttt_asynchronous_transfer_thread, UX_NULL, &thread_state, UX_NULL, UX_NULL, UX_NULL, UX_NULL, UX_NULL, UX_NULL));
    } while(thread_state != TX_SUSPENDED);

    /* Now abort transfer. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_transfer_request_abort(transfer_request));

    /* Make sure the semaphore wasn't put(). */
    UX_TEST_ASSERT(transfer_request->ux_transfer_request_semaphore.tx_semaphore_count == 0);

    UX_TEST_CHECK_SUCCESS(tx_thread_terminate(&ttt_asynchronous_transfer_thread));
    UX_TEST_CHECK_SUCCESS(tx_thread_delete(&ttt_asynchronous_transfer_thread));

    lock_in_storage_thread();

    ux_test_set_expedient(original_expedient_value);
}

static void ux_test_thread_host_simulation_entry(ULONG arg)
{

UCHAR   original_expedient_value;
UINT    status;
ULONG   error_count = 0;
UINT    i;
void(*tests[])() =
{

    transfer_timeout_test,
    unremovable_media_test,
    synchronize_cache_test,
    fx_media_read_test,

    thirteen_cases_4_5_test_host,
    thirteen_cases_9_11_test_host,
    stall_while_receiving_csw_test,
    csw_contains_phase_error_test,
    basic_test_with_globals,
    test_unit_ready_test,
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    first_sector_non_boot_test,
#endif
    abort_media_test,
    flush_media_test,
    media_characteristics_get_test,
    storage_endpoints_get_test,
    system_host_change_function_test,
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    direct_calls_test,
    first_sector_different_signatures_test,
#endif
    close_media_test,
    ux_host_class_storage_request_sense_test,
    transport_failures_cause_device_reset_test,
    entry_command_test,
    media_capacity_get_test,

#ifdef UX_TEST_RACE_CONDITION_TESTS_ON
    disconnect_during_storage_thread_unmounting_media_test,
#endif
};


    ux_test_turn_off_expedient(&original_expedient_value);

    /* Format the ram drive. */
    status = fx_media_format(&global_ram_disk, _fx_ram_driver, global_ram_disk_memory, global_ram_disk_working_buffer, global_sector_size, "RAM DISK", 2, UX_TEST_NUM_DIRECTORY_ENTRIES, 0, UX_RAM_DISK_SIZE/global_sector_size, global_sector_size, 4, 1, 1);
    if (status != FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Open the ram_disk.  */
    status = fx_media_open(&global_ram_disk, "RAM DISK", _fx_ram_driver, global_ram_disk_memory, global_ram_disk_working_buffer, global_sector_size);
    if (status != FX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    UX_TEST_ASSERT(wait_for_enum_completion_and_get_global_storage_values() == UX_SUCCESS);

    /* Ensure slave activate callback was invoked. */
    if (global_slave_storage == UX_NULL)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Get basic memory amount. */
    disconnect_host_and_slave();
    global_memory_test_no_device_memory_free_amount = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    connect_host_and_slave();

    /* With basic memory amount, now do basic memory test. */
    disconnect_host_and_slave();
    connect_host_and_slave();

    for (i = 0; i < ARRAY_COUNT(tests); i++)
    {

        tests[i]();

        ux_test_free_user_list_actions();

        if (/* Left over actions? */
            ux_test_check_actions_empty() == UX_FALSE ||
            /* Invalid semaphore? */
            global_test_semaphore.tx_semaphore_count != 0 ||
            global_test_semaphore.tx_semaphore_suspended_count != 0)
        {

            /* Error! */
            printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
            test_control_return(1);
        }

        /* Reset for next test. */
        reset_to_bo();
    }

    /* The storage thread runs every two seconds. Make sure it runs at least once so the device reset will occurr. */
    ux_utility_delay_ms(UX_TEST_SLEEP_STORAGE_THREAD_RUN_ONCE);

    /* Disconnect device and host. */
    stepinfo("Disconnect test\n");

    disconnect_host_and_slave();

    /* Let enum thread run. */
    tx_thread_sleep(50);

    /* Ensure system change callback was invoked. */
    if (global_storage_change_function != UX_NULL)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Ensure slave deactivate callback was invoked. */
    if (global_slave_storage != UX_NULL)
    {

        printf("Error on line %d, error code: 0x%lx\n", __LINE__, (ULONG)status);
        test_control_return(1);
    }

    /* Start cleaning up. */
    stepinfo("clean up\n");

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_storage_name, ux_device_class_storage_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    ux_test_set_expedient(original_expedient_value);

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}


static UINT default_device_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{

UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_STATUS_PARAMS  params = { storage, lun, media_id, media_status };
UX_TEST_ACTION                      action;


    action = ux_test_action_handler(UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_STATUS, &params);
    if (action.matched)
    {
        if (action.action_func)
        {
            action.action_func(&action, &params);
        }

        if (!action.no_return)
        {
            return action.status;
        }
    }
    else
    {
        *media_status = 0;
    }

    /* The ATA drive never fails. This is just for demo only !!!! */
    return(UX_SUCCESS);
}

static UINT default_device_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT                                    status;
UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS  params = { storage, lun, data_pointer, number_blocks, lba, media_status };
UX_TEST_ACTION                          action;


    action = ux_test_action_handler(UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ, &params);
    if (action.matched && !action.do_after)
    {
        if (action.action_func)
        {
            action.action_func(&action, &params);
        }

        if (!action.no_return)
        {
            return action.status;
        }
    }

    if (lba == 0)
    {

        global_ram_disk.fx_media_driver_logical_sector = 0;
        global_ram_disk.fx_media_driver_sectors = 1;
        global_ram_disk.fx_media_driver_request = FX_DRIVER_BOOT_READ;
        global_ram_disk.fx_media_driver_buffer = data_pointer;
        _fx_ram_driver(&global_ram_disk);
        *(data_pointer) = 0xeb;
        *(data_pointer+1) = 0x3c;
        *(data_pointer+2) = 0x90;
        *(data_pointer+21) = 0xF8;

        *(data_pointer+24) = 0x01;
        *(data_pointer+26) = 0x10;
        *(data_pointer+28) = 0x01;

        *(data_pointer+510) = 0x55;
        *(data_pointer+511) = 0xaa;
        ux_utility_memory_copy(data_pointer+0x36, "FAT12", 5);


        status = global_ram_disk.fx_media_driver_status;
        UX_TEST_CHECK_SUCCESS(status);
    }
    else
    {

        while (number_blocks--)
        {
            status = fx_media_read(&global_ram_disk, lba, data_pointer);
            UX_TEST_CHECK_SUCCESS(status);
            data_pointer += global_sector_size;
            lba++;
        }
    }

    if (action.matched && action.do_after)
    {
        if (action.action_func)
        {
            action.action_func(&action, &params);
        }

        if (!action.no_return)
        {
            return action.status;
        }
    }

    return(status);
}

static UINT default_device_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT                                                status;
UX_TEST_ACTION                                      action;
UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS  params = { storage, lun, data_pointer, number_blocks, lba, media_status };


    action = ux_test_action_handler(UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_WRITE, &params);
    if (action.matched && !action.do_after)
    {
        if (action.action_func)
        {
            action.action_func(&action, &params);
        }

        if (!action.no_return)
        {
            return action.status;
        }
    }

    if(lba == 0)
    {

        global_ram_disk.fx_media_driver_logical_sector =  0;
        global_ram_disk.fx_media_driver_sectors =  1;
        global_ram_disk.fx_media_driver_request =  FX_DRIVER_BOOT_WRITE;
        global_ram_disk.fx_media_driver_buffer =  data_pointer;
        _fx_ram_driver(&global_ram_disk);

        status = global_ram_disk.fx_media_driver_status;

    }
    else
    {

        while(number_blocks--)
        {

            status =  fx_media_write(&global_ram_disk,lba,data_pointer);
            data_pointer+=global_sector_size;
            lba++;
        }
        return(status);
    }

    if (action.matched && action.do_after)
    {
        if (action.action_func)
        {
            action.action_func(&action, &params);
        }

        if (!action.no_return)
        {
            return action.status;
        }
    }

    return(status);
}


static UINT default_device_media_flush(VOID *storage, ULONG lun, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS    params = { storage, lun, UX_NULL, number_blocks, lba, media_status };


    ux_test_action_handler(UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_FLUSH, &params);

    return(UX_SUCCESS);
}
