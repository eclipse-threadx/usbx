/* This test is designed to test the simple dpump host/device class operation.  */

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

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

/* Define constants.  */
#define                             UX_DEMO_STACK_SIZE              2048
#define                             UX_DEMO_MEMORY_SIZE             (256*1024)
#define                             UX_DEMO_BUFFER_SIZE             2048

#define                             UX_RAM_DISK_SIZE                (200 * 1024)
#define                             UX_RAM_DISK_LAST_LBA            ((UX_RAM_DISK_SIZE / 512) -1)

/* Define local/extern function prototypes.  */

VOID _fx_ram_driver(FX_MEDIA *media_ptr);
void _fx_ram_drive_format(ULONG disk_size, UINT sector_size, UINT sectors_per_cluster,
                                                UINT fat_entries, UINT root_directory_entries);

static void        demo_thread_entry(ULONG);
static TX_THREAD   tx_demo_thread_host_simulation;
static TX_THREAD   tx_demo_thread_slave_simulation;
static void        tx_demo_thread_host_simulation_entry(ULONG);

static UINT        demo_thread_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT        demo_thread_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT        demo_thread_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status);

/* Define global data structures.  */

static UCHAR                        usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

static ULONG                        error_counter;

static TX_THREAD                    demo_thread;

static UX_HOST_CLASS_STORAGE                *storage;
static UX_SLAVE_CLASS_STORAGE_PARAMETER     global_storage_parameter;

static FX_MEDIA                     ram_disk_media1;
static FX_MEDIA                     ram_disk_media2;
static CHAR                         ram_disk_buffer1[512];
static CHAR                         ram_disk_buffer2[512];
static CHAR                         ram_disk_memory1[UX_RAM_DISK_SIZE];
static CHAR                         ram_disk_memory2[UX_RAM_DISK_SIZE];
static CHAR                         *ram_disk_memory[] =
{
    ram_disk_memory1, ram_disk_memory2
};
static UINT                         ram_disk_status = UX_SUCCESS;
static ULONG                        ram_disk_media_status = 0;
static CHAR                         ram_disk_status_sent = 0;

static ULONG                               set_cfg_counter;

static ULONG                               rsc_mem_alloc_cnt_on_set_cfg;
static ULONG                               rsc_sem_on_set_cfg;
static ULONG                               rsc_sem_get_on_set_cfg;
static ULONG                               rsc_mutex_on_set_cfg;

static ULONG                               rsc_enum_sem_usage;
static ULONG                               rsc_enum_sem_get_count;
static ULONG                               rsc_enum_mutex_usage;
static ULONG                               rsc_enum_mem_alloc_count;

static ULONG                               rsc_storage_sem_usage;
static ULONG                               rsc_storage_sem_get_count;
static ULONG                               rsc_storage_mutex_usage;
static ULONG                               rsc_storage_mem_alloc_count;

static ULONG                               interaction_count;

static UCHAR                               error_callback_ignore = UX_TRUE;
static ULONG                               error_callback_counter;

#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED 50
static UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
        0x81, 0x07, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Configuration descriptor */
        0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00

    };


#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED 60
static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x81, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Device qualifier descriptor */
        0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x01, 0x00,

    /* Configuration descriptor */
        0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x00, 0x01, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x00, 0x01, 0x00

    };


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


static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        {
            /* Failed test.  */
            printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
            test_control_return(1);
        }
    }
}

static UINT host_storage_instance_get(ULONG timeout_x10ms)
{

UINT                status;
UX_HOST_CLASS       *class;


    /* Find the main storage container */
    status =  ux_host_stack_class_get(_ux_system_host_class_storage_name, &class);
    if (status != UX_SUCCESS)
        return(status);

    /* Get storage instance, wait it to be live and media attached.  */
    do
    {
        if (timeout_x10ms)
        {
            ux_utility_delay_ms(10);
            if (timeout_x10ms != 0xFFFFFFFF)
                timeout_x10ms --;
        }

        status =  ux_host_stack_class_instance_get(class, 0, (void **) &storage);
        if (status == UX_SUCCESS)
        {
            if (storage -> ux_host_class_storage_state == UX_HOST_CLASS_INSTANCE_LIVE &&
                class -> ux_host_class_media != UX_NULL)
                return(UX_SUCCESS);
        }

    } while(timeout_x10ms > 0);

    return(UX_ERROR);
}

