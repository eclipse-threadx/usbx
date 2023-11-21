/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "fx_api.h"

#include "ux_device_stack.h"
#include "ux_device_class_storage.h"

#include "ux_host_stack.h"
#include "ux_host_class_storage.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

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
#define                             UX_DEMO_STACK_SIZE              2048
#define                             UX_DEMO_MEMORY_SIZE             (256*1024)
#define                             UX_DEMO_BUFFER_SIZE             (UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE * 3)

#define                             UX_RAM_DISK_SIZE                (200 * 1024)
#define                             UX_RAM_DISK_LAST_LBA            ((UX_RAM_DISK_SIZE / 512) -1)

/* Define local/extern function prototypes.  */
VOID               _fx_ram_driver(FX_MEDIA *media_ptr);

static void        demo_thread_entry(ULONG);
static TX_THREAD   tx_demo_thread_host_simulation;
static TX_THREAD   tx_demo_thread_device_simulation;
static void        tx_demo_thread_host_simulation_entry(ULONG);
static void        tx_demo_thread_device_simulation_entry(ULONG);

static UINT        ux_test_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst);

static UINT        demo_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT        demo_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT        demo_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status);
static UINT        demo_media_flush(VOID *storage, ULONG lun, ULONG number_blocks, ULONG lba, ULONG *media_status);

static VOID        ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);

/* Define global data structures.  */

static UCHAR                            usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UCHAR                            buffer[UX_DEMO_BUFFER_SIZE];

static ULONG                            error_counter;

static UX_HOST_CLASS_STORAGE            *storage;
static UX_HOST_CLASS_STORAGE_MEDIA      *storage_media;
static FX_MEDIA                         *media;
static UX_SLAVE_CLASS_STORAGE_PARAMETER global_storage_parameter;

static FX_MEDIA                         ram_disk1;
static FX_MEDIA                         ram_disk2;
static CHAR                             ram_disk_memory1[UX_RAM_DISK_SIZE];
static CHAR                             ram_disk_memory2[UX_RAM_DISK_SIZE];
static UCHAR                            buffer1[512];
static UCHAR                            buffer2[512];

static FX_MEDIA                         *ram_disks[] = {&ram_disk1, &ram_disk2};
static UCHAR                            *buffers[] = {buffer1, buffer2};
static CHAR                             *ram_disk_memory[] = { ram_disk_memory1, ram_disk_memory2 };

static UINT                             ram_disk_status = UX_SUCCESS;
static ULONG                            ram_disk_media_status = 0;
static CHAR                             ram_disk_status_sent = 0;

static ULONG                            ram_disk_rw_fail_mode = 0; /* 1: BulkIN, 2: BulkOUT, 0: DISK. 0x80: once */
#define                                 ram_disk_rw_fail_mode_bulk_in()   ((ram_disk_rw_fail_mode & 0x7F) == 1)
#define                                 ram_disk_rw_fail_mode_bulk_out()  ((ram_disk_rw_fail_mode & 0x7F) == 2)
#define                                 ram_disk_rw_fail_mode_disk()      ((ram_disk_rw_fail_mode & 0x7F) == 0)
#define                                 ram_disk_rw_fail_mode_one_shot()  ((ram_disk_rw_fail_mode & 0x80) >  0)
static ULONG                            ram_disk_rw_fail_after = 0xFFFFFFFFu;
static ULONG                            ram_disk_rw_count = 0;
static ULONG                            ram_disk_rw_wait_delay = 0;
static ULONG                            ram_disk_rw_wait_start = 0;
static UCHAR                            ram_disk_rw_wait_state = 0;/* 0: idle, 1: wait */

static CHAR                             ram_disk_flush = 0;
static UINT                             ram_disk_flush_status = UX_STATE_NEXT;

static ULONG                            set_cfg_counter;

static ULONG                            rsc_mem_alloc_cnt_on_set_cfg;
static ULONG                            rsc_sem_on_set_cfg;
static ULONG                            rsc_sem_get_on_set_cfg;
static ULONG                            rsc_mutex_on_set_cfg;

static ULONG                            rsc_enum_sem_usage;
static ULONG                            rsc_enum_sem_get_count;
static ULONG                            rsc_enum_mutex_usage;
static ULONG                            rsc_enum_mem_alloc_count;

static ULONG                            rsc_storage_sem_usage;
static ULONG                            rsc_storage_sem_get_count;
static ULONG                            rsc_storage_mutex_usage;
static ULONG                            rsc_storage_mem_alloc_count;

static ULONG                            interaction_count;

static UCHAR                            error_callback_ignore = UX_TRUE;
static ULONG                            error_callback_counter;

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
        0x07, 0x05, 0x81, 0x02, 0x00, 0x02, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x00, 0x02, 0x00

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


/* Setup requests */

static UX_TEST_SETUP _SetConfigure = UX_TEST_SETUP_SetConfigure;

static UX_TEST_HCD_SIM_ACTION log_on_SetCfg[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_SetConfigure,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_SUCCESS, ux_test_hcd_entry_set_cfg,
        UX_TRUE}, /* Invoke callback & continue */
{   0   }
};


static UX_TEST_HCD_SIM_ACTION fail_on_bulkin[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_DCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_MATCH_EP, 0x81, UX_NULL, 0, UX_ERROR,
        UX_STATE_ERROR, UX_NULL,
        UX_FALSE}, /* Invoke callback & no continue */
{   0   }
};


static UX_TEST_HCD_SIM_ACTION fail_on_bulkout[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_DCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_MATCH_EP, 0x02, UX_NULL, 0, UX_ERROR,
        UX_STATE_ERROR, UX_NULL,
        UX_FALSE}, /* Invoke callback & no continue */
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


#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    storage_media = UX_NULL;
    media = UX_NULL;
