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

FX_MEDIA    *_ux_host_class_storage_driver_media(INT i);
VOID        _ux_host_class_storage_driver_entry(FX_MEDIA *media);
VOID        _ux_host_class_storage_media_insert(UX_HOST_CLASS_STORAGE_MEDIA *storage_media, ULONG format_open);
VOID        _ux_host_class_storage_media_remove(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
INT         _ux_host_class_storage_media_index(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
FX_MEDIA    *_ux_host_class_storage_media_fx_media(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
UCHAR       *_ux_host_class_storage_media_fx_media_memory(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
VOID        _ux_host_class_storage_driver_read_write_notify(
    VOID (*func)(UINT, UINT, UX_HOST_CLASS_STORAGE *, ULONG, ULONG, UCHAR*));

static VOID demo_host_media_read_write_notify(UINT fx_req, UINT fx_rc,
                                UX_HOST_CLASS_STORAGE *storage,
                                ULONG sec_start, ULONG sec_count, UCHAR* buf);

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
static UX_HOST_CLASS_STORAGE_MEDIA      *storage_media = UX_NULL;
static FX_MEDIA                         *media = UX_NULL;
static UX_SLAVE_CLASS_STORAGE_PARAMETER global_storage_parameter;

static ULONG                            host_event;
static UX_HOST_CLASS                    *host_event_cls;
static VOID                             *host_event_inst;

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
static ULONG                            ram_disk_media_attention = 0;
static ULONG                            ram_disk_media_status = 0;
static CHAR                             ram_disk_status_sent = 0;

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

UINT                        status;
UX_HOST_CLASS               *class;
UX_HOST_CLASS_STORAGE_MEDIA *tmp_media;

    /* Find the main storage container */
    status =  ux_host_stack_class_get(_ux_system_host_class_storage_name, &class);
    if (status != UX_SUCCESS)
        return(status);

    /* Get storage instance, wait it to be live and media attached.  */
    do
    {
        /* Run host tasks.  */
        ux_system_tasks_run();

        if (timeout_x10ms)
        {
            tx_thread_sleep(UX_MS_TO_TICK_NON_ZERO(10));
            if (timeout_x10ms != 0xFFFFFFFF)
                timeout_x10ms --;
        }

        status =  ux_host_stack_class_instance_get(class, 0, (void **) &storage);
        if (status == UX_SUCCESS)
        {
            /* Always use first storage media.  */
            status = ux_host_class_storage_media_get(storage, 0, &tmp_media);
            if (status == UX_SUCCESS && storage_media == UX_NULL)
            {
                stepinfo("%s:%d >>>>>>>>>>>>>>> Mount media %p\n", __FILE__, __LINE__, (void*)tmp_media);
                /* Use callback to check read/write.  */
                _ux_host_class_storage_driver_read_write_notify(demo_host_media_read_write_notify);
                /* Media must not be associated inside callback. Do it now.  */
                storage_media = tmp_media;
                _ux_host_class_storage_media_insert(storage_media, 1);
                media = _ux_host_class_storage_media_fx_media(storage_media);
                return(UX_SUCCESS);
            }
            if (status == UX_SUCCESS && tmp_media == storage_media)
                return(UX_SUCCESS);
        }

        if (status != UX_SUCCESS && storage_media != UX_NULL)
        {
            stepinfo("%s:%d >>>>>>>>>>>>>>> Remove media %p\n", __FILE__, __LINE__, (void*)storage_media);
            _ux_host_class_storage_media_remove(storage_media);
            storage_media = UX_NULL;
            media = UX_NULL;
        }

    } while(timeout_x10ms > 0);

    return(UX_ERROR);
}

static UINT sleep_break_on_error(VOID)
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

static UINT sleep_break_on_disconnect(VOID)
{
    if (host_storage_instance_get(0) == UX_SUCCESS)
        return(0);
    return(1);
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
void    usbx_standalone_host_storage_insert_eject_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
ULONG                           mem_free;
ULONG                           test_n;


    /* Inform user.  */
    printf("Running STANDALONE Host Storage Insert Eject Test................... ");
#ifndef UX_HOST_STANDALONE
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
#if defined(UX_DEVICE_STANDALONE)
        /* Run device tasks.  */
        ux_system_tasks_run();
#endif
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
        status =  host_storage_instance_get(100);
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
                50,
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

                stepinfo("ERROR #%d.%ld: Disconnect fail\n", __LINE__, test_n);
                test_control_return(1);
            }

            /* Update memory free level (disconnect) */
            if (mem_free == (~0))
                mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
            else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
            {

                stepinfo("ERROR #%d.%ld: Memory level different after re-enumerations %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
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
                    stepinfo("ERROR #%d.%ld: device detected when there is memory error\n", __LINE__, test_n);
                    test_control_return(1);
                }
            }
            stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
        }
        ux_test_utility_sim_mem_alloc_error_generation_stop();
        if (rsc_storage_mem_alloc_count) stepinfo("\n");
    }

    /* If storage disconnected, re-connect.  */
    if (host_storage_instance_get(0) != UX_SUCCESS)
    {
        ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);

        ux_test_breakable_sleep(
            (UX_MS_TO_TICK_NON_ZERO(UX_RH_ENUMERATION_RETRY_DELAY) +
                UX_MS_TO_TICK_NON_ZERO(UX_HOST_CLASS_STORAGE_DEVICE_INIT_DELAY)) *
            50,
            sleep_break_on_connect);

        /* Check */
        if (host_storage_instance_get(0) != UX_SUCCESS)
        {

            printf("ERROR #%d: Enumeration fail\n", __LINE__);
            test_control_return(1);
        }
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
    for(test_n = 0; test_n < 1; test_n ++)
    {
        stepinfo(">>>>>>>>>>>> Disk write/read(%d) test - disk write/read\n", test_size);
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
    }
}

