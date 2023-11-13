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

/* Define constants.  */
#define                             UX_DEMO_STACK_SIZE              2048
#define                             UX_DEMO_BUFFER_SIZE             2048
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE   512
#define                             UX_DEMO_FILE_BUFFER_SIZE        512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE    64
#define                             UX_DEMO_MEMORY_SIZE             (256*1024)
#define                             UX_DEMO_FILE_SIZE               (128 * 512)
#define                             UX_RAM_DISK_SIZE                (200 * 1024)
#define                             UX_RAM_DISK_LAST_LBA            ((UX_RAM_DISK_SIZE / 512) -1)

/* Define local/extern function prototypes.  */
static void                         demo_thread_entry(ULONG);
static TX_THREAD                    tx_demo_thread_host_simulation;
static TX_THREAD                    tx_demo_thread_slave_simulation;
static void                         tx_demo_thread_host_simulation_entry(ULONG);



VOID                                _fx_ram_driver(FX_MEDIA *media_ptr);

static UINT                         demo_thread_media_read1(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                         demo_thread_media_write1(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                         demo_thread_media_status1(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status);

static UINT                         demo_thread_media_read2(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                         demo_thread_media_write2(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                         demo_thread_media_status2(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status);


/* Define global data structures.  */
UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
ULONG                               error_counter;
TX_THREAD                           demo_thread;
UX_HOST_CLASS_STORAGE               *storage;
UX_HOST_CLASS_STORAGE_MEDIA         *global_storage_media;
FX_MEDIA                            *media1;
FX_MEDIA                            *media2;
UINT                                status;
UINT                                transfer_completed;
ULONG                               requested_length;
FX_FILE                             my_file;
TX_SEMAPHORE                        demo_semaphore;
CHAR                                file_name[64];
UCHAR                               global_buffer[UX_DEMO_FILE_BUFFER_SIZE];
UCHAR                               buffer[512];
FX_MEDIA                            ram_disk1;
CHAR                                ram_disk_memory1[UX_RAM_DISK_SIZE];
FX_MEDIA                            ram_disk2;
CHAR                                ram_disk_memory2[UX_RAM_DISK_SIZE];
UX_SLAVE_CLASS_STORAGE_PARAMETER    global_storage_parameter;

#if defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
FX_MEDIA    *_ux_host_class_storage_driver_media(INT i);
VOID        _ux_host_class_storage_driver_entry(FX_MEDIA *media);
VOID        _ux_host_class_storage_media_insert(UX_HOST_CLASS_STORAGE_MEDIA *storage_media, ULONG format_open);
VOID        _ux_host_class_storage_media_remove(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
FX_MEDIA    *_ux_host_class_storage_media_fx_media(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);
UCHAR       *_ux_host_class_storage_media_fx_media_memory(UX_HOST_CLASS_STORAGE_MEDIA *storage_media);

static UINT ux_test_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

    switch(event)
    {

        case UX_STORAGE_MEDIA_INSERTION:
            _ux_host_class_storage_media_insert((UX_HOST_CLASS_STORAGE_MEDIA*)inst, 1);
            break;

        case UX_STORAGE_MEDIA_REMOVAL:
            _ux_host_class_storage_media_remove((UX_HOST_CLASS_STORAGE_MEDIA*)inst);
            _ux_host_class_storage_media_fx_media((UX_HOST_CLASS_STORAGE_MEDIA*)inst) -> fx_media_id = 0;/* Testing code is checking ID to detect removal.  */
            break;

        default:
            break;
    }

    return 0;
}
#else
#define ux_test_system_host_change_function UX_NULL
#endif


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

    /* Failed test.  */
    printf("Error on line %d, system_level: %d, system_context: %d, error code: %d\n", __LINE__, system_level, system_context, error_code);
    test_control_return(1);
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_storage_multi_lun_test_application_define(void *first_unused_memory)
#endif
{
    
UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Initialize FileX.  */
    fx_system_initialize();

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running Multiple LUN Storage Test................................... ERROR #1\n");
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* Reset ram disks memory.  */
    ux_utility_memory_set(ram_disk_memory1, 0, UX_RAM_DISK_SIZE);
    ux_utility_memory_set(ram_disk_memory2, 0, UX_RAM_DISK_SIZE);

    /* The code below is required for installing the device portion of USBX. 
       In this demo, DFU is possible and we have a call back for state change. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("Running Multiple LUN Storage Test................................... ERROR #5\n");
        test_control_return(1);
    }

    /* Store the number of LUN in this device storage instance.  */
    global_storage_parameter.ux_slave_class_storage_parameter_number_lun = 2;

    /* Initialize the storage class parameters for reading/writing to the first Flash Disk.  */
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_last_lba        =  UX_RAM_DISK_LAST_LBA;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_block_length    =  512;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_type            =  0;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_removable_flag  =  0x80;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read            =  demo_thread_media_read1;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write           =  demo_thread_media_write1; 
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status          =  demo_thread_media_status1;

    /* Initialize the storage class parameters for reading/writing to the second Flash Disk.  */
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_last_lba        =  UX_RAM_DISK_LAST_LBA;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_block_length    =  512;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_type            =  0;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_removable_flag  =  0x80;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_read            =  demo_thread_media_read2;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_write           =  demo_thread_media_write2; 
    global_storage_parameter.ux_slave_class_storage_parameter_lun[1].ux_slave_class_storage_media_status          =  demo_thread_media_status2;

    /* Initilize the device storage class. The class is connected with interface 0 on configuration 1. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry, 
                                                1, 0, (VOID *)&global_storage_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("Running Multiple LUN Storage Test................................... ERROR #6\n");
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running Multiple LUN Storage Test................................... ERROR #7\n");
        test_control_return(1);
    }

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(ux_test_system_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("Running Multiple LUN Storage Test................................... ERROR #2\n");
        test_control_return(1);
    }
    
    /* Register storage class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_storage_name, ux_host_class_storage_entry);
    if (status != UX_SUCCESS)
    {

        printf("Running Multiple LUN Storage Test................................... ERROR #3\n");
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running Multiple LUN Storage Test................................... ERROR #4\n");
        test_control_return(1);
    }
    
    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,  
            stack_pointer, UX_DEMO_STACK_SIZE, 
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Running Multiple LUN Storage Test................................... ERROR #8\n");
        test_control_return(1);
    }
            
}

static UINT  host_storage_instance_get(void)
{

UINT                status;
UX_HOST_CLASS       *class;


    /* Find the main storage container */
    status =  ux_host_stack_class_get(_ux_system_host_class_storage_name, &class);
    if (status != UX_SUCCESS)
        return(status);

    /* We get the first instance of the storage device */
    do  
    {

        status =  ux_host_stack_class_instance_get(class, 0, (void **) &storage);
        tx_thread_sleep(10);
    } while (status != UX_SUCCESS);

    /* We still need to wait for the storage status to be live */
    while (storage -> ux_host_class_storage_state != UX_HOST_CLASS_INSTANCE_LIVE)
        tx_thread_sleep(10);

    /* We try to get the first media attached to the class container.  */
    while (class -> ux_host_class_media == UX_NULL)
    {

        tx_thread_sleep(10);
    }

    /* Setup media pointers.  */
    global_storage_media =  (UX_HOST_CLASS_STORAGE_MEDIA *) class -> ux_host_class_media;

#if !defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    media1 =  &global_storage_media -> ux_host_class_storage_media;
    media2 =  &(global_storage_media+1) -> ux_host_class_storage_media;
#else
    media1 = _ux_host_class_storage_media_fx_media(global_storage_media);
    media2 = _ux_host_class_storage_media_fx_media(global_storage_media+1);
#endif

    return(UX_SUCCESS);
}


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT        status;
FX_FILE     my_file;
ULONG       error_count = 0;
ULONG       total_length;
UCHAR       buffer_pattern;


    /* Inform user.  */
    printf("Running Multiple LUN Storage Test................................... ");

    /* Format the ram drive 1. */
    status =  fx_media_format(&ram_disk1, _fx_ram_driver, ram_disk_memory1, buffer, 512, "RAM DISK", 2, 512, 0, UX_RAM_DISK_SIZE/512, 512, 4, 1, 1);

    /* Check the media format status.  */
    if (status != FX_SUCCESS)
    {

        /* Multiple LUN Storage test error.  */
        printf("ERROR #9\n");
        test_control_return(1);
    }
    
    /* Format the ram drive 2. */
    status =  fx_media_format(&ram_disk2, _fx_ram_driver, ram_disk_memory2, buffer, 512, "RAM DISK", 2, 512, 0, UX_RAM_DISK_SIZE/512, 512, 4, 1, 1);

    /* Check the media format status.  */
    if (status != FX_SUCCESS)
    {

        /* Multiple LUN Storage test error.  */
        printf("ERROR #10\n");
        test_control_return(1);
    }
    

    /* Open the ram_disk1 .  */
    status =  fx_media_open(&ram_disk1, "RAM DISK", _fx_ram_driver, ram_disk_memory1, buffer, 512);

    /* Check the media open status.  */
    if (status != FX_SUCCESS)
    {

        /* Multiple LUN Storage test error.  */
        printf("ERROR #11\n");
        test_control_return(1);
    }
    
    /* Open the ram_disk2 .  */
    status =  fx_media_open(&ram_disk2, "RAM DISK", _fx_ram_driver, ram_disk_memory2, buffer, 512);

    /* Check the media open status.  */
    if (status != FX_SUCCESS)
    {

        /* Multiple LUN Storage test error.  */
        printf("ERROR #12\n");
        test_control_return(1);
    }
    

    /* Find the storage class */
    status =  host_storage_instance_get();
    if (status != UX_SUCCESS)
    {

        /* Multiple LUN Storage test error.  */
        printf("ERROR #13\n");
        test_control_return(1);
    }
    
    /* Medfia 1.  */            
    /* Delete the target file.  */
    status =  fx_file_delete(media1, "FILE.CPY");

    /* Create the file.  */
    status =  fx_file_create(media1, "FILE.CPY");
    if (status != UX_SUCCESS)
    {

        /* Multiple LUN Storage test error.  */
        printf("ERROR #14\n");
        test_control_return(1);
    }

    memset(&my_file, 0, sizeof(my_file));

    /* Open file for copying.  */
    status =  fx_file_open(media1, &my_file, "FILE.CPY", FX_OPEN_FOR_WRITE);
    if (status != UX_SUCCESS)
    {

        /* Multiple LUN Storage test error.  */
        printf("ERROR #15\n");
        test_control_return(1);
    }

    /* Seek to the beginning to copy over an existing file.  */
    fx_file_seek(&my_file, 0);
    
    /* Set the file length.  */
    total_length = UX_DEMO_FILE_SIZE;
    
    /* Set pattern first letter.  */
    buffer_pattern = 'a';
    
    while(total_length !=0)
    {
    
        /* Set the buffer with pattern.  */
        ux_utility_memory_set(global_buffer, buffer_pattern, UX_DEMO_FILE_BUFFER_SIZE); 

        /* Copy the file in blocks */
        status = fx_file_write(&my_file, global_buffer, UX_DEMO_FILE_BUFFER_SIZE);

        /* Check if status OK.  */
        if (status != UX_SUCCESS)
        {

            /* Multiple LUN Storage test error.  */
            printf("ERROR #16\n");
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
    /* Finished reading file either at the end or because of error. */
    fx_file_close(&my_file);
    
    /* Media 2.  */            
    /* Delete the target file.  */
    status =  fx_file_delete(media2, "FILE.CPY");

    /* Create the file.  */
    status =  fx_file_create(media2, "FILE.CPY");
    if (status != UX_SUCCESS)
    {

        /* Multiple LUN Storage test error.  */
        printf("ERROR #17\n");
        test_control_return(1);
    }

    /* Open file for copying.  */
    status =  fx_file_open(media2, &my_file, "FILE.CPY", FX_OPEN_FOR_WRITE);
    if (status != UX_SUCCESS)
    {

        /* Multiple LUN Storage test error.  */
        printf("ERROR #18\n");
        test_control_return(1);
    }

    /* Seek to the beginning to copy over an existing file.  */
    fx_file_seek(&my_file, 0);
    
    /* Set the file length.  */
    total_length = UX_DEMO_FILE_SIZE;
    
    /* Set pattern first letter.  */
    buffer_pattern = 'a';
    
    while(total_length !=0)
    {
    
        /* Set the buffer with pattern.  */
        ux_utility_memory_set(global_buffer, buffer_pattern, UX_DEMO_FILE_BUFFER_SIZE); 

        /* Copy the file in blocks */
        status = fx_file_write(&my_file, global_buffer, UX_DEMO_FILE_BUFFER_SIZE);

        /* Check if status OK.  */
        if (status != UX_SUCCESS)
        {

            /* Multiple LUN Storage test error.  */
            printf("ERROR #19\n");
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
    /* Finished reading file either at the end or because of error. */
    fx_file_close(&my_file);

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}


static UINT    demo_thread_media_status1(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{

    /* The ATA drive never fails. This is just for demo only !!!! */
    return(UX_SUCCESS);
}

static UINT    demo_thread_media_read1(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT status;

    if(lba == 0)
    {
        ram_disk1.fx_media_driver_logical_sector =  0;
        ram_disk1.fx_media_driver_sectors =  1;
        ram_disk1.fx_media_driver_request =  FX_DRIVER_BOOT_READ;
        ram_disk1.fx_media_driver_buffer =  data_pointer;
        _fx_ram_driver(&ram_disk1);
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


        status = ram_disk1.fx_media_driver_status;
    }        
    else
    {        
        while(number_blocks--)
        {
            status =  fx_media_read(&ram_disk1,lba,data_pointer);
            data_pointer+=512;
            lba++;
        }
    }        
    return(status);
}

static UINT    demo_thread_media_write1(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT status;

    if(lba == 0)
    {
        ram_disk1.fx_media_driver_logical_sector =  0;
        ram_disk1.fx_media_driver_sectors =  1;
        ram_disk1.fx_media_driver_request =  FX_DRIVER_BOOT_WRITE;
        ram_disk1.fx_media_driver_buffer =  data_pointer;
        _fx_ram_driver(&ram_disk1);

        status = ram_disk1.fx_media_driver_status;

    }        
    else
    {

        while(number_blocks--)
        {
            status =  fx_media_write(&ram_disk1,lba,data_pointer);
            data_pointer+=512;
            lba++;
        }        
        return(status);
    }
    return(1);
}




static UINT    demo_thread_media_status2(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{

    /* The ATA drive never fails. This is just for demo only !!!! */
    return(UX_SUCCESS);
}

static UINT    demo_thread_media_read2(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT status;

    if(lba == 0)
    {
        ram_disk2.fx_media_driver_logical_sector =  0;
        ram_disk2.fx_media_driver_sectors =  1;
        ram_disk2.fx_media_driver_request =  FX_DRIVER_BOOT_READ;
        ram_disk2.fx_media_driver_buffer =  data_pointer;
        _fx_ram_driver(&ram_disk2);
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


        status = ram_disk2.fx_media_driver_status;
    }        
    else
    {        
        while(number_blocks--)
        {
            status =  fx_media_read(&ram_disk2,lba,data_pointer);
            data_pointer+=512;
            lba++;
        }
    }        
    return(status);
}

static UINT    demo_thread_media_write2(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT status;

    if(lba == 0)
    {
        ram_disk2.fx_media_driver_logical_sector =  0;
        ram_disk2.fx_media_driver_sectors =  1;
        ram_disk2.fx_media_driver_request =  FX_DRIVER_BOOT_WRITE;
        ram_disk2.fx_media_driver_buffer =  data_pointer;
        _fx_ram_driver(&ram_disk2);

        status = ram_disk2.fx_media_driver_status;

    }        
    else
    {

        while(number_blocks--)
        {
            status =  fx_media_write(&ram_disk2,lba,data_pointer);
            data_pointer+=512;
            lba++;
        }        
        return(status);
    }
    return(1);
}