#endif

    /* Find the main storage container */
    status =  ux_host_stack_class_get(_ux_system_host_class_storage_name, &class);
    if (status != UX_SUCCESS)
        return(status);

    /* Get storage instance, wait it to be live and media attached.  */
    do
    {
        if (timeout_x10ms)
        {
            tx_thread_sleep(UX_MS_TO_TICK_NON_ZERO(10));
            if (timeout_x10ms != 0xFFFFFFFF)
                timeout_x10ms --;
        }

        status =  ux_host_stack_class_instance_get(class, 0, (void **) &storage);
        if (status == UX_SUCCESS)
        {
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
            storage_media = (UX_HOST_CLASS_STORAGE_MEDIA *) class -> ux_host_class_media;
            media = &storage_media -> ux_host_class_storage_media;
#endif
            if (storage -> ux_host_class_storage_state == UX_HOST_CLASS_INSTANCE_LIVE &&
                class -> ux_host_class_ext != UX_NULL &&
                class -> ux_host_class_media != UX_NULL)
            {
                return(UX_SUCCESS);
            }
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

static UINT sleep_break_on_connect(VOID)
{
    if (host_storage_instance_get(0) == UX_SUCCESS)
        return(1);
    if (error_callback_counter >= 3)
        return(1);
    return(0);
}

static VOID ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *_params)
{

    set_cfg_counter ++;

    rsc_mem_alloc_cnt_on_set_cfg = ux_test_utility_sim_mem_alloc_count();

    rsc_sem_on_set_cfg = ux_test_utility_sim_sem_create_count();
    rsc_sem_get_on_set_cfg = ux_test_utility_sim_sem_get_count();
    rsc_mutex_on_set_cfg = ux_test_utility_sim_mutex_create_count();
}


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_device_storage_basic_standalone_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
ULONG                           mem_free;
ULONG                           test_n;


    /* Inform user.  */
    printf("Running STANDALONE Device Storage Read Write Test................... ");
#ifndef UX_DEVICE_STANDALONE
    printf("Skip\n");
    test_control_return(0);
#endif

    stepinfo("\n");

    /* Initialize FileX.  */
    fx_system_initialize();

    /* Reset ram disks memory.  */
    ux_utility_memory_set(ram_disk_memory1, 0, UX_RAM_DISK_SIZE);
    ux_utility_memory_set(ram_disk_memory2, 0, UX_RAM_DISK_SIZE);

    /* Format the ram drive. */
    status =   fx_media_format(&ram_disk1, _fx_ram_driver, ram_disk_memory1, buffer1, 512, "RAM DISK1", 2, 512, 0, UX_RAM_DISK_SIZE/512, 512, 4, 1, 1);
    status |=  fx_media_format(&ram_disk2, _fx_ram_driver, ram_disk_memory2, buffer2, 512, "RAM DISK2", 2, 512, 0, UX_RAM_DISK_SIZE/512, 512, 4, 1, 1);
    if (status != FX_SUCCESS)
    {

        /* Storage basic test error.  */
        printf("ERROR #8\n");
        test_control_return(1);
    }

    /* Open the ram_disk.  */
    status =   fx_media_open(&ram_disk1, "RAM DISK1", _fx_ram_driver, ram_disk_memory1, buffer1, 512);
    status |=  fx_media_open(&ram_disk2, "RAM DISK2", _fx_ram_driver, ram_disk_memory2, buffer2, 512);
    if (status != FX_SUCCESS)
    {

        /* Storage basic test error.  */
        printf("ERROR %d\n", __LINE__);
        test_control_return(1);
    }

    /* Reset testing counts. */
    ux_test_utility_sim_mem_alloc_log_enable(UX_TRUE);
    ux_test_utility_sim_mem_alloc_count_reset();
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    /* Reset error generations */
    ux_test_utility_sim_sem_error_generation_stop();
    ux_test_utility_sim_mutex_error_generation_stop();
    ux_test_utility_sim_sem_get_error_generation_stop();

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
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read            =  demo_media_read;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write           =  demo_media_write;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status          =  demo_media_status;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_flush           =  demo_media_flush;

    /* Initialize the storage class parameters for reading/writing to the second Flash Disk.  */
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_last_lba        =  UX_RAM_DISK_LAST_LBA;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_block_length    =  512;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_type            =  0;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_removable_flag  =  0x80;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_read            =  demo_media_read;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_write           =  demo_media_write;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_status          =  demo_media_status;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_flush           =  demo_media_flush;

    /* Initialize the device storage class. The class is connected with interface 0 on configuration 1. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry,
                                                1, 0, (VOID *)&global_storage_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    // status =  _ux_dcd_sim_slave_initialize();
    status =  _ux_test_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(ux_test_system_host_change_function);
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

    /* Create the main device simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_device_simulation, "tx demo device simulation", tx_demo_thread_device_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            21, 21, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    stack_pointer += UX_DEMO_STACK_SIZE;

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

static void tx_demo_thread_device_simulation_entry(ULONG arg)
{
    while(1)
    {

        /* Run device tasks.  */
        ux_system_tasks_run();

        /* Relinquish to other thread.  */
        tx_thread_relinquish();
    }
}

static ULONG _test_dw_minus(ULONG d0, ULONG d1)
{
    if (d0 >= d1)
        return(d0 - d1);
    return(d0 + (0xFFFFFFFF - d1));
}

static UINT  _media_driver_read(FX_MEDIA *media,
    VOID (*_media_driver)(FX_MEDIA *),
    UCHAR *buffer, ULONG lba, ULONG n_lb)
{
UINT status;

    if (lba == 0)
    {
        media->fx_media_driver_logical_sector = lba;
        media->fx_media_driver_sectors = n_lb;
        media->fx_media_driver_request = FX_DRIVER_BOOT_READ;
        media->fx_media_driver_buffer = buffer;
        _media_driver(media);
        *(buffer) =  0xeb;
        *(buffer+1) =  0x3c;
        *(buffer+2) =  0x90;
        *(buffer+21) =  0xF8;

        *(buffer+24) =  0x01;
        *(buffer+26) =  0x10;
        *(buffer+28) =  0x01;

        *(buffer+510) =  0x55;
        *(buffer+511) =  0xaa;
        ux_utility_memory_copy(buffer+0x36,"FAT12",5);

        if (media->fx_media_driver_status != FX_SUCCESS)
        {
            printf("%s:%d: FX error 0x%x\n", __FILE__, __LINE__, media->fx_media_driver_status);
            return(UX_ERROR);
        }

        lba++;
        n_lb --;
        buffer += 512;
    }
    media->fx_media_driver_logical_sector = lba;
    media->fx_media_driver_sectors = n_lb;
    media->fx_media_driver_request = FX_DRIVER_READ;
    media->fx_media_driver_buffer = buffer;
    _media_driver(media);
    if (media->fx_media_driver_status != FX_SUCCESS)
    {
        stepinfo("%s:%d: FX error 0x%x\n", __FILE__, __LINE__, media->fx_media_driver_status);
        return(UX_ERROR);
    }
    return(UX_SUCCESS);
}

static UINT  _media_driver_write(FX_MEDIA *media,
    VOID (*_media_driver)(FX_MEDIA *),
    UCHAR *buffer, ULONG lba, ULONG n_lb)
{
UINT status;

    if (lba == 0)
    {
        media -> fx_media_driver_logical_sector = 0;
        media -> fx_media_driver_sectors = 1;
        media -> fx_media_driver_request = FX_DRIVER_BOOT_WRITE;
        media -> fx_media_driver_buffer = buffer;
        _media_driver(media);

        if (media->fx_media_driver_status != FX_SUCCESS)
        {
            printf("%s:%d: FX error 0x%x\n", __FILE__, __LINE__, media->fx_media_driver_status);
            return(UX_ERROR);
        }

        lba ++;
        n_lb --;
        buffer += 512;
    }
    if (n_lb)
    {
        media -> fx_media_driver_logical_sector = lba;
        media -> fx_media_driver_sectors = n_lb;
        media -> fx_media_driver_request = FX_DRIVER_WRITE;
        media -> fx_media_driver_buffer = buffer;
        _media_driver(media);

        if (media->fx_media_driver_status != FX_SUCCESS)
        {
            printf("%s:%d: FX error 0x%x\n", __FILE__, __LINE__, media->fx_media_driver_status);
            return(UX_ERROR);
        }
    }
    return(UX_SUCCESS);
}

static UINT _test_host_class_storage_inquiry(UX_HOST_CLASS_STORAGE *storage,
    UCHAR flags, ULONG data_length, ULONG cb_length,
    UCHAR page_code, UCHAR *response, ULONG response_length)
{
UINT            status;
UCHAR           *cbw;
UINT            command_length;

    /* Use a pointer for the cbw, easier to manipulate.  */
    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;

    /* Initialize the CBW for this command.  */
    _ux_host_class_storage_cbw_initialize(storage, flags, data_length, cb_length);

    /* Prepare the INQUIRY command block.  */
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_INQUIRY_OPERATION) = UX_HOST_CLASS_STORAGE_SCSI_INQUIRY;

    /* Store the page code.  */
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_INQUIRY_PAGE_CODE) = page_code;

    /* Store the length of the Inquiry Response.  */
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_INQUIRY_ALLOCATION_LENGTH) =  (UCHAR)response_length;

    /* Send the command to transport layer.  */
    status =  _ux_host_class_storage_transport(storage, response);

    /* Return completion status.  */
    return(status);
}
static UINT test_host_class_storage_inquiry(UX_HOST_CLASS_STORAGE *storage, UCHAR page_code, UCHAR *response, ULONG response_length)
{
    _test_host_class_storage_inquiry(storage,
        UX_HOST_CLASS_STORAGE_DATA_IN,
        UX_HOST_CLASS_STORAGE_INQUIRY_RESPONSE_LENGTH,
        UX_HOST_CLASS_STORAGE_INQUIRY_COMMAND_LENGTH_SBC,
        page_code, response, response_length);
}

