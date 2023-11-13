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
static UINT        demo_thread_media_flush(VOID *storage, ULONG lun, ULONG number_blocks, ULONG lba, ULONG *media_status);

/* Define global data structures.  */

static UCHAR                        usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UCHAR                        buffer[UX_DEMO_BUFFER_SIZE];
static UCHAR                        stack_buffer[UX_HOST_CLASS_STORAGE_THREAD_STACK_SIZE];
static UCHAR                        cbw_buffer[UX_SLAVE_CLASS_STORAGE_CBW_LENGTH];
static UCHAR                        csw_buffer[UX_SLAVE_CLASS_STORAGE_CSW_LENGTH];

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

static UINT                         ram_disk_read_status = UX_SUCCESS;
static ULONG                        ram_disk_read_media_status = 0;
static CHAR                         ram_disk_read_sent = 0;

static UINT                         ram_disk_write_status = UX_SUCCESS;
static ULONG                        ram_disk_write_media_status = 0;
static CHAR                         ram_disk_write_sent = 0;

static CHAR                         ram_disk_flush = 0;
static UINT                         ram_disk_flush_status = UX_SUCCESS;

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

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,

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

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x00, 0x01, 0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x00, 0x01, 0x00,

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

static UX_TEST_SETUP _GetMaxLun = UX_TEST_SETUP_STORAGE_GetMaxLun;

static UX_TEST_HCD_SIM_ACTION stall_GetMaxLun[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_GetMaxLun,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ | UX_TEST_SIM_REQ_ANSWER, 0, buffer, 1, UX_TRANSFER_STALLED,
        UX_TRANSFER_STALLED, UX_NULL},
{   0   }
};

static UX_TEST_HCD_SIM_ACTION length_error_GetMaxLun[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_GetMaxLun,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ | UX_TEST_SIM_REQ_ANSWER, 0, buffer, 2, UX_SUCCESS,
        UX_SUCCESS, UX_NULL},
{   0   }
};

static UX_TEST_HCD_SIM_ACTION replaced_GetMaxLun[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_GetMaxLun,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ | UX_TEST_SIM_REQ_ANSWER, 0, buffer, 1, UX_SUCCESS,
        UX_SUCCESS, UX_NULL},
{   0   }
};

static UX_TEST_HCD_SIM_ACTION fail_bulk_out[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_MATCH_EP, 0x02, UX_NULL, 0, UX_ERROR,
        UX_ERROR, UX_NULL},
{   0   }
};

static UX_TEST_HCD_SIM_ACTION fail_2nd_bulk_out[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_MATCH_EP, 0x02, UX_NULL, 0, UX_SUCCESS,
        UX_SUCCESS, UX_NULL,
        UX_TRUE},
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_MATCH_EP, 0x02, UX_NULL, 0, UX_ERROR,
        UX_ERROR, UX_NULL},
{   0   }
};

