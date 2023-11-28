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

#define                             UX_RAM_DISK_SIZE                (20 * 1000 * 1024)
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

/* Define global data structures.  */

static UCHAR                        usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

static ULONG                        error_counter;

static TX_THREAD                    demo_thread;

static UX_HOST_CLASS_STORAGE        *storage;
static UX_HOST_CLASS_STORAGE_MEDIA  *storage_media;
static FX_MEDIA                     *storage_disk;
static UX_SLAVE_CLASS_STORAGE_PARAMETER global_storage_parameter;

static FX_FILE                      my_file;

static FX_MEDIA                     ram_disk1;
static FX_MEDIA                     ram_disk2;
static CHAR                         ram_disk_memory1[UX_RAM_DISK_SIZE];
static CHAR                         ram_disk_memory2[UX_RAM_DISK_SIZE];
static UCHAR                        buffer1[512];
static UCHAR                        buffer2[512];

static FX_MEDIA                     *ram_disks[] = {&ram_disk1, &ram_disk2};
static UCHAR                        *buffers[] = {buffer1, buffer2};
static CHAR                         *ram_disk_memory[] = { ram_disk_memory1, ram_disk_memory2 };

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

UINT                                        status;
UX_HOST_CLASS                               *class;

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
            if ((storage -> ux_host_class_storage_state == UX_HOST_CLASS_INSTANCE_LIVE) &&
                (class -> ux_host_class_ext != UX_NULL) &&
                (class -> ux_host_class_media != UX_NULL))
            {
                storage_media = (UX_HOST_CLASS_STORAGE_MEDIA *)class->ux_host_class_media;
#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
                storage_disk = &storage_media -> ux_host_class_storage_media;
#endif
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


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_storage_fats_exfat_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
ULONG                           mem_free;
ULONG                           test_n;


    /* Inform user.  */
    printf("Running Storage FATs & exFAT Test................................... ");
    stepinfo("\n");
#if defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    printf("Skip\n");
    test_control_return(0);
    return;
#endif

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

    /* Initialize the device storage class. The class is connected with interface 0 on configuration 1. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry,
                                                1, 0, (VOID *)&global_storage_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

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

static UINT _test_format_FAT(void)
{
UINT status;

    status =   fx_media_format(&ram_disk1, _fx_ram_driver, ram_disk_memory1, buffer1, 512, "RAM DISK1", 2, 512, 0, UX_RAM_DISK_SIZE/512, 512, 4, 1, 1);
    if (status != FX_SUCCESS)
        return(status);
    status =   fx_media_format(&ram_disk2, _fx_ram_driver, ram_disk_memory2, buffer2, 512, "RAM DISK2", 2, 512, 0, UX_RAM_DISK_SIZE/512, 512, 4, 1, 1);
    return(status);
}

static UINT _test_format_exFAT(void)
{
UINT status;

    /* Format the media.  This needs to be done before opening it!  */
    status =  fx_media_exFAT_format(&ram_disk1, 
                            _fx_ram_driver,         // Driver entry
                            ram_disk_memory1,       // RAM disk memory pointer
                            buffer1,                // Media buffer pointer
                            512,                    // Media buffer size 
                            "RAM_DISK1",            // Volume Name
                            1,                      // Number of FATs
                            0,                      // Hidden sectors
                            UX_RAM_DISK_SIZE/512,   // Total sectors 
                            512,                    // Sector size   
                            1,                      // exFAT Sectors per cluster
                            11111,                  // Volume ID
                            0);                     // Boundary unit
    if (status != FX_SUCCESS)
        return(status);

    status =  fx_media_exFAT_format(&ram_disk2, 
                            _fx_ram_driver,         // Driver entry
                            ram_disk_memory2,       // RAM disk memory pointer
                            buffer1,                // Media buffer pointer
                            512,                    // Media buffer size 
                            "RAM_DISK1",            // Volume Name
                            1,                      // Number of FATs
                            0,                      // Hidden sectors
                            UX_RAM_DISK_SIZE/512,   // Total sectors 
                            512,                    // Sector size   
                            1,                      // exFAT Sectors per cluster
                            22222,                  // Volume ID
                            0);                     // Boundary unit
    return(status);
}