static UINT _test_send_cbw_EX(int __line__, ULONG length)
{

UX_TRANSFER     *transfer_request;
UINT            status;
UCHAR           *cbw;

    stepinfo(">>>>> %d.%d: CBW\n", __LINE__, __line__);
    transfer_request =  &storage -> ux_host_class_storage_bulk_out_endpoint -> ux_endpoint_transfer_request;
    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;

    transfer_request -> ux_transfer_request_data_pointer =      cbw;
    transfer_request -> ux_transfer_request_requested_length =  length;
    status =  ux_host_stack_transfer_request(transfer_request);

    /* There is error, return the error code.  */
    if (status != UX_SUCCESS)
        return(status);

    /* Wait transfer done.  */
    status =  _ux_host_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, MS_TO_TICK(UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT));

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
static UINT _test_send_cbw(int __line__)
{
    _test_send_cbw_EX(__line__, UX_HOST_CLASS_STORAGE_CBW_LENGTH);
}

static UINT _test_transfer_data(int __line__, UCHAR *data, ULONG size, UCHAR do_read)
{

UX_TRANSFER     *transfer_request;
UINT            status;


    stepinfo(">>>>> %d.%d: DATA\n", __LINE__, __line__);
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
    status =  _ux_host_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, MS_TO_TICK(UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT));

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

static UINT _test_wait_csw_EX(int __line__)
{

UX_TRANSFER     *transfer_request;
UINT            status;


    stepinfo(">>>>> %d.%d: CSW_Ex\n", __LINE__, __line__);

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
    status =  _ux_host_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, MS_TO_TICK(UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT));

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

static UINT _test_wait_csw(int __line__)
{
UINT   status;

    stepinfo(">>>>> %d.%d: CSW\n", __LINE__, __line__);
    status = _test_wait_csw_EX(__LINE__);
    if (status == UX_TRANSFER_STALLED)
    {
        _test_clear_stall(UX_TRUE);
        status = _test_wait_csw_EX(__LINE__);
    }
    return(status);
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
    status = _test_send_cbw(__LINE__);
    if (status == UX_SUCCESS)
    {
        status = _test_transfer_data(__LINE__, request_sense_response, UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH, UX_TRUE);
        if (status == UX_SUCCESS)
        {
            status = _test_wait_csw(__LINE__);
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

typedef struct cbw_read_struct
{
    UCHAR flags_pos;
    UCHAR lba_pos;
    UCHAR lba_size;
    UCHAR len_pos;
    UCHAR len_size;
} cbw_read_struct_t;
static cbw_read_struct_t cbw_READ_info[] =
{
    { 1,  2, 2,  4, 1},
    { 1,  2, 4,  7, 2},
    { 1,  2, 4,  6, 4},
    { 1,  2, 8, 10, 4},
    {10, 12, 8, 28, 4}
};
static UCHAR _read_op(UCHAR op_code)
{
    switch(op_code)
    {
    case 0x28: return 1;
    case 0xA8: return 2;
    case 0x88: return 3;
    case 0x7F: return 4;
    case 0x08: return 0;
    default:   return 0xFF;
    }
}
static void  _test_init_cbw_READ_EX(
    UCHAR flags, ULONG data_length,
    UCHAR op6_10_12_16_32, ULONG lba, ULONG len)
{
UCHAR               *cbw;
UINT                command_length;
UCHAR               op = _read_op(op6_10_12_16_32);
UINT                i;


    if (op >= 5)
        return;

    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;
    command_length =  UX_HOST_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_SBC;
    _ux_host_class_storage_cbw_initialize(storage, flags, data_length, command_length);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0)                                = op6_10_12_16_32;
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + cbw_READ_info[op].flags_pos)      = 0;
    for (i = cbw_READ_info[op].lba_pos + cbw_READ_info[op].lba_size - 1; i >= cbw_READ_info[op].lba_pos; i --)
    {
        *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + i) = (UCHAR)lba;
        lba >>= 8;
    }
    for (i = cbw_READ_info[op].len_pos + cbw_READ_info[op].len_size - 1; i >= cbw_READ_info[op].len_size; i --)
    {
        *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + i) = (UCHAR)len;
        len >>= 8;
    }
}
static void  _test_init_cbw_READ(UCHAR op6_10_12_16_32, ULONG lba, ULONG len)
{
    _test_init_cbw_READ_EX(0x80, len * 512, op6_10_12_16_32, lba, len);
}

typedef struct cbw_write_struct
{
    UCHAR flags_pos;
    UCHAR lba_pos;
    UCHAR lba_size;
    UCHAR len_pos;
    UCHAR len_size;
} cbw_write_struct_t;
static cbw_write_struct_t cbw_WRITE_info[] =
{
    { 1,  2, 2,  4, 1},
    { 1,  2, 4,  7, 2},
    { 1,  2, 4,  6, 4},
    { 1,  2, 8, 10, 4},
    {10, 12, 8, 28, 4}
};
static UCHAR _write_op(UCHAR op_code)
{
    switch(op_code)
    {
    case 0x2A: return 1;
    case 0xAA: return 2;
    case 0x8A: return 3;
    case 0x7F: return 4;
    case 0x0A: return 0;
    default:   return 0xFF;
    }
}
static void  _test_init_cbw_WRITE_EX(UCHAR flags, ULONG data_length,
    UCHAR op6_10_12_16_32, ULONG lba, ULONG len)
{

UCHAR               *cbw;
UINT                command_length;
UCHAR               op = _write_op(op6_10_12_16_32);
UINT                i;


    if (op >= 5)
        return;

    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;
    command_length =  UX_HOST_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_SBC;
    _ux_host_class_storage_cbw_initialize(storage, flags, data_length, command_length);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0)                                = op6_10_12_16_32;
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + cbw_WRITE_info[op].flags_pos)      = 0;
    for (i = cbw_WRITE_info[op].lba_pos + cbw_WRITE_info[op].lba_size - 1; i >= cbw_WRITE_info[op].lba_pos; i --)
    {
        *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + i) = (UCHAR)lba;
        lba >>= 8;
    }
    for (i = cbw_WRITE_info[op].len_pos + cbw_WRITE_info[op].len_size - 1; i >= cbw_WRITE_info[op].len_size; i --)
    {
        *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + i) = (UCHAR)len;
        len >>= 8;
    }
}
static void  _test_init_cbw_WRITE(UCHAR op6_10_12_16_32, ULONG lba, ULONG len)
{
    _test_init_cbw_WRITE_EX(0x00, len * 512, op6_10_12_16_32, lba, len);
}

static void  _test_init_cbw_REQUEST_SENSE(ULONG data_length)
{

UCHAR           *cbw;
UINT            command_length;


    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;
    command_length =  UX_HOST_CLASS_STORAGE_REQUEST_SENSE_COMMAND_LENGTH_SBC;
    _ux_host_class_storage_cbw_initialize(storage, 0x80, data_length, command_length);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0) =  UX_SLAVE_CLASS_STORAGE_SCSI_REQUEST_SENSE;
}

static void  _test_init_cbw_TEST_READY(ULONG data_length)
{

UCHAR           *cbw;
UINT            command_length;


    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;
    command_length =  UX_HOST_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_SBC;
    _ux_host_class_storage_cbw_initialize(storage, 0x00, data_length, command_length);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0) =  UX_SLAVE_CLASS_STORAGE_SCSI_TEST_READY;
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

static void  _test_init_cbw_START_STOP(void)
{

UCHAR           *cbw;
UINT            command_length;


    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;
    command_length =  UX_HOST_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_SBC;
    _ux_host_class_storage_cbw_initialize(storage, 0, 0, command_length);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0) =  UX_SLAVE_CLASS_STORAGE_SCSI_START_STOP;
}

static void  _test_init_cbw_VERIFY(void)
{

UCHAR           *cbw;
UINT            command_length;


    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;
    command_length =  UX_HOST_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_SBC;
    _ux_host_class_storage_cbw_initialize(storage, 0, 0, command_length);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0) =  UX_SLAVE_CLASS_STORAGE_SCSI_VERIFY;
}

static void  _test_init_cbw_MODE_SELECT(ULONG data_length)
{

UCHAR           *cbw;
UINT            command_length;


    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;
    command_length =  UX_HOST_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_SBC;
    _ux_host_class_storage_cbw_initialize(storage, 0, data_length, command_length);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0) =  UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SELECT;
}