static void _msc_media_insert_eject_test(const char* __file__, int __line__)
{
UINT test_n;
UINT connected;
UINT status;

    stepinfo("\n%s:%d:MSC Media Insert/Eject tests\n", __file__, __line__);

    if (media == UX_NULL || media -> fx_media_id == 0)
    {
        printf("ERROR %d.%d: media error\n", __LINE__, __line__);
        test_control_return(1);
    }

    /* LUN Eject/Insert detection with host stack tasks run and storage media get.  */
    connected = UX_TRUE;
    error_counter = 0;
    for (test_n = 0; test_n < 3; test_n ++)
    {

        if (connected)
        {
            stepinfo(">>>>>>>>>>>> Disk Eject test #%d\n", __LINE__);
            ram_disk_status = UX_ERROR;
            ram_disk_media_attention = 0;
            ram_disk_media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x02, 0x3A, 0x00);
            ux_test_breakable_sleep(UX_MS_TO_TICK(UX_HOST_CLASS_STORAGE_THREAD_SLEEP_TIME) * 3 / 2,
                                    sleep_break_on_disconnect);
            connected = host_storage_instance_get(0) == UX_SUCCESS;
            if (connected)
            {

                printf("ERROR #%d: LUN eject fail\n", __LINE__);
                error_counter ++;
                continue;
            }
        }

        if (!connected)
        {
            stepinfo(">>>>>>>>>>>> Disk Insert test #%d\n", __LINE__);
            ram_disk_status = UX_SUCCESS;
            ram_disk_media_attention = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x06, 0x28, 0x00);
            ram_disk_media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x00, 0x00, 0x00);
            error_callback_counter = 0;
            ux_test_breakable_sleep(UX_MS_TO_TICK(UX_HOST_CLASS_STORAGE_THREAD_SLEEP_TIME) * 3 / 2,
                                    sleep_break_on_connect);
            connected = host_storage_instance_get(0) == UX_SUCCESS;
            if (!connected)
            {

                printf("ERROR #%d: LUN insert fail\n", __LINE__);
                error_counter ++;
                continue;
            }
        }
    }
    if (error_counter > 0)
    {
        printf("ERROR #%d.%d: LUN change detection fail\n", __LINE__, __line__);
        test_control_return(1);
    }

    /* LUN Eject/Insert detection with media check (blocking).  */
    error_counter = 0;
    for (test_n = 0; test_n < 3; test_n ++)
    {

        stepinfo(">>>>>>>>>>>> Disk Eject test #%d\n", __LINE__);
        ram_disk_status = UX_ERROR;
        ram_disk_media_attention = 0;
        ram_disk_media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x02, 0x3A, 0x00);
        status = ux_host_class_storage_media_lock(storage_media, 100);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d.%d: LUN lock fail 0x%x\n", __LINE__, __line__, status);
            test_control_return(1);
        }
        status = ux_host_class_storage_media_check(storage_media->ux_host_class_storage_media_storage);
        if (status == UX_SUCCESS)
        {
            printf("ERROR #%d.%d: LUN Eject fail\n", __LINE__, __line__);
            test_control_return(1);
        }
        ux_host_class_storage_media_unlock(storage_media);
        /* Unmount media.  */
        _ux_host_class_storage_media_remove(storage_media);

        stepinfo(">>>>>>>>>>>> Disk Insert test #%d\n", __LINE__);
        ram_disk_status = UX_SUCCESS;
        ram_disk_media_attention = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x06, 0x28, 0x00);
        ram_disk_media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x00, 0x00, 0x00);
        status = ux_host_class_storage_media_lock(storage_media, 100);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d.%d: LUN lock fail 0x%x\n", __LINE__, __line__, status);
            test_control_return(1);
        }
        status = ux_host_class_storage_media_check(storage_media->ux_host_class_storage_media_storage);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d.%d: LUN Insert fail 0x%x\n", __LINE__, __line__, status);
            test_control_return(1);
        }
        ux_host_class_storage_media_unlock(storage_media);
        /* Mount media.  */
        _ux_host_class_storage_media_insert(storage_media, 1);
        media = _ux_host_class_storage_media_fx_media(storage_media);
    }
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                        status;


    /* Find the storage class. */
    status =  host_storage_instance_get(500);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> MSC Insert Eject test\n");

    _msc_media_insert_eject_test(__FILE__, __LINE__);

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

    if (lun == 0 && ram_disk_media_attention)
    {
        if (media_status)
            *media_status = ram_disk_media_attention;
        ram_disk_media_attention = 0;
        return(UX_ERROR);
    }
    if (lun == 0 && ram_disk_status)
    {
        status = ram_disk_status;
        if (media_status)
            *media_status = ram_disk_media_status;
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


    status = _media_driver_read(ram_disks[lun], _fx_ram_driver, data_pointer, lba, number_blocks);
    if (status != UX_SUCCESS)
    {
        *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x02,0x54,0x00);
        return(UX_ERROR);
    }
    return(UX_SUCCESS);
}

