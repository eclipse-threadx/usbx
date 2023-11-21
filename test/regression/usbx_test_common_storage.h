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

#include "usbx_test_common.h"

/* Define constants.  */
#define                             UX_DEMO_STACK_SIZE              2048
#define                             UX_DEMO_BUFFER_SIZE             2048
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE   512
#define                             UX_DEMO_FILE_BUFFER_SIZE        512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE    64
#define                             UX_DEMO_MEMORY_SIZE             (256 * 1024)
#define                             UX_DEMO_FILE_SIZE               (128 * 1024)
#define                             UX_RAM_DISK_SIZE                (200 * 1024)
#define                             UX_RAM_DISK_LAST_LBA            ((UX_RAM_DISK_SIZE / 512) -1)

/* Define local/extern function prototypes.  */
static void                                       demo_thread_entry(ULONG);
static TX_THREAD                                  tx_demo_thread_host_simulation;
static TX_THREAD                                  tx_demo_thread_slave_simulation;
static void                                       tx_demo_thread_host_simulation_entry(ULONG);


VOID                                              _fx_ram_driver(FX_MEDIA *media_ptr);
static UINT                                       demo_device_state_change(ULONG event);
static void                                       demo_thread_entry(ULONG);
static UINT                                       default_device_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                                       default_device_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                                       default_device_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status);


/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static ULONG                               error_counter;
static TX_THREAD                           demo_thread;
static UX_HOST_CLASS_STORAGE               *global_storage;
static UX_HOST_CLASS_STORAGE_MEDIA         *global_storage_media;
static FX_MEDIA                            *global_media;
static UINT                                status;
static UINT                                transfer_completed;
static ULONG                               requested_length;
static FX_FILE                             my_file;
static TX_SEMAPHORE                        demo_semaphore;
static CHAR                                file_name[64];
static UCHAR                               global_buffer[UX_DEMO_FILE_BUFFER_SIZE];
static UCHAR                               global_ram_disk_working_buffer[512];
static FX_MEDIA                            global_ram_disk;
static CHAR                                global_ram_disk_memory[UX_RAM_DISK_SIZE];
static UX_SLAVE_CLASS_STORAGE_PARAMETER    global_storage_parameter;
static TX_SEMAPHORE                        storage_instance_live_semaphore;



/* Prototype for test control return.  */

void  test_control_return(UINT status);

/* Prototype for _ux_hcd_sim_host_entry */
UINT _ux_hcd_sim_host_entry(UX_HCD *, UINT, VOID *);

static UINT  host_storage_instance_get(void)
{

UINT                status;
UX_HOST_CLASS       *host_class;


    /* Find the main storage container */
    status =  ux_host_stack_class_get(_ux_system_host_class_storage_name, &host_class);
    if (status != UX_SUCCESS)
        return(status);

    status = ux_utility_semaphore_get(&storage_instance_live_semaphore, 5000);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status =  ux_host_stack_class_instance_get(host_class, 0, (void **) &global_storage);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    if (global_storage -> ux_host_class_storage_state != UX_HOST_CLASS_INSTANCE_LIVE)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    if (host_class -> ux_host_class_media == UX_NULL)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Setup media pointer.  */
    global_storage_media =  (UX_HOST_CLASS_STORAGE_MEDIA *) host_class -> ux_host_class_media;
    global_media =  &global_storage_media -> ux_host_class_storage_media;

    if (global_media == UX_NULL)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }
    
    return(UX_SUCCESS);
}