static void  _test_init_cbw_MODE_SENSE(UCHAR op6, UCHAR page_code, ULONG buffer_length)
{

UCHAR           *cbw;
UINT            command_length;


    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;
    command_length =  UX_HOST_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_SBC;
    _ux_host_class_storage_cbw_initialize(storage, 0x80, buffer_length, command_length);
    if (op6)
    {
        *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0) =  UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE_SHORT;
        *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_SLAVE_CLASS_STORAGE_MODE_SENSE_PARAMETER_LIST_LENGTH_6) =  (UCHAR)buffer_length;
    }
    else
    {
        *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0) =  UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE;
        _ux_utility_short_put_big_endian(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_SLAVE_CLASS_STORAGE_MODE_SENSE_PARAMETER_LIST_LENGTH_10, (USHORT)buffer_length);
    }
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_SLAVE_CLASS_STORAGE_MODE_SENSE_PC_PAGE_CODE) = page_code;
}

static void  _test_init_cbw_SYNCHRONIZE_CACHE(UCHAR op16, UCHAR immed, ULONG lba, ULONG nb_blocks)
{

UCHAR           *cbw;
UINT            command_length;


    (void)op16;

    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;
    command_length =  UX_HOST_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_SBC;
    _ux_host_class_storage_cbw_initialize(storage, 0, 0, command_length);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0) = UX_SLAVE_CLASS_STORAGE_SCSI_SYNCHRONIZE_CACHE;
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_SLAVE_CLASS_STORAGE_SYNCHRONIZE_CACHE_FLAGS) = immed ? UX_SLAVE_CLASS_STORAGE_SYNCHRONIZE_CACHE_FLAGS_IMMED : 0;
    _ux_utility_long_put_big_endian(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_SLAVE_CLASS_STORAGE_SYNCHRONIZE_CACHE_LBA, lba);
    _ux_utility_short_put_big_endian(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_SLAVE_CLASS_STORAGE_SYNCHRONIZE_CACHE_NUMBER_OF_BLOCKS, (USHORT)nb_blocks);
}

static void  _test_init_cbw_PREVENT_ALLOW_MEDIA_REMOVAL(void)
{

UCHAR           *cbw;
UINT            command_length;


    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;
    command_length =  UX_HOST_CLASS_STORAGE_TEST_READY_COMMAND_LENGTH_SBC;
    _ux_host_class_storage_cbw_initialize(storage, 0, 0, command_length);
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0) =  UX_SLAVE_CLASS_STORAGE_SCSI_PREVENT_ALLOW_MEDIA_REMOVAL;
}


static void _msc_cbw_fail_cases_test(const char* __file__, int __line__)
{
UINT            status;
UCHAR           *cbw;

    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;

    stepinfo("\n%s:%d:MSC CBW fail tests\n", __file__, __line__);

    stepinfo(">>>>>>>>>>>>>>>> Test VERIFY CBW length error\n");
    _test_init_cbw_VERIFY();
    status = _test_send_cbw_EX(__LINE__, UX_HOST_CLASS_STORAGE_CBW_LENGTH - 1);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_TRUE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);

    stepinfo(">>>>>>>>>>>>>>>> Test VERIFY CBW LUN error\n");
    _test_init_cbw_VERIFY();
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_LUN + 0) = 0xFF;
    status = _test_send_cbw_EX(__LINE__, UX_HOST_CLASS_STORAGE_CBW_LENGTH);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_TRUE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);

    stepinfo(">>>>>>>>>>>>>>>> Test VERIFY CBW Signature error\n");
    _test_init_cbw_VERIFY();
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_SIGNATURE + 0) = 0xFF;
    status = _test_send_cbw_EX(__LINE__, UX_HOST_CLASS_STORAGE_CBW_LENGTH);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_TRUE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);

    stepinfo(">>>>>>>>>>>>>>>> Test VERIFY CBW CBW_CB length error\n");
    _test_init_cbw_VERIFY();
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB_LENGTH + 0) = 0;
    status = _test_send_cbw_EX(__LINE__, UX_HOST_CLASS_STORAGE_CBW_LENGTH);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_TRUE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_FALSE);
    _test_clear_stall(UX_TRUE);
    _test_init_cbw_VERIFY();
    status = _test_send_cbw_EX(__LINE__, UX_HOST_CLASS_STORAGE_CBW_LENGTH);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);

    stepinfo(">>>>>>>>>>>>>>>> Test CBW CMD unknown error\n");
    _test_init_cbw_VERIFY();
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + 0) = 0xFF;
    status = _test_send_cbw_EX(__LINE__, UX_HOST_CLASS_STORAGE_CBW_LENGTH);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);
}


static void  _msc_get_max_lun_cases_test(const char* __file__, int __line__)
{
UINT                                        status;
UX_DEVICE                                   *device;
UX_ENDPOINT                                 *control_endpoint;
UX_TRANSFER                                 *transfer_request;

    stepinfo("\n%s:%d:MSC GET_MAX_LUN tests\n", __file__, __line__);

    status = ux_host_stack_device_get(0, &device);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: device_get fail\n", __LINE__, __line__);
        test_control_return(1);
    }
    control_endpoint = &device->ux_device_control_endpoint;
    transfer_request = &control_endpoint->ux_endpoint_transfer_request;

    /* Send transfer request - GetMaxLun. */
    transfer_request -> ux_transfer_request_data_pointer =      UX_NULL;
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_SLAVE_CLASS_STORAGE_GET_MAX_LUN;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             storage -> ux_host_class_storage_interface -> ux_interface_descriptor.bInterfaceNumber;
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Invalid wValue.  */
    transfer_request -> ux_transfer_request_value =             1;
    transfer_request -> ux_transfer_request_index =             storage -> ux_host_class_storage_interface -> ux_interface_descriptor.bInterfaceNumber;
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Invalid wIndex.  */
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             storage -> ux_host_class_storage_interface -> ux_interface_descriptor.bInterfaceNumber + 1;
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Send transfer request - UX_SLAVE_CLASS_STORAGE_RESET.  */
    transfer_request -> ux_transfer_request_function =          UX_SLAVE_CLASS_STORAGE_RESET;

    /* Invalid wValue.  */
    transfer_request -> ux_transfer_request_value =             1;
    transfer_request -> ux_transfer_request_index =             storage -> ux_host_class_storage_interface -> ux_interface_descriptor.bInterfaceNumber;
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Invalid wIndex.  */
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             storage -> ux_host_class_storage_interface -> ux_interface_descriptor.bInterfaceNumber + 1;
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Invalid wLength.  */
    transfer_request -> ux_transfer_request_data_pointer =      (UCHAR*)&status;
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             storage -> ux_host_class_storage_interface -> ux_interface_descriptor.bInterfaceNumber;
    transfer_request -> ux_transfer_request_requested_length =  1;
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}

static void _msc_inquiry_cases_test(const char* __file__, int __line__)
{
UINT                                        status;
UX_SLAVE_DEVICE                             *slave_device;
UX_SLAVE_CLASS                              *slave_class;
UX_SLAVE_CLASS_STORAGE                      *slave_storage;

    stepinfo("\n%s:%d:MSC INQUIRY tests\n", __file__, __line__);

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(standard)\n");
    status = test_host_class_storage_inquiry(storage, 0x00, buffer, 64);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(standard, UX_SLAVE_CLASS_STORAGE_MEDIA_CDROM)\n");
    slave_device =  &_ux_system_slave->ux_system_slave_device;
    slave_class = _ux_system_slave->ux_system_slave_interface_class_array[0];
    slave_storage = (UX_SLAVE_CLASS_STORAGE *)slave_class->ux_slave_class_instance;
    slave_storage -> ux_slave_class_storage_lun[0].ux_slave_class_storage_media_type = UX_SLAVE_CLASS_STORAGE_MEDIA_CDROM;
    status = test_host_class_storage_inquiry(storage, 0x00, buffer, 64);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(unknown page code)\n");
    status = test_host_class_storage_inquiry(storage, 0xEF, buffer, 64);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(big buffer)\n");
    status = _test_host_class_storage_inquiry(storage, 0x80,
        64,
        UX_HOST_CLASS_STORAGE_INQUIRY_COMMAND_LENGTH_SBC,
        0x00, buffer, 64);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(Hi <> Do)\n");
    status = _test_host_class_storage_inquiry(storage, 0x00,
        UX_HOST_CLASS_STORAGE_INQUIRY_RESPONSE_LENGTH,
        UX_HOST_CLASS_STORAGE_INQUIRY_COMMAND_LENGTH_SBC,
        0x00, buffer, UX_HOST_CLASS_STORAGE_INQUIRY_RESPONSE_LENGTH);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(Hi <> Do)\n");
    status = _test_host_class_storage_inquiry(storage, 0x00,
        0,
        UX_HOST_CLASS_STORAGE_INQUIRY_COMMAND_LENGTH_SBC,
        0x00, buffer, UX_HOST_CLASS_STORAGE_INQUIRY_RESPONSE_LENGTH);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
}