static UINT    demo_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT    status =  0;


    status = _media_driver_write(ram_disks[lun], _fx_ram_driver, data_pointer, lba, number_blocks);
    if (status != UX_SUCCESS)
    {
        *media_status = UX_DEVICE_CLASS_STORAGE_SENSE_STATUS(0x02,0x54,0x00);
        return(UX_ERROR);
    }
    return(UX_SUCCESS);
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
            stepinfo("Function insert: %p, %p\n", (void*)cls, inst);
            break;

        case UX_DEVICE_REMOVAL:
            stepinfo("Function removal: %p, %p\n", (void*)cls, inst);
            break;

        case UX_DEVICE_CONNECTION:
            stepinfo("Device connect: %p, %p\n", (void*)cls, inst);
            break;

        case UX_DEVICE_DISCONNECTION:
            stepinfo("Device disconnect: %p, %p\n", (void*)cls, inst);
            break;

        case UX_STORAGE_MEDIA_INSERTION:
            stepinfo("Media insert: %p\n", inst);
            break;

        case UX_STORAGE_MEDIA_REMOVAL:
            stepinfo("Media removal: %p\n", inst);
            break;

        case UX_STANDALONE_WAIT_BACKGROUND_TASK:
            tx_thread_relinquish();

        default:
            break;
    }

    return 0;
}

static void dump_data(UCHAR *buf, ULONG len)
{
ULONG l;
    for(l = 0; l < len; l ++)
    {
        if ((l % 32) == 0) printf("\n[%4ld]", l);
        printf(" %02X", buf[l]);
    }
    printf("\n");
}
static VOID demo_host_media_read_write_notify(UINT fx_req, UINT fx_rc,
                                UX_HOST_CLASS_STORAGE *storage,
                                ULONG sec_start, ULONG sec_count, UCHAR* buf)
{
    UX_PARAMETER_NOT_USED(fx_req);
    UX_PARAMETER_NOT_USED(fx_rc);
    UX_PARAMETER_NOT_USED(storage);
    UX_PARAMETER_NOT_USED(sec_start);
    UX_PARAMETER_NOT_USED(sec_count);
    UX_PARAMETER_NOT_USED(buf);
#if 0
    if (fx_req == FX_DRIVER_READ)
    {
        printf("Read(%ld,%ld) : 0x%x\n", sec_start, sec_count, fx_rc);
    }
    if (fx_req == FX_DRIVER_WRITE)
    {
        printf("Write(%ld,%ld) : 0x%x\n", sec_start, sec_count, fx_rc);
    }
    dump_data(buf, 1 * 512);

    printf("Ref data:");
    dump_data(ram_disk_memory1 + sec_start * 512, 1 * 512);
#endif
}