static VOID sending_cbw(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{

UCHAR       *cbw = (UCHAR *)storage -> ux_host_class_storage_cbw;
UX_TRANSFER *transfer_request =  &storage -> ux_host_class_storage_bulk_out_endpoint -> ux_endpoint_transfer_request;


    _ux_utility_memory_copy(cbw_buffer, cbw, 31);

    /* Prepare CSW.  */
    _ux_utility_memory_copy(csw_buffer +  0, cbw_buffer + 0, 4); /* dCBWSignature -> dCSWSignature */
    _ux_utility_memory_copy(csw_buffer +  4, cbw_buffer + 4, 4); /* dCBWTag -> dCSWTag */
    _ux_utility_memory_set (csw_buffer +  8, 0x00, 4);           /* dCSWDataResidue <= 0 */
    _ux_utility_memory_set (csw_buffer + 12, 0x00, 1);           /* bCSWStatus <= 0 */

    _ux_utility_semaphore_put(&transfer_request -> ux_transfer_request_semaphore);
}

static VOID getting_data(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{

UX_TRANSFER *transfer_request =  &storage -> ux_host_class_storage_bulk_in_endpoint -> ux_endpoint_transfer_request;


    _ux_utility_long_put(csw_buffer + 8, 1); /* dCSWDataResidue <= 1 for error test */
    _ux_utility_semaphore_put(&transfer_request -> ux_transfer_request_semaphore);
}

static VOID getting_csw(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{

UCHAR       *csw = (UCHAR *)storage -> ux_host_class_storage_csw;
UX_TRANSFER *transfer_request =  &storage -> ux_host_class_storage_bulk_in_endpoint -> ux_endpoint_transfer_request;


    _ux_utility_memory_copy(csw, csw_buffer, 13);
    _ux_utility_semaphore_put(&transfer_request -> ux_transfer_request_semaphore);
}

static UX_TEST_HCD_SIM_ACTION replace_cbw_data_csw[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_MATCH_EP, 0x02, UX_NULL, 31, UX_SUCCESS,
        UX_SUCCESS,
        .action_func = sending_cbw,
        .no_return = UX_FALSE,
        .do_after = UX_FALSE},
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_MATCH_EP | UX_TEST_SIM_REQ_ANSWER, 0x81, buffer, 63, UX_SUCCESS,
        UX_SUCCESS,
        .action_func = getting_data,
        .no_return = UX_FALSE,
        .do_after = UX_FALSE},
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_MATCH_EP | UX_TEST_SIM_REQ_ANSWER, 0x81, csw_buffer, 13, UX_SUCCESS,
        UX_SUCCESS,
        .action_func = getting_csw,
        .no_return = UX_FALSE,
        .do_after = UX_FALSE},
{   0   }
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
void    usbx_ux_host_class_storage_entry_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Inform user.  */
    printf("Running ux_host_class_storage_entry Test............................ ");
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
    global_storage_parameter.ux_slave_class_storage_parameter_number_lun = 2;

    /* Initialize the storage class parameters for reading/writing to the first Flash Disk.  */
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_last_lba        =  UX_RAM_DISK_LAST_LBA;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_block_length    =  512;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_type            =  0;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_removable_flag  =  0x80;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read            =  demo_thread_media_read;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write           =  demo_thread_media_write;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status          =  demo_thread_media_status;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_flush           =  demo_thread_media_flush;

    /* Initialize the storage class parameters for reading/writing to the second Flash Disk.  */
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_last_lba        =  UX_RAM_DISK_LAST_LBA;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_block_length    =  512;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_type            =  0;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_removable_flag  =  0x80;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_read            =  demo_thread_media_read;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_write           =  demo_thread_media_write;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_status          =  demo_thread_media_status;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_flush           =  demo_thread_media_flush;

    /* Initialize the device storage class. The class is connected with interface 0 on configuration 1. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry,
                                                1, 0, (VOID *)&global_storage_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    // status =  _ux_test_dcd_sim_slave_initialize();
    status =  ux_dcd_sim_slave_initialize();

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
    // status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

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


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                        status;
UX_HOST_CLASS                               *class;
UX_HOST_CLASS_STORAGE_EXT                   *storage_ext0, *storage_ext1;
UX_HOST_CLASS_STORAGE_MEDIA                 *storage_media;
UX_ENDPOINT                                 *control_endpoint;
UX_TRANSFER                                 *transfer_request;
UX_DEVICE                                   *device;
UX_CONFIGURATION                            *configuration;
UX_INTERFACE                                *interface;
ULONG                                       rfree, cfree;
FX_MEDIA                                    *media;
UX_HOST_CLASS_COMMAND                       command;


    /* Find the storage class. */
    status =  host_storage_instance_get(100);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
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
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    media = &storage_media->ux_host_class_storage_media;
#endif

    /* Confirm media enum done ().  */
    status = storage_media_status_wait(storage_media, UX_HOST_CLASS_STORAGE_MEDIA_MOUNTED, 100);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Pause the class driver thread.  */
    _ux_utility_thread_suspend(&((UX_HOST_CLASS_STORAGE_EXT*)class->ux_host_class_ext)->ux_host_class_thread);

    configuration = device -> ux_device_first_configuration;
    interface = configuration -> ux_configuration_first_interface;

    /* Disconnect.  */
    // Seems there is memory problem if do resource free here???!!!
    // _ux_utility_thread_delete(&((UX_HOST_CLASS_STORAGE_EXT*)class->ux_host_class_ext)->ux_host_class_thread);
    // _ux_utility_memory_free(class->ux_host_class_media);
    // _ux_utility_memory_free(class->ux_host_class_ext);
    class->ux_host_class_media = UX_NULL;
    class->ux_host_class_ext = UX_NULL;

    /* Command.  */
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_ACTIVATE;
    command.ux_host_class_command_class_ptr = class;
    command.ux_host_class_command_container = interface;

    rfree = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    cfree = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available;

    stepinfo(">>>>>>>>>>>>>>> ux_host_class_storage_entry - UX_HOST_CLASS_COMMAND_ACTIVATE ext memory ERROR\n");
    ux_test_utility_sim_mem_alloc_fail_all_start();
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_ACTIVATE;
    status = _ux_host_class_storage_entry(&command);
    ux_test_utility_sim_mem_alloc_fail_all_stop();
    if (status != UX_MEMORY_INSUFFICIENT)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_DESTROY;
    _ux_host_class_storage_entry(&command);

    stepinfo(">>>>>>>>>>>>>>> ux_host_class_storage_entry - UX_HOST_CLASS_COMMAND_ACTIVATE media memory ERROR\n");
    ux_test_utility_sim_mem_allocate_until(sizeof(UX_HOST_CLASS_STORAGE_EXT) + sizeof(UX_HOST_CLASS_STORAGE_MEDIA) / 2);
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_ACTIVATE;
    status = _ux_host_class_storage_entry(&command);
    ux_test_utility_sim_mem_free_all();
    if (status != UX_MEMORY_INSUFFICIENT)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_DESTROY;
    _ux_host_class_storage_entry(&command);

#if 0 /* No need since stack is allocated with ext.  */
    stepinfo(">>>>>>>>>>>>>>> ux_host_class_storage_entry - UX_HOST_CLASS_COMMAND_ACTIVATE stack memory ERROR\n");
    ux_test_utility_sim_mem_allocate_until(UX_HOST_CLASS_STORAGE_MAX_MEDIA*sizeof(UX_HOST_CLASS_STORAGE_MEDIA));
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_ACTIVATE;
    status = _ux_host_class_storage_entry(&command);
    ux_test_utility_sim_mem_free_all();
    if (status != UX_MEMORY_INSUFFICIENT)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_DESTROY;
    _ux_host_class_storage_entry(&command);
#endif

    stepinfo(">>>>>>>>>>>>>>> ux_host_class_storage_entry - UX_HOST_CLASS_COMMAND_ACTIVATE thread ERROR\n");
    storage_ext0=_ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HOST_CLASS_STORAGE_EXT) + 128);
    storage_ext1=_ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HOST_CLASS_STORAGE_EXT));
    _ux_utility_thread_create(&storage_ext1->ux_host_class_thread,
        "tmp", _ux_host_class_storage_thread_entry, (ULONG)(ALIGN_TYPE)class,
        storage_ext0->ux_host_class_thread_stack, UX_HOST_CLASS_STORAGE_THREAD_STACK_SIZE,
        UX_HOST_CLASS_STORAGE_THREAD_PRIORITY_CLASS, UX_HOST_CLASS_STORAGE_THREAD_PRIORITY_CLASS,
        TX_NO_TIME_SLICE, TX_DONT_START);
    UX_THREAD_EXTENSION_PTR_SET(&storage_ext1->ux_host_class_thread, class)
    _ux_utility_memory_free(storage_ext0);
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_ACTIVATE;
    status = _ux_host_class_storage_entry(&command);
    if (status != UX_THREAD_ERROR)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_utility_thread_delete(&storage_ext1->ux_host_class_thread);
    _ux_utility_memory_free(storage_ext1);
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_DESTROY;
    _ux_host_class_storage_entry(&command);

    /* Wait a while so the thread goes.  */
    _ux_utility_delay_ms(10);

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

    if (lun > 1)
        return UX_ERROR;

    ram_disk_read_sent = UX_TRUE;
    if (ram_disk_read_status != UX_SUCCESS)
    {
        if (media_status != UX_NULL)
            *media_status = ram_disk_read_media_status;

        return ram_disk_read_status;
    }

    ux_utility_memory_copy(data_pointer, &ram_disk_memory[lun][lba * 512], number_blocks * 512);

    return UX_SUCCESS;
}

static UINT    demo_thread_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
    (void)storage;

    if (lun > 1)
        return UX_ERROR;

    ram_disk_write_sent = UX_TRUE;
    if (ram_disk_write_status != UX_SUCCESS)
    {
        if (media_status != UX_NULL)
            *media_status = ram_disk_write_media_status;

        return ram_disk_write_status;
    }

    ux_utility_memory_copy(&ram_disk_memory[lun][lba * 512], data_pointer, number_blocks * 512);

    return UX_SUCCESS;
}

static UINT demo_thread_media_flush(VOID *storage, ULONG lun, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
    (void)storage;
    (void)number_blocks;
    (void)lba;
    (void)media_status;

    if (lun > 1)
        return UX_ERROR;

    ram_disk_flush = UX_TRUE;
    return ram_disk_flush_status;
}