static void _msc_read_cases_test(const char* __file__, int __line__)
{
UINT                                        status;

    stepinfo("\n%s:%d:MSC READ tests\n", __file__, __line__);

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_READ32 - success\n");
    _test_init_cbw_READ(UX_SLAVE_CLASS_STORAGE_SCSI_READ32, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 512, UX_TRUE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_READ16 - status fail\n");
    ram_disk_status = UX_ERROR;
    _test_init_cbw_READ(UX_SLAVE_CLASS_STORAGE_SCSI_READ16, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 512, UX_TRUE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    ram_disk_status = UX_SUCCESS;
    _test_clear_stall(UX_TRUE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_READ16 - transfer fail\n");
    _test_init_cbw_READ(UX_SLAVE_CLASS_STORAGE_SCSI_READ16, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    ux_test_dcd_sim_slave_set_actions(fail_on_bulkin);
    status = _test_transfer_data(__LINE__, buffer, 512, UX_TRUE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_TRUE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_READ16 - Case(7) Hi < Di\n");
    _test_init_cbw_READ_EX(0x80, 0, UX_SLAVE_CLASS_STORAGE_SCSI_READ16, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 512, UX_TRUE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_TRUE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_READ16 - Case(8) Hi <> Do\n");
    _test_init_cbw_READ_EX(0x00, 512, UX_SLAVE_CLASS_STORAGE_SCSI_READ16, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 512, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_FALSE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_READ16 - Case(5) Hi > Di\n");
    _test_init_cbw_READ_EX(0x80, 1024, UX_SLAVE_CLASS_STORAGE_SCSI_READ16, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 1024, UX_TRUE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_TRUE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);
}

static void _msc_write_cases_test(const char* __file__, int __line__)
{
UINT                                        status;
UX_SLAVE_DEVICE                             *slave_device;
UX_SLAVE_CLASS                              *slave_class;
UX_SLAVE_CLASS_STORAGE                      *slave_storage;

    stepinfo("\n%s:%d:MSC WRITE tests\n", __file__, __line__);

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_WRITE32 - success\n");
    _test_init_cbw_WRITE(UX_SLAVE_CLASS_STORAGE_SCSI_WRITE32, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 512, UX_FALSE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16 - status fail\n");
    ram_disk_status = UX_ERROR;
    _test_init_cbw_WRITE(UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 512, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    ram_disk_status = UX_SUCCESS;
    _test_clear_stall(UX_FALSE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16 - transfer fail\n");
    _test_init_cbw_WRITE(UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    ux_test_dcd_sim_slave_set_actions(fail_on_bulkout);
    status = _test_transfer_data(__LINE__, buffer, 512, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_FALSE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16 - WP fail\n");
    slave_device =  &_ux_system_slave->ux_system_slave_device;
    slave_class = _ux_system_slave->ux_system_slave_interface_class_array[0];
    slave_storage = (UX_SLAVE_CLASS_STORAGE *)slave_class->ux_slave_class_instance;
    slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_read_only_flag = UX_TRUE;
    _test_init_cbw_WRITE(UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 512, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_FALSE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_read_only_flag = UX_FALSE;

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16 - Case (3) Hn < Do\n");
    _test_init_cbw_WRITE_EX(0x00, 0, UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    #if 0 /* Host expect no data.  */
    status = _test_transfer_data(__LINE__, buffer, 512, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_FALSE);
    #endif
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16 - Case (8) Hi <> Do\n");
    _test_init_cbw_WRITE_EX(0x80, 512, UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 512, UX_TRUE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_TRUE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16 - Case (9) Ho > Do\n");
    _test_init_cbw_WRITE_EX(0x00, 1024, UX_SLAVE_CLASS_STORAGE_SCSI_WRITE16, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 1024, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_FALSE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);
}

static void _msc_request_sense_cases_test(const char* __file__, int __line__)
{
UINT            status;

    stepinfo("\n%s:%d:MSC REQUEST_SENSE tests\n", __file__, __line__);

    _test_init_cbw_REQUEST_SENSE(64);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 64, UX_TRUE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_TRUE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);
}

static void _msc_test_ready_cases_test(const char* __file__, int __line__)
{
UINT status;

    stepinfo("\n%s:%d:MSC UNIT_READY tests\n", __file__, __line__);

    _test_init_cbw_TEST_READY(64);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 64, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_FALSE);
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: code 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    _ux_host_class_storage_device_reset(storage);
}

static void _msc_format_unit_cases_test(const char* __file__, int __line__)
{
UINT status;

    stepinfo("\n%s:%d:MSC FORMAT_UNIT tests\n", __file__, __line__);

    _test_init_cbw_FORMAT_UNIT();
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    #if 0 /* Host expects no data.  */
    status = _test_transfer_data(__LINE__, "test dead beef", 15, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_FALSE);
    #endif
    status = _test_wait_csw(__LINE__);
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
}

static void _msc_start_stop_cases_test(const char* __file__, int __line__)
{
UINT status;

    stepinfo("\n%s:%d:MSC START_STOP tests\n", __file__, __line__);

    _test_init_cbw_START_STOP();
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}

static void _msc_verify_cases_test(const char* __file__, int __line__)
{
UINT status;

    stepinfo("\n%s:%d:MSC VERIFY tests\n", __file__, __line__);

    _test_init_cbw_VERIFY();
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}

static void _msc_mode_sense_cases_test(const char* __file__, int __line__)
{
UINT                                        status;
UX_SLAVE_DEVICE                             *slave_device;
UX_SLAVE_CLASS                              *slave_class;
UX_SLAVE_CLASS_STORAGE                      *slave_storage;

    stepinfo("\n%s:%d:MSC MODE_SENSE tests\n", __file__, __line__);

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE(PAGE_CODE_CDROM):\n");

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE(PAGE_CODE_CDROM) - 128B\n");
    _test_init_cbw_MODE_SENSE(UX_FALSE, UX_SLAVE_CLASS_STORAGE_MMC2_PAGE_CODE_CDROM, 128);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_TRUE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE(PAGE_CODE_CDROM) - 74B\n");
    _test_init_cbw_MODE_SENSE(UX_TRUE, UX_SLAVE_CLASS_STORAGE_MMC2_PAGE_CODE_CDROM, 74);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 74, UX_TRUE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Change read_only_flag */
    slave_device =  &_ux_system_slave->ux_system_slave_device;
    slave_class = _ux_system_slave->ux_system_slave_interface_class_array[0];
    slave_storage = (UX_SLAVE_CLASS_STORAGE *)slave_class->ux_slave_class_instance;
    slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_read_only_flag = UX_TRUE;

    /************ UX_SLAVE_CLASS_STORAGE_PAGE_CODE_CACHE ****************/
    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE(PAGE_CODE_CACHE):\n");

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE(PAGE_CODE_CACHE) - 128B\n");
    _test_init_cbw_MODE_SENSE(UX_FALSE, UX_SLAVE_CLASS_STORAGE_PAGE_CODE_CACHE, 128);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_TRUE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE(PAGE_CODE_CACHE) - 24B\n");
    _test_init_cbw_MODE_SENSE(UX_TRUE, UX_SLAVE_CLASS_STORAGE_PAGE_CODE_CACHE, 24);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 24, UX_TRUE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE(PAGE_CODE_CACHE) - 24B\n");
    slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_flush = demo_media_flush;
    _test_init_cbw_MODE_SENSE(UX_TRUE, UX_SLAVE_CLASS_STORAGE_PAGE_CODE_CACHE, 24);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 24, UX_TRUE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /************ UX_SLAVE_CLASS_STORAGE_PAGE_CODE_IEC ****************/

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE(PAGE_CODE_IEC) - 128B\n");
    _test_init_cbw_MODE_SENSE(UX_FALSE, UX_SLAVE_CLASS_STORAGE_PAGE_CODE_IEC, 128);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_TRUE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE(PAGE_CODE_IEC) - 16B\n");
    _test_init_cbw_MODE_SENSE(UX_TRUE, UX_SLAVE_CLASS_STORAGE_PAGE_CODE_IEC, 16);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 16, UX_TRUE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /************ 0x3F ****************/

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE(0x3F) - 128B\n");
    _test_init_cbw_MODE_SENSE(UX_FALSE, 0x3F, 128);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_TRUE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /************ 0x3F ****************/

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_MODE_SENSE(0x3F) - %dB\n",
        UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE + 8);
    _test_init_cbw_MODE_SENSE(UX_FALSE, 0x3F, UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE + 8);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, buffer, 128, UX_TRUE);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}

static void _msc_mode_select_cases_test(const char* __file__, int __line__)
{
UINT status;

    stepinfo("\n%s:%d:MSC MODE_SELECT tests\n", __file__, __line__);

    _test_init_cbw_MODE_SELECT(15);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_transfer_data(__LINE__, "test dead beef", 15, UX_FALSE);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_FALSE);
    status = _test_wait_csw(__LINE__);
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
}

static void _msc_synchronous_cache_cases_test(const char* __file__, int __line__)
{
UINT                                        status;
UX_SLAVE_DEVICE                             *slave_device;
UX_SLAVE_CLASS                              *slave_class;
UX_SLAVE_CLASS_STORAGE                      *slave_storage;

    stepinfo("\n%s:%d:MSC SYNCHRONOUS_CACHE tests\n", __file__, __line__);

    slave_device =  &_ux_system_slave->ux_system_slave_device;
    slave_class = _ux_system_slave->ux_system_slave_interface_class_array[0];
    slave_storage = (UX_SLAVE_CLASS_STORAGE *)slave_class->ux_slave_class_instance;

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_SYNCHRONIZE_CACHE no CB - success\n");
    slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_flush = UX_NULL;
    slave_storage->ux_slave_class_storage_lun[1].ux_slave_class_storage_media_flush = UX_NULL;
    _test_init_cbw_SYNCHRONIZE_CACHE(UX_FALSE, UX_FALSE, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_SYNCHRONIZE_CACHE - success\n");
    slave_storage->ux_slave_class_storage_lun[0].ux_slave_class_storage_media_flush = demo_media_flush;
    slave_storage->ux_slave_class_storage_lun[1].ux_slave_class_storage_media_flush = demo_media_flush;
    _test_init_cbw_SYNCHRONIZE_CACHE(UX_FALSE, UX_FALSE, 0, 1);
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_SYNCHRONIZE_CACHE - status fail\n");
    _test_init_cbw_SYNCHRONIZE_CACHE(UX_FALSE, UX_FALSE, 0, 1);
    ram_disk_status = UX_ERROR;
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw_EX(__LINE__);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_TRUE);
    status = _test_wait_csw_EX(__LINE__);
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
    ram_disk_status = UX_SUCCESS;

    stepinfo(">>>>>>>>>>>>>>> UX_SLAVE_CLASS_STORAGE_SCSI_SYNCHRONIZE_CACHE - flush fail\n");
    _test_init_cbw_SYNCHRONIZE_CACHE(UX_FALSE, UX_FALSE, 0, 1);
    ram_disk_flush_status = UX_STATE_ERROR;
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw_EX(__LINE__);
    if (status != UX_TRANSFER_STALLED)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _test_clear_stall(UX_TRUE);
    status = _test_wait_csw_EX(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    ram_disk_flush_status = UX_STATE_NEXT;
}

static void _msc_prevent_allow_removal_cases_test(const char* __file__, int __line__)
{
UINT status;

    stepinfo("\n%s:%d:MSC PREVENT_ALLOW_MEDIA_REMOVAL tests\n", __file__, __line__);

    _test_init_cbw_PREVENT_ALLOW_MEDIA_REMOVAL();
    status = _test_send_cbw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_wait_csw(__LINE__);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}


static void _msc_media_read_test(const char* __file__, int __line__)
{
UINT                                        status;
ULONG                                       test_n;
ULONG                                       test_size[] = {
    UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE * 2 / 512,
    UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE * 3 / 512,
    UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE * 2 / 512 + 1};

    stepinfo("\n%s:%d:MSC Media Read tests\n", __file__, __line__);

    if (media == UX_NULL || media -> fx_media_id == 0)
    {
        printf("ERROR %d: media error\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> Disk read(1) test\n");
    {
        status = fx_media_read(media, 48, buffer);
        if (status != FX_SUCCESS)
        {
            printf("ERROR %d: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }
    }

    /* Disk read multiple test.  */
    for (test_n = 0; test_n < sizeof(test_size)/sizeof(test_size[0]); test_n ++)
    {

        stepinfo(">>>>>>>>>>>> Disk read(%d) test\n", test_size[test_n]);
        status = _media_driver_read(media, _ux_host_class_storage_driver_entry,
                buffer, 0, test_size[test_n]);
        if (status != UX_SUCCESS)
        {
            printf("ERROR %d.%ld: 0x%x\n", __LINE__, test_n, status);
            test_control_return(1);
        }
    }
}

static void _msc_media_write_read_test(const char* __file__, int __line__)
{
UINT                                        status;
ULONG                                       test_n;
INT                                         i;
ULONG                                       test_size[] = {
    UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE * 2 / 512,
    UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE * 3 / 512,
    UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE * 2 / 512 + 1};

    stepinfo("\n%s:%d:MSC Media Write & Read tests\n", __file__, __line__);

    /* Check if media still available.  */
    if (media == UX_NULL || media -> fx_media_id == 0)
    {
        printf("ERROR %d: media error\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> Disk write(1)/read(1) test\n");
    {
        for(i = 0; i < 512; i ++)
            buffer[i] = i;
        status = fx_media_write(media, 48, buffer);
        if (status != FX_SUCCESS)
        {
            printf("ERROR %d: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }
        _ux_utility_memory_set(buffer, 0x00, 512);
        status = fx_media_read(media, 48, buffer);
        if (status != FX_SUCCESS)
        {
            printf("ERROR %d: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }
        for (i = 0; i < 512; i ++)
        {
            if (buffer[i] != (UCHAR)i)
            {
                printf("ERROR %d: %d <> %d\n", __LINE__, i, buffer[i]);
                test_control_return(1);
            }
        }
    }

    /* Disk write/read multiple test.  */
    for (test_n = 0; test_n < sizeof(test_size)/sizeof(test_size[0]); test_n ++)
    {
        stepinfo(">>>>>>>>>>>> Disk write(%ld)/read(%ld) test\n",
                                        test_size[test_n], test_size[test_n]);

        for(i = 0; i < test_size[test_n] * 512; i ++)
            buffer[i] = (UCHAR)(i + (i >> 8));
        status = _media_driver_write(media, _ux_host_class_storage_driver_entry,
                buffer, 48, test_size[test_n]);
        if (status != FX_SUCCESS)
        {
            printf("ERROR %d.%ld: 0x%x\n", __LINE__, test_n, status);
            test_control_return(1);
        }
        _ux_utility_memory_set(buffer, 0x00, test_size[test_n] * 512);
        status = _media_driver_read(media, _ux_host_class_storage_driver_entry,
                buffer, 48, test_size[test_n]);
        if (status != FX_SUCCESS)
        {
            printf("ERROR %d.%ld: 0x%x\n", __LINE__, test_n, status);
            test_control_return(1);
        }
        for (i = 0; i < 512; i ++)
        {
            if (buffer[i] != (UCHAR)(i + (i >> 8)))
            {
                printf("ERROR %d: %d <> %d\n", __LINE__, i, buffer[i]);
                test_control_return(1);
            }
        }
    }
}

static void _msc_enumeration_test(const char* __file__, int __line__, unsigned option)
{
UINT                                        status;
ULONG                                       mem_free;
ULONG                                       test_n;

    stepinfo("\n%s:%d:MSC Enumeration tests\n", __file__, __line__);

    stepinfo(">>>>>>>>>>>> Enumeration information collection\n");
    {

        /* Test disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Check connection. */
        status = host_storage_instance_get(0);
        if (status == UX_SUCCESS)
        {

            printf("ERROR #%d\n", __LINE__);
            test_control_return(1);
        }

        /* Reset testing counts. */
        ux_test_utility_sim_mem_alloc_count_reset();
        ux_test_utility_sim_mutex_create_count_reset();
        ux_test_utility_sim_sem_create_count_reset();
        ux_test_utility_sim_sem_get_count_reset();
        ux_test_hcd_sim_host_set_actions(log_on_SetCfg);

        /* Save free memory usage. */
        mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);

        /* Check connection. */
        status =  host_storage_instance_get(50);
        if (status != UX_SUCCESS)
        {

            printf("ERROR #%d\n", __LINE__);
            test_control_return(1);
        }

        /* Log create counts for further tests. */
        rsc_enum_mutex_usage = rsc_mutex_on_set_cfg;
        rsc_enum_sem_usage = rsc_sem_on_set_cfg;
        rsc_enum_mem_alloc_count = rsc_mem_alloc_cnt_on_set_cfg;
        /* Log create counts when instances active for further tests. */
        rsc_storage_mutex_usage = ux_test_utility_sim_mutex_create_count() - rsc_enum_mutex_usage;
        rsc_storage_sem_usage = ux_test_utility_sim_sem_create_count() - rsc_enum_sem_usage;
        rsc_storage_mem_alloc_count = ux_test_utility_sim_mem_alloc_count() - rsc_enum_mem_alloc_count;

        /* Lock log base for tests. */
        ux_test_utility_sim_mem_alloc_log_lock();

        stepinfo("enum mem: %ld\n", rsc_enum_mem_alloc_count);
        stepinfo("storage mem : %ld\n", rsc_storage_mem_alloc_count);
        stepinfo("mem free: %ld, %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available);
    }

    /* Simulate detach and attach for FS enumeration,
       and check if there is memory error in normal enumeration.
     */
    if (option & (1u))
    {
        stepinfo(">>>>>>>>>>>> Enumeration test\n");
        mem_free = (~0);
        for (test_n = 0; test_n < 3; test_n++)
        {
            stepinfo("%4ld / 2\n", test_n);

            /* Disconnect. */
            ux_test_dcd_sim_slave_disconnect();
            ux_test_hcd_sim_host_disconnect();

            /* Check */
            if (host_storage_instance_get(0) == UX_SUCCESS)
            {

                printf("ERROR #%d.%ld: Disconnect fail\n", __LINE__, test_n);
                test_control_return(1);
            }

            /* Update memory free level (disconnect) */
            if (mem_free == (~0))
                mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
            else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
            {

                printf("ERROR #%d.%ld: Memory level different after re-enumerations %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
                test_control_return(1);
            }

            /* Connect. */
            error_callback_counter = 0;
            ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
            ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);

            /* Wait and break on error. */
            error_callback_counter = 0;
            ux_test_breakable_sleep(
                (UX_MS_TO_TICK_NON_ZERO(UX_RH_ENUMERATION_RETRY_DELAY) +
                 UX_MS_TO_TICK_NON_ZERO(UX_HOST_CLASS_STORAGE_DEVICE_INIT_DELAY)) *
                3,
                sleep_break_on_connect);

            /* Check */
            if (host_storage_instance_get(0) != UX_SUCCESS)
            {

                printf("ERROR #%d.%ld: Enumeration fail\n", __LINE__, test_n);
                test_control_return(1);
            }
        }
        stepinfo("\n");
    }

    /* Simulate detach and attach for FS enumeration,
       and test possible memory allocation error handlings.
     */
    if (option & (2u))
    {
        if (rsc_storage_mem_alloc_count) stepinfo(">>>>>>>>>>>> Memory errors enumeration test\n");
        mem_free = (~0);
        for (test_n = 0; test_n < rsc_storage_mem_alloc_count; test_n ++)
        {

            stepinfo("%4ld / %4ld\n", test_n, rsc_storage_mem_alloc_count - 1);

            /* Disconnect. */
            ux_test_dcd_sim_slave_disconnect();
            ux_test_hcd_sim_host_disconnect();

            /* Check */
            if (host_storage_instance_get(0) == UX_SUCCESS)
            {

                printf("ERROR #%d.%ld: Disconnect fail\n", __LINE__, test_n);
                test_control_return(1);
            }

            /* Update memory free level (disconnect) */
            if (mem_free == (~0))
                mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
            else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
            {

                printf("ERROR #%d.%ld: Memory level different after re-enumerations %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
                test_control_return(1);
            }

            /* Set memory error generation */
            ux_test_utility_sim_mem_alloc_error_generation_start(test_n + rsc_enum_mem_alloc_count);

            /* Connect. */
            error_callback_counter = 0;
            ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
            ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

            /* Wait and break on errors. */
            ux_test_breakable_sleep(100, sleep_break_on_error);

            /* Check error */
            if (host_storage_instance_get(0) == UX_SUCCESS)
            {

                /* Could be media errors,
                in this case instance is ready,
                check error trap. */
                if (error_callback_counter == 0)
                {
                    printf("ERROR #%d.%ld: device detected when there is memory error\n", __LINE__, test_n);
                    test_control_return(1);
                }
            }
            stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
        }
        ux_test_utility_sim_mem_alloc_error_generation_stop();
        if (rsc_storage_mem_alloc_count) stepinfo("\n");
    }
}

static void _msc_media_write_read_misc_test(const char* __file__, int __line__)
{
UINT                                        status;
UINT                                        test_size = UX_SLAVE_CLASS_STORAGE_BUFFER_SIZE * 3 / 512;
UINT                                        test_n;
ULONG                                       test_start;
ULONG                                       test_ticks;
INT                                         i;

    stepinfo("\n%s:%d:MSC Media Read tests\n", __file__, __line__);

    if (media == UX_NULL || media -> fx_media_id == 0)
    {
        printf("ERROR %d.%d: media error\n", __LINE__, __line__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> Disk read(%d) test - tick obtain\n", test_size);
    test_start = tx_time_get();
    status = _media_driver_read(media, _ux_host_class_storage_driver_entry,
            buffer, 0, test_size);
    test_ticks = _test_dw_minus(tx_time_get(), test_start);
    if (status != UX_SUCCESS)
    {
        printf("ERROR %d.%d: 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }
    test_ticks /= 3;
    stepinfo(" :: Buffer XFR time: %ld ticks\n", test_ticks);

    stepinfo(">>>>>>>>>>>> Disk write/read(%d) test - slow disk write/read\n", test_size);
    ram_disk_rw_wait_delay = ((test_ticks + 1) >> 1) + 1;
    for(test_n = 0; test_n < 4; test_n ++)
    {
        stepinfo(">>>>>>>>>>>> Disk write/read(%d) test - disk write/read wait %ld\n",
                                            test_size, ram_disk_rw_wait_delay);
        for(i = 0; i < test_size * 512; i ++)
            buffer[i] = (UCHAR)(i + (i >> 8));
        ram_disk_rw_wait_start = tx_time_get();
        status = _media_driver_write(media, _ux_host_class_storage_driver_entry,
                buffer, 48, test_size);
        if (status != FX_SUCCESS)
        {
            printf("ERROR %d.%d.%d: 0x%x\n", __LINE__, __line__, test_n, status);
            test_control_return(1);
        }
        _ux_utility_memory_set(buffer, 0x00, test_size * 512);
        ram_disk_rw_wait_start = tx_time_get();
        status = _media_driver_read(media, _ux_host_class_storage_driver_entry,
                buffer, 48, test_size);
        if (status != FX_SUCCESS)
        {
            printf("ERROR %d.%d.%d: 0x%x\n", __LINE__, __line__, test_n, status);
            test_control_return(1);
        }
        for (i = 0; i < 512; i ++)
        {
            if (buffer[i] != (UCHAR)(i + (i >> 8)))
            {
                printf("ERROR %d.%d.%d: %d <> %d\n", __LINE__, __line__, test_n, i, buffer[i]);
                test_control_return(1);
            }
        }
        ram_disk_rw_wait_delay <<= 1;
    }

    stepinfo(">>>>>>>>>>>> Disk read(%d) test - USB fail while disk waiting\n", test_size);
    ram_disk_rw_wait_start = tx_time_get();
    ram_disk_rw_count = 0;
    ram_disk_rw_fail_mode = 0x81;
    ram_disk_rw_fail_after = 1;
    status = _media_driver_read(media, _ux_host_class_storage_driver_entry,
            buffer, 48, test_size);
    if (status == UX_SUCCESS)
    {
        printf("ERROR %d.%d: 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> Disk write(%d) test - USB fail while disk waiting\n", test_size);
    ram_disk_rw_wait_start = tx_time_get();
    ram_disk_rw_count = 0;
    ram_disk_rw_fail_mode = 0x82;
    ram_disk_rw_fail_after = 0;
    status = _media_driver_write(media, _ux_host_class_storage_driver_entry,
            buffer, 48, test_size);
    if (0 /*status == UX_SUCCESS*/)
    {
        printf("ERROR %d.%d: 0x%x\n", __LINE__, __line__, status);
        test_control_return(1);
    }

    /* Restore: no disk wait delay.  */
    ram_disk_rw_fail_after = 0xFFFFFFFF;
    ram_disk_rw_wait_delay = 0;
}


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                        status;


    /* Find the storage class. */
    status =  host_storage_instance_get(800);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> MSC Read Write test\n");

    _msc_media_write_read_test(__FILE__, __LINE__);
    _msc_media_write_read_misc_test(__FILE__, __LINE__);

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


static UINT    demo_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{

static UCHAR lun_init_done[2] = {0, 0};
UINT         status;
ULONG        mstatus = UX_SLAVE_CLASS_STORAGE_SENSE_KEY_NO_SENSE;


    (void)storage;
    (void)media_id;

    if (ram_disk_status)
    {
        status = ram_disk_status;
        if (media_status)
            *media_status = ram_disk_media_status;
        ram_disk_status = UX_SUCCESS;
        ram_disk_status_sent = UX_TRUE;
        return(status);
    }

    if (lun > 1)
        status = (UX_ERROR);
    else if (lun_init_done[lun] > 0)
        status = (UX_SUCCESS);
    else
    {
        lun_init_done[lun] ++;
        mstatus = UX_SLAVE_CLASS_STORAGE_SENSE_KEY_UNIT_ATTENTION | (0x28 << 8);
        status = (UX_ERROR);
    }

    if (media_status)
        *media_status = mstatus;

    return status;
}

static UINT    demo_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT    status =  0;

    /* Abort.  */
    if (data_pointer == UX_NULL)
    {
        ram_disk_rw_wait_state = 0;
        return(UX_STATE_NEXT);
    }

    /* Media RW fail simulation.  */
    if (ram_disk_rw_fail_after < 0xFFFFFFFF)
    {
        ram_disk_rw_count ++;
        if (ram_disk_rw_count >= ram_disk_rw_fail_after)
        {
            if (ram_disk_rw_fail_mode_one_shot())
            {
                ram_disk_rw_fail_after = 0xFFFFFFFF;
                ram_disk_rw_count = 0;
            }
            if (ram_disk_rw_fail_mode_bulk_in())
                ux_test_dcd_sim_slave_set_actions(fail_on_bulkin);
            else if (ram_disk_rw_fail_mode_bulk_out())
                ux_test_dcd_sim_slave_set_actions(fail_on_bulkout);
            else if (ram_disk_rw_fail_mode_disk())
            {
                *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x02,0x54,0x00);
                return(UX_STATE_ERROR);
            }
        }
    }

    /* Media RW wait simulation.  */
    if (ram_disk_rw_wait_delay)
    {
        if (_test_dw_minus(tx_time_get(), ram_disk_rw_wait_start) < ram_disk_rw_wait_delay)
        {
            ram_disk_rw_wait_state = 1;
            return(UX_STATE_WAIT);
        }
        ram_disk_rw_wait_state = 0;
        ram_disk_rw_wait_start = tx_time_get();
    }

    status = _media_driver_read(ram_disks[lun], _fx_ram_driver, data_pointer, lba, number_blocks);
    if (status != UX_SUCCESS)
    {
        *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x02,0x54,0x00);
        return(UX_STATE_ERROR);
    }
    return(UX_STATE_NEXT);
}

static UINT    demo_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT    status =  0;

    /* Abort.  */
    if (data_pointer == UX_NULL)
    {
        ram_disk_rw_wait_state = 0;
        return(UX_STATE_NEXT);
    }

    /* Media RW fail simulation.  */
    if (ram_disk_rw_fail_after < 0xFFFFFFFF)
    {
        ram_disk_rw_count ++;
        if (ram_disk_rw_count >= ram_disk_rw_fail_after)
        {
            if (ram_disk_rw_fail_mode_one_shot())
            {
                ram_disk_rw_fail_after = 0xFFFFFFFF;
                ram_disk_rw_count = 0;
            }
            if (ram_disk_rw_fail_mode_bulk_in())
                ux_test_dcd_sim_slave_set_actions(fail_on_bulkin);
            else if (ram_disk_rw_fail_mode_bulk_out())
                ux_test_dcd_sim_slave_set_actions(fail_on_bulkout);
            else if (ram_disk_rw_fail_mode_disk())
            {
                *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x02,0x54,0x00);
                return(UX_STATE_ERROR);
            }
        }
    }
    /* Media RW wait simulation.  */
    if (ram_disk_rw_wait_delay)
    {
        if (_test_dw_minus(tx_time_get(), ram_disk_rw_wait_start) < ram_disk_rw_wait_delay)
        {
            ram_disk_rw_wait_state = 1;
            return(UX_STATE_WAIT);
        }
        ram_disk_rw_wait_state = 0;
        ram_disk_rw_wait_start = tx_time_get();
    }

    status = _media_driver_write(ram_disks[lun], _fx_ram_driver, data_pointer, lba, number_blocks);
    if (status != UX_SUCCESS)
    {
        *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x02,0x54,0x00);
        return(UX_STATE_ERROR);
    }
    return(UX_STATE_NEXT);
}

static UINT    demo_media_flush(VOID *storage, ULONG lun, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
    (void)storage;
    (void)number_blocks;
    (void)lba;
    (void)media_status;

    if (lun > 1)
        return UX_STATE_ERROR;

    ram_disk_flush = UX_TRUE;
    return ram_disk_flush_status;
}

static UINT ux_test_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

    switch(event)
    {

        case UX_DEVICE_INSERTION:
            break;

        case UX_DEVICE_REMOVAL:
            break;

#if defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
        case UX_STORAGE_MEDIA_INSERTION:
            /* keep using first media.  */
            if (_ux_host_class_storage_media_index((UX_HOST_CLASS_STORAGE_MEDIA*)inst) == 0)
            {
                _ux_host_class_storage_media_insert((UX_HOST_CLASS_STORAGE_MEDIA*)inst, 1);
                storage_media = (UX_HOST_CLASS_STORAGE_MEDIA*)inst;
                media = _ux_host_class_storage_media_fx_media((UX_HOST_CLASS_STORAGE_MEDIA*)inst);
            }
            break;

        case UX_STORAGE_MEDIA_REMOVAL:
            if (_ux_host_class_storage_media_index((UX_HOST_CLASS_STORAGE_MEDIA*)inst) == 0)
            {
                _ux_host_class_storage_media_remove((UX_HOST_CLASS_STORAGE_MEDIA*)inst);
                storage_media = UX_NULL;
                media = UX_NULL;
            }
            break;
#endif

        default:
            break;
    }

    return 0;
}