static UINT  sleep_break_on_error(VOID)
{

    if (error_callback_counter >= 3)
        return error_callback_counter;

    return UX_SUCCESS;
}


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_class_storage_format_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Inform user.  */
    printf("Running ux_device_class_storage_format Test......................... ");
    stepinfo("\n");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* Reset ram disks memory.  */
    ux_utility_memory_set(ram_disk_memory1, 0, UX_RAM_DISK_SIZE);
    ux_utility_memory_set(ram_disk_memory2, 0, UX_RAM_DISK_SIZE);

    /* Initialize FileX.  */
    fx_system_initialize();

    /* Change the ram drive values. */
    fx_media_format(&ram_disk_media1, _fx_ram_driver, ram_disk_memory1, ram_disk_buffer1, 512, "RAM DISK1", 2, 512, 0, UX_RAM_DISK_SIZE/512, 512, 4, 1, 1);
    fx_media_format(&ram_disk_media2, _fx_ram_driver, ram_disk_memory2, ram_disk_buffer2, 512, "RAM DISK2", 2, 512, 0, UX_RAM_DISK_SIZE/512, 512, 4, 1, 1);

    /* The code below is required for installing the device portion of USBX.
       In this demo, DFU is possible and we have a call back for state change. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Store the number of LUN in this device storage instance.  */
    global_storage_parameter.ux_slave_class_storage_parameter_number_lun = 1;

    /* Initialize the storage class parameters for reading/writing to the first Flash Disk.  */
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_last_lba        =  UX_RAM_DISK_LAST_LBA;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_block_length    =  512;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_type            =  0;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_removable_flag  =  0x80;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read            =  demo_thread_media_read;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write           =  demo_thread_media_write;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status          =  demo_thread_media_status;

    /* Initialize the storage class parameters for reading/writing to the second Flash Disk.  */
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_last_lba        =  UX_RAM_DISK_LAST_LBA;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_block_length    =  512;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_type            =  0;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_removable_flag  =  0x80;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_read            =  demo_thread_media_read;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_write           =  demo_thread_media_write;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_status          =  demo_thread_media_status;

    /* Initialize the device storage class. The class is connected with interface 0 on configuration 1. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry,
                                                1, 0, (VOID *)&global_storage_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register storage class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_storage_name, ux_host_class_storage_entry);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

}

static UINT storage_media_status_wait(UX_HOST_CLASS_STORAGE_MEDIA *storage_media, ULONG status, ULONG timeout)
{

    while(1)
    {
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
        if (storage_media->ux_host_class_storage_media_status == status)
            return UX_SUCCESS;
#else
        if ((status == UX_HOST_CLASS_STORAGE_MEDIA_MOUNTED &&
            storage_media->ux_host_class_storage_media_storage != UX_NULL) ||
            (status == UX_HOST_CLASS_STORAGE_MEDIA_UNMOUNTED &&
            storage_media->ux_host_class_storage_media_storage == UX_NULL))
            return(UX_SUCCESS);
#endif
        if (timeout == 0)
            break;
        if (timeout != 0xFFFFFFFF)
            timeout --;
        _ux_utility_delay_ms(10);
    }
    return UX_ERROR;
}

static void  _test_init_cbw_FORMAT_UNIT(void)
{

UCHAR           *cbw;
UINT            command_length;


    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;
    command_length =  UX_HOST_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_SBC;
    _ux_host_class_storage_cbw_initialize(storage, 0, 0, command_length);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0) =  UX_SLAVE_CLASS_STORAGE_SCSI_FORMAT;
}

static UINT _test_send_cbw(void)
{

UX_TRANSFER     *transfer_request;
UINT            status;
UCHAR           *cbw;


    transfer_request =  &storage -> ux_host_class_storage_bulk_out_endpoint -> ux_endpoint_transfer_request;
    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;

    transfer_request -> ux_transfer_request_data_pointer =      cbw;
    transfer_request -> ux_transfer_request_requested_length =  UX_HOST_CLASS_STORAGE_CBW_LENGTH;
    status =  ux_host_stack_transfer_request(transfer_request);

    /* There is error, return the error code.  */
    if (status != UX_SUCCESS)
        return(status);

    /* Wait transfer done.  */
    status =  _ux_utility_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, MS_TO_TICK(UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT));

    /* No error, it's done.  */
    if (status == UX_SUCCESS)
        return(transfer_request->ux_transfer_request_completion_code);

    /* All transfers pending need to abort. There may have been a partial transfer.  */
    ux_host_stack_transfer_request_abort(transfer_request);

    /* Set the completion code.  */
    transfer_request -> ux_transfer_request_completion_code =  UX_TRANSFER_TIMEOUT;

    /* There was an error, return to the caller.  */
    return(UX_TRANSFER_TIMEOUT);
}