static UINT _test_media_open(void)
{
UINT status;
    status =   fx_media_open(&ram_disk1, "RAM DISK1", _fx_ram_driver, ram_disk_memory1, buffer1, 512);
    if (status != FX_SUCCESS)
        return(status);
    status =  fx_media_open(&ram_disk2, "RAM DISK2", _fx_ram_driver, ram_disk_memory2, buffer2, 512);
    return(status);
}

static UINT _test_media_close(void)
{
    fx_media_close(&ram_disk1);
    fx_media_close(&ram_disk2);
}

static VOID _test_storage_disk(void)
{
UINT            status;
ULONG           actual;
UCHAR           local_buffer[32];

    // printf("Sector 0: %02x %02x %02x %c %c %c %c %c ...\n", ram_disk_memory1[0], ram_disk_memory1[1], ram_disk_memory1[2], ram_disk_memory1[3], ram_disk_memory1[4], ram_disk_memory1[5], ram_disk_memory1[6], ram_disk_memory1[7]);

    /* Create a file called TEST.TXT in the root directory.  */
    status =  fx_file_create(storage_disk, "TEST.TXT");

    /* Check the create status.  */
    if (status != FX_SUCCESS)
    {

        /* Check for an already created status.  This is not fatal, just 
           let the user know.  */
        if (status != FX_ALREADY_CREATED)
        {

            printf("f_create ERROR %x!\n", status);
            return;
        }
    }

    /* Open the test file.  */
    status =  fx_file_open(storage_disk, &my_file, "TEST.TXT", FX_OPEN_FOR_WRITE);

    /* Check the file open status.  */
    if (status != FX_SUCCESS)
    {

        printf("f_open ERROR %x!\n", status);
        return;
    }

    /* Seek to the beginning of the test file.  */
    status =  fx_file_seek(&my_file, 0);

    /* Check the file seek status.  */
    if (status != FX_SUCCESS)
    {

        printf("f_seek ERROR %x!\n", status);
        return;
    }

    /* Write a string to the test file.  */
    status =  fx_file_write(&my_file, " ABCDEFGHIJKLMNOPQRSTUVWXYZ\n", 28);

    /* Check the file write status.  */
    if (status != FX_SUCCESS)
    {

        printf("f_write ERROR %x!\n", status);
        return;
    }

    /* Seek to the beginning of the test file.  */
    status =  fx_file_seek(&my_file, 0);

    /* Check the file seek status.  */
    if (status != FX_SUCCESS)
    {

        printf("%d:f_seek ERROR %x!\n", __LINE__, status);
        return;
    }

    /* Read the first 28 bytes of the test file.  */
    status =  fx_file_read(&my_file, local_buffer, 28, &actual);

    /* Check the file read status.  */
    if ((status != FX_SUCCESS) || (actual != 28))
    {

        printf("%d:f_read ERROR %x!\n", __LINE__, status);
        return;
    }

    /* Close the test file.  */
    status =  fx_file_close(&my_file);

    /* Check the file close status.  */
    if (status != FX_SUCCESS)
    {

        printf("%d:f_close ERROR %x!\n", __LINE__, status);
        return;
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
    status =   _test_format_FAT();
    if (status != FX_SUCCESS)
    {

        /* Storage basic test error.  */
        printf("ERROR #8\n");
        test_control_return(1);
    }

    /* Open the ram_disk.  */
    status = _test_media_open();
    if (status != FX_SUCCESS)
    {

        /* Storage basic test error.  */
        printf("ERROR %d\n", __LINE__);
        test_control_return(1);
    }

    /* Find the storage class. */
    status =  host_storage_instance_get(500);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Test (FAT).  */
    // printf("Test RW on FAT\n");
    _test_storage_disk();

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

    /* Close the ram_disk.  */
    _test_media_close();
    status = _test_format_exFAT();
    if (status != UX_SUCCESS)
    {

        /* Storage basic test error.  */
        printf("ERROR %d, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _test_media_open();
    if (status != UX_SUCCESS)
    {

        /* Storage basic test error.  */
        printf("ERROR %d\n", __LINE__);
        test_control_return(1);
    }

    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

    /* Check connection. */
    status =  host_storage_instance_get(500);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Test (exFAT).  */
    // printf("Test RW on exFAT\n");
    _test_storage_disk();

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