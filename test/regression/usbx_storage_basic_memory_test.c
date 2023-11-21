/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "fx_api.h"

#include "ux_device_class_storage.h"
#include "ux_device_stack.h"
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
VOID               _fx_ram_driver(FX_MEDIA *media_ptr);

static void        demo_thread_entry(ULONG);
static TX_THREAD   tx_demo_thread_host_simulation;
static TX_THREAD   tx_demo_thread_slave_simulation;
static void        tx_demo_thread_host_simulation_entry(ULONG);

static UINT        demo_thread_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT        demo_thread_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT        demo_thread_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status);

static VOID        ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);

/* Define global data structures.  */

static UCHAR                        usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

static ULONG                        error_counter;

static TX_THREAD                    demo_thread;

static UX_HOST_CLASS_STORAGE        *storage;
static UX_SLAVE_CLASS_STORAGE_PARAMETER global_storage_parameter;

static FX_MEDIA                     ram_disk1;
static FX_MEDIA                     ram_disk2;
static CHAR                         ram_disk_memory1[UX_RAM_DISK_SIZE];
static CHAR                         ram_disk_memory2[UX_RAM_DISK_SIZE];
static UCHAR                        buffer1[512];
static UCHAR                        buffer2[512];

static FX_MEDIA                     *ram_disks[] = {&ram_disk1, &ram_disk2};
static UCHAR                        *buffers[] = {buffer1, buffer2};
static CHAR                         *ram_disk_memory[] = { ram_disk_memory1, ram_disk_memory2 };

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
                class -> ux_host_class_ext != UX_NULL &&
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
void    usbx_storage_basic_memory_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
ULONG                           mem_free;
ULONG                           test_n;


    /* Inform user.  */
    printf("Running Storage Basic Memory Test................................... ");
    stepinfo("\n");

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

    /* Reset testing counts. */
    ux_test_utility_sim_mem_alloc_count_reset();

    /* Initialize the device storage class. The class is connected with interface 0 on configuration 1. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry,
                                                1, 0, (VOID *)&global_storage_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    rsc_storage_mem_alloc_count = ux_test_utility_sim_mem_alloc_count();

    /* Lock log base for tests. */
    ux_test_utility_sim_mem_alloc_log_lock();

    stepinfo("init mem : %ld\n", rsc_storage_mem_alloc_count);

    if (rsc_storage_mem_alloc_count) stepinfo(">>>>>>>>>>>> Memory errors class register test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < rsc_storage_mem_alloc_count; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_storage_mem_alloc_count - 1);

        /* Unregister.  */
        ux_device_stack_class_unregister(_ux_system_slave_class_storage_name, ux_device_class_storage_entry);

        /* Update memory free level (unregister).  */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #%d.%ld: Memory level different after re-register %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            test_control_return(1);
        }

        /* Set memory error generation */
        ux_test_utility_sim_mem_alloc_error_generation_start(test_n);

        /* Initialize the device storage class. The class is connected with interface 0 on configuration 1. */
        status =  ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry,
                                                    1, 0, (VOID *)&global_storage_parameter);
        if (status == UX_SUCCESS)
        {
            printf("ERROR #%d.%ld: Class registered when there is memory error\n", __LINE__, test_n);
            test_control_return(1);
        }
    }
    ux_test_utility_sim_mem_alloc_error_generation_stop();
    if (rsc_storage_mem_alloc_count) stepinfo("\n");

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


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                        status;
ULONG                                       mem_free;
ULONG                                       test_n;


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

    /* Find the storage class. */
    status =  host_storage_instance_get(100);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

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
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

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

    /* Simulate detach and attach for FS enumeration,
       and check if there is memory error in normal enumeration.
     */
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
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        /* Wait and break on error. */
        ux_test_breakable_sleep(100, sleep_break_on_error);

        /* Check */
        if (host_storage_instance_get(0) != UX_SUCCESS)
        {

            printf("ERROR #%d.%ld: Enumeration fail\n", __LINE__, test_n);
            test_control_return(1);
        }
    }
    stepinfo("\n");

    /* Simulate detach and attach for FS enumeration,
       and test possible memory allocation error handlings.
     */
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

static UCHAR lun_init_done[2] = {0, 0};
UINT         status;
ULONG        mstatus = UX_SLAVE_CLASS_STORAGE_SENSE_KEY_NO_SENSE;


    (void)storage;
    (void)media_id;


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

UINT    demo_thread_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT    status =  0;

    if(lba == 0)
    {
        ram_disks[lun]->fx_media_driver_logical_sector =  0;
        ram_disks[lun]->fx_media_driver_sectors =  1;
        ram_disks[lun]->fx_media_driver_request =  FX_DRIVER_BOOT_READ;
        ram_disks[lun]->fx_media_driver_buffer =  data_pointer;
        _fx_ram_driver(ram_disks[lun]);
        *(data_pointer) =  0xeb;
        *(data_pointer+1) =  0x3c;
        *(data_pointer+2) =  0x90;
        *(data_pointer+21) =  0xF8;

        *(data_pointer+24) =  0x01;
        *(data_pointer+26) =  0x10;
        *(data_pointer+28) =  0x01;

        *(data_pointer+510) =  0x55;
        *(data_pointer+511) =  0xaa;
        ux_utility_memory_copy(data_pointer+0x36,"FAT12",5);


        status = ram_disks[lun]->fx_media_driver_status;
    }
    else
    {
        while(number_blocks--)
        {
            status =  fx_media_read(ram_disks[lun],lba,data_pointer);
            data_pointer+=512;
            lba++;
        }
    }
    return(status);
}

UINT    demo_thread_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT    status =  0;

    if(lba == 0)
    {
        ram_disks[lun]->fx_media_driver_logical_sector =  0;
        ram_disks[lun]->fx_media_driver_sectors =  1;
        ram_disks[lun]->fx_media_driver_request =  FX_DRIVER_BOOT_WRITE;
        ram_disks[lun]->fx_media_driver_buffer =  data_pointer;
        _fx_ram_driver(ram_disks[lun]);

        status = ram_disks[lun]->fx_media_driver_status;

    }
    else
    {

        while(number_blocks--)
        {
            status =  fx_media_write(ram_disks[lun],lba,data_pointer);
            data_pointer+=512;
            lba++;
        }
        return(status);
    }
    return(1);
}