static UINT _test_transfer_data(UCHAR *data, ULONG size, UCHAR do_read)
{

UX_TRANSFER     *transfer_request;
UINT            status;


    transfer_request =  do_read ?
            &storage -> ux_host_class_storage_bulk_in_endpoint -> ux_endpoint_transfer_request :
            &storage -> ux_host_class_storage_bulk_out_endpoint -> ux_endpoint_transfer_request;
    transfer_request -> ux_transfer_request_data_pointer = data;
    transfer_request -> ux_transfer_request_requested_length =  size;

    status =  ux_host_stack_transfer_request(transfer_request);

    /* There is error, return the error code.  */
    if (status != UX_SUCCESS)
        return(status);

    /* Wait transfer done.  */
    status =  _ux_utility_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, MS_TO_TICK(UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT));

    /* No error, it's done.  */
    if (status == UX_SUCCESS)
        return(transfer_request->ux_transfer_request_completion_code);

    /* All transfers pending need to abort. There may have been a partial transfer.  */
    ux_host_stack_transfer_request_abort(transfer_request);

    /* Set the completion code.  */
    transfer_request -> ux_transfer_request_completion_code =  UX_TRANSFER_TIMEOUT;

    /* There was an error, return to the caller.  */
    return(UX_TRANSFER_TIMEOUT);
}

static UINT _test_wait_csw(void)
{

UX_TRANSFER     *transfer_request;
UINT            status;


    /* Get the pointer to the transfer request, on the bulk in endpoint.  */
    transfer_request =  &storage -> ux_host_class_storage_bulk_in_endpoint -> ux_endpoint_transfer_request;

    /* Fill in the transfer_request parameters.  */
    transfer_request -> ux_transfer_request_data_pointer =      (UCHAR *) &storage -> ux_host_class_storage_csw;
    transfer_request -> ux_transfer_request_requested_length =  UX_HOST_CLASS_STORAGE_CSW_LENGTH;

    /* Get the CSW on the bulk in endpoint.  */
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
        return(status);

    /* Wait for the completion of the transfer request.  */
    status =  _ux_utility_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, MS_TO_TICK(UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT));

    /* If OK, we are done.  */
    if (status == UX_SUCCESS)
        return(transfer_request->ux_transfer_request_completion_code);

    /* All transfers pending need to abort. There may have been a partial transfer.  */
    ux_host_stack_transfer_request_abort(transfer_request);

    /* Set the completion code.  */
    transfer_request -> ux_transfer_request_completion_code =  UX_TRANSFER_TIMEOUT;

    /* There was an error, return to the caller.  */
    return(UX_TRANSFER_TIMEOUT);
}

static VOID _test_clear_stall(UCHAR clear_read_stall)
{

UX_ENDPOINT     *endpoint;


    endpoint =  clear_read_stall ?
            storage -> ux_host_class_storage_bulk_in_endpoint :
            storage -> ux_host_class_storage_bulk_out_endpoint;
    _ux_host_stack_endpoint_reset(endpoint);
}

static UINT _test_request_sense(void)
{

UINT            status;
UX_TRANSFER     *transfer_request;
UCHAR           *cbw;
UCHAR           *request_sense_response;
ULONG           sense_code;
UINT            command_length = UX_HOST_CLASS_STORAGE_REQUEST_SENSE_COMMAND_LENGTH_SBC;


    transfer_request =  &storage -> ux_host_class_storage_bulk_out_endpoint -> ux_endpoint_transfer_request;
    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;

    _ux_utility_memory_copy(storage -> ux_host_class_storage_saved_cbw, storage -> ux_host_class_storage_cbw, UX_HOST_CLASS_STORAGE_CBW_LENGTH);
    _ux_host_class_storage_cbw_initialize(storage, UX_HOST_CLASS_STORAGE_DATA_IN, UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH, command_length);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_REQUEST_SENSE_OPERATION) =  UX_HOST_CLASS_STORAGE_SCSI_REQUEST_SENSE;
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_REQUEST_SENSE_ALLOCATION_LENGTH) =  UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH;
    request_sense_response =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH);
    if (request_sense_response == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);
    status = _test_send_cbw();
    if (status == UX_SUCCESS)
    {
        status = _test_transfer_data(request_sense_response, UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH, UX_TRUE);
        if (status == UX_SUCCESS)
        {
            status = _test_wait_csw();
            if (status == UX_SUCCESS)
            {

                sense_code =  (((ULONG) *(request_sense_response + UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_SENSE_KEY)) & 0x0f) << 16;
                sense_code |=  ((ULONG) *(request_sense_response + UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_CODE)) << 8;
                sense_code |=  (ULONG)  *(request_sense_response + UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_CODE_QUALIFIER);

                storage -> ux_host_class_storage_sense_code =  sense_code;
            }
        }
    }
    _ux_utility_memory_free(request_sense_response);
    _ux_utility_memory_copy(storage -> ux_host_class_storage_cbw, storage -> ux_host_class_storage_saved_cbw, UX_HOST_CLASS_STORAGE_CBW_LENGTH);
    return(status);
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                        status;
UX_HOST_CLASS                               *class;
UX_HOST_CLASS_STORAGE_MEDIA                 *storage_media;
UX_ENDPOINT                                 *control_endpoint;
UX_TRANSFER                                 *transfer_request;
UX_DEVICE                                   *device;


    /* Find the storage class. */
    status =  host_storage_instance_get(100);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    status = ux_host_stack_device_get(0, &device);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: device_get fail\n", __LINE__);
        test_control_return(1);
    }
    control_endpoint = &device->ux_device_control_endpoint;
    transfer_request = &control_endpoint->ux_endpoint_transfer_request;

    /* Wait enough time for media mounting.  */
    _ux_utility_delay_ms(UX_HOST_CLASS_STORAGE_DEVICE_INIT_DELAY);

    class = storage->ux_host_class_storage_class;
    storage_media = (UX_HOST_CLASS_STORAGE_MEDIA *)class->ux_host_class_media;

    /* Confirm media enum done.  */
    status = storage_media_status_wait(storage_media, UX_HOST_CLASS_STORAGE_MEDIA_MOUNTED, 100);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Pause the class driver thread.  */
    _ux_utility_thread_suspend(&((UX_HOST_CLASS_STORAGE_EXT*)class->ux_host_class_ext)->ux_host_class_thread);

    _test_init_cbw_FORMAT_UNIT();
    status = _test_send_cbw();
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data("test dead beef", 15, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_FALSE);
    status = _test_wait_csw();
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_request_sense();
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_storage_name, ux_device_class_storage_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}


static UINT    demo_thread_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{

UINT status = ram_disk_status;


    (void)storage;
    (void)media_id;

    if (media_status)
        *media_status = ram_disk_media_status;

    /* If there is attention, it must be changed to ready after reported.  */
    if (ram_disk_media_status == (UX_SLAVE_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION | (0x28 << 8)))
    {
        ram_disk_status = UX_SUCCESS;
        ram_disk_media_status = 0;
    }
    else
        ram_disk_status_sent = UX_TRUE;

    return status;
}

static UINT    demo_thread_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
    (void)storage;
    (void)media_status;

    if (lun > 1)
        return UX_ERROR;

    ux_utility_memory_copy(data_pointer, &ram_disk_memory[lun][lba * 512], number_blocks * 512);

    return UX_SUCCESS;
}

static UINT    demo_thread_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
    (void)storage;
    (void)media_status;

    if (lun > 1)
        return UX_ERROR;

    ux_utility_memory_copy(&ram_disk_memory[lun][lba * 512], data_pointer, number_blocks * 512);

    return UX_SUCCESS;
}
