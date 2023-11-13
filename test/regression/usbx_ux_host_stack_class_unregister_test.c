/* This test is designed to test the ux_utility_memory_....  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_device_class_dpump.h"
#include "ux_host_class_dpump.h"

#include "fx_api.h"
#include "ux_device_class_storage.h"
#include "ux_host_class_storage.h"

#include "ux_device_class_hid.h"
#include "ux_host_class_hid.h"
#include "ux_host_class_hid_mouse.h"

#include "ux_host_stack.h"
#include "ux_hcd_sim_host.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_dcd_sim_slave.h"
#include "ux_test_utility_sim.h"


/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE              4096
#define UX_TEST_MEMORY_SIZE             (256*1024)

#define UX_RAM_DISK_SIZE                (200 * 1024)
#define UX_RAM_DISK_LAST_LBA            ((UX_RAM_DISK_SIZE / 512) -1)

#define     LSB(x) ( (x) & 0x00ff)
#define     MSB(x) (((x) & 0xff00) >> 8)

/* Define the counters used in the test application...  */

static ULONG                            thread_0_counter = 0;
static ULONG                            thread_1_counter = 0;

static ULONG                            error_counter = 0;

static UCHAR                            error_callback_ignore = UX_FALSE;
static ULONG                            error_callback_counter = 0;

static UCHAR                            bad_name[UX_MAX_HCD_NAME_LENGTH + 1];

static FX_MEDIA                         ram_disk1 = {0};
static CHAR                             ram_disk_memory1[UX_RAM_DISK_SIZE];
static UCHAR                            ram_disk_buffer1[512];

static FX_MEDIA                         *ram_disks[] = {&ram_disk1};
static UCHAR                            *ram_disk_buffers[] = {ram_disk_buffer1};
static CHAR                             *ram_disk_memories[] = {ram_disk_memory1};

static UX_SLAVE_CLASS_DPUMP_PARAMETER       dpump_parameter = {0};
static UX_SLAVE_CLASS_STORAGE_PARAMETER     storage_parameter = {0};
static UX_SLAVE_CLASS_HID_PARAMETER         hid_parameter = {0};

UX_HOST_CLASS_DPUMP                         *host_dpump = UX_NULL;
UX_HOST_CLASS_STORAGE                       *host_storage = UX_NULL;
UX_HOST_CLASS_HID                           *host_hid = UX_NULL;


/* Define USBX test global variables.  */

static UCHAR hid_mouse_report[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0x09, 0x38,                    //     USAGE (Mouse Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};
#define HID_MOUSE_REPORT_LENGTH (sizeof(hid_mouse_report)/sizeof(hid_mouse_report[0]))


#define CONFIG_DESCRIPTOR(wTotalLength, n_iface) \
    0x09, 0x02, LSB(wTotalLength), MSB(wTotalLength), (n_iface), 0x01, 0x00, 0xc0, 0x32,

#define DPUMP_DESCRIPTORS_LENGTH                                (9+7+7)
#define DPUMP_DESCRIPTORS(iface, ep_out, ep_in, mps)            \
    /* Interface descriptor */                                  \
    0x09, 0x04, (iface), 0x00, 0x02, 0x99, 0x99, 0x99, 0x00,    \
    /* Endpoint descriptor (Bulk Out) */                        \
    0x07, 0x05, (ep_out), 0x02, LSB(mps), MSB(mps), 0x00,       \
    /* Endpoint descriptor (Bulk In) */                         \
    0x07, 0x05, (ep_in), 0x02, LSB(mps), MSB(mps), 0x00,

#define STORAGE_DESCRIPTORS_LENGTH                              (9+7+7)
#define STORAGE_DESCRIPTORS(iface, ep_out, ep_in, mps)          \
    /* Interface descriptor */                                  \
    0x09, 0x04, (iface), 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,    \
    /* Endpoint descriptor (Bulk In) */                         \
    0x07, 0x05, (ep_in), 0x02, 0x00, 0x01, 0x00,                \
    /* Endpoint descriptor (Bulk Out) */                        \
    0x07, 0x05, (ep_out), 0x02, 0x00, 0x01, 0x00,

#define HID_DESCRIPTORS_LENGTH                                  (9+9+7)
#define HID_DESCRIPTORS(iface, ep_in, mps, interval, rpt_len)   \
    /* Interface descriptor */                                  \
    0x09, 0x04, (iface), 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,    \
    /* HID descriptor */                                        \
    0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22,                   \
    LSB(rpt_len), MSB(rpt_len),                                 \
    /* Endpoint descriptor (Interrupt) */                       \
    0x07, 0x05, (ep_in), 0x03, LSB(mps), MSB(mps), (interval),


static UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
    0xec, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01,

    CONFIG_DESCRIPTOR(DPUMP_DESCRIPTORS_LENGTH +
                      STORAGE_DESCRIPTORS_LENGTH +
                      HID_DESCRIPTORS_LENGTH + 9, 0x03)
    DPUMP_DESCRIPTORS(0x00, 0x01, 0x82, 64)
    STORAGE_DESCRIPTORS(0x01, 0x03, 0x84, 64)
    HID_DESCRIPTORS(0x02, 0x85, 8, 8, HID_MOUSE_REPORT_LENGTH)
};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)


static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
    0x03, 0x01,

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x01, 0x00,

    CONFIG_DESCRIPTOR(DPUMP_DESCRIPTORS_LENGTH +
                      STORAGE_DESCRIPTORS_LENGTH +
                      HID_DESCRIPTORS_LENGTH + 9, 0x03)
    DPUMP_DESCRIPTORS(0x00, 0x01, 0x82, 512)
    STORAGE_DESCRIPTORS(0x01, 0x03, 0x84, 512)
    HID_DESCRIPTORS(0x02, 0x85, 8, 8, HID_MOUSE_REPORT_LENGTH)
};
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)

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
    0x09, 0x04, 0x02, 0x0c,
    0x44, 0x61, 0x74, 0x61, 0x50, 0x75, 0x6d, 0x70,
    0x44, 0x65, 0x6d, 0x6f,

    /* Serial Number string descriptor : Index 3 */
    0x09, 0x04, 0x03, 0x04,
    0x30, 0x30, 0x30, 0x31
};


/* Multiple languages are supported on the device, to add
    a language besides English, the unicode language code must
    be appended to the language_id_framework array and the length
    adjusted accordingly. */
#define LANGUAGE_ID_FRAMEWORK_LENGTH 2
static UCHAR language_id_framework[] = {

    /* English. */
        0x09, 0x04
};


/* Define prototypes for external Controller's (HCD/DCDs), classes and clients.  */

extern VOID                 _fx_ram_driver(FX_MEDIA *media_ptr);

static TX_THREAD            ux_test_thread_simulation_0;
static void                 ux_test_thread_simulation_0_entry(ULONG);

static TX_THREAD            ux_test_thread_simulation_1;
static void                 ux_test_thread_simulation_1_entry(ULONG);

static UX_SLAVE_CLASS_DPUMP *dpump_device = UX_NULL;
static VOID                 ux_test_dpump_instance_activate(VOID *dpump_instance);
static VOID                 ux_test_dpump_instance_deactivate(VOID *dpump_instance);

static UINT                 ux_test_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                 ux_test_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                 ux_test_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status);

/* Prototype for test control return.  */

void  test_control_return(UINT status);

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        {
            /* Failed test.  */
            printf("#%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
        }
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_stack_class_unregister_test_application_define(void *first_unused_memory)
#endif
{

UINT                                status;
UCHAR                               *stack_pointer;
UCHAR                               *memory_pointer;


    /* Inform user.  */
    printf("Running ux_host_stack_class_unregister Test......................... ");

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

#if UX_TEST_MULTI_CLS_OVER(2) && UX_TEST_MULTI_IFC_OVER(2) /* At least 3 classes used.  */

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);
#endif

    /* The code below is required for installing the device portion of USBX */
    status = ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH, UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Set the parameters for callback when insertion/extraction of a Data Pump device.  */
    dpump_parameter.ux_slave_class_dpump_instance_activate   =  ux_test_dpump_instance_activate;
    dpump_parameter.ux_slave_class_dpump_instance_deactivate =  ux_test_dpump_instance_deactivate;

    /* Initialize the device dpump class. The class is connected with interface 0 */
    status = ux_device_stack_class_register(_ux_system_slave_class_dpump_name, _ux_device_class_dpump_entry,
                                            1, 0, &dpump_parameter);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Store the number of LUN in this device storage instance.  */
    storage_parameter.ux_slave_class_storage_parameter_number_lun = 1;

    /* Initialize the storage class parameters for reading/writing to the first Flash Disk.  */
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_last_lba        =  UX_RAM_DISK_LAST_LBA;
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_block_length    =  512;
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_type            =  0;
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_removable_flag  =  0x80;
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read            =  ux_test_media_read;
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write           =  ux_test_media_write;
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status          =  ux_test_media_status;

    /* Initialize the device storage class. The class is connected with interface 0 on configuration 1. */
    status = ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry,
                                            1, 1, (VOID *)&storage_parameter);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
#endif

    /* Initialize the hid class parameters for a mouse.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_mouse_report;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_MOUSE_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = UX_NULL;

    /* Initilize the device hid class. The class is connected with interface 2 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                             1, 2, (VOID *)&hid_parameter);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
#endif

    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_simulation_0, "test simulation", ux_test_thread_simulation_0_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}

static void ux_test_ram_disk_initialize(INT line)
{
UINT        status;

    /* Initialize FileX.  */
    fx_system_initialize();

    /* Reset ram disks memory.  */
    ux_utility_memory_set(ram_disk_memory1, 0, UX_RAM_DISK_SIZE);

    /* Format the ram drive. */
    status =   fx_media_format(&ram_disk1, _fx_ram_driver, ram_disk_memory1, ram_disk_buffer1, 512, "RAM DISK1", 2, 512, 0, UX_RAM_DISK_SIZE/512, 512, 4, 1, 1);
    if (status != FX_SUCCESS)
    {
        printf("ERROR #%d.%d: 0x%x\n", line, __LINE__, status);
        test_control_return(1);
    }

    /* Open the ram_disk.  */
    status =   fx_media_open(&ram_disk1, "RAM DISK1", _fx_ram_driver, ram_disk_memory1, ram_disk_buffer1, 512);
    if (status != FX_SUCCESS)
    {
        printf("ERROR #%d.%d: 0x%x\n", line, __LINE__, status);
        test_control_return(1);
    }

}

static void ux_test_host_stack_class_register(INT line)
{

UINT        status;

    // printf("#%d.%d: register classes\n", line, __LINE__);

    status = ux_host_stack_class_register(_ux_system_host_class_dpump_name, ux_host_class_dpump_entry);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: 0x%x\n", line, __LINE__, status);
        test_control_return(1);
    }

#if UX_MAX_CLASS_DRIVER == 1
    error_callback_ignore = UX_TRUE;
#endif

    status = ux_host_stack_class_register(_ux_system_host_class_storage_name, ux_host_class_storage_entry);
#if UX_MAX_CLASS_DRIVER > 1
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: 0x%x\n", line, __LINE__, status);
        test_control_return(1);
    }
#endif

    status = ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
#if UX_MAX_CLASS_DRIVER > 1
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: 0x%x\n", line, __LINE__, status);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_mouse_name, ux_host_class_hid_mouse_entry);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: 0x%x\n", line, __LINE__, status);
        test_control_return(1);
    }
#endif

#if UX_MAX_CLASS_DRIVER == 1
    error_callback_ignore = UX_FALSE;
#endif
}

static void ux_test_host_stack_class_unregister(INT line)
{

UINT        status;

    // printf("#%d.%d: unregister classes\n", line, __LINE__);

    /* Dpump has no "destroy", ignore UX_FUNCTION_NOT_SUPPORTED print.  */
    error_callback_ignore = UX_TRUE;
    status = _ux_host_stack_class_unregister(ux_host_class_dpump_entry);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: 0x%x\n", line, __LINE__, status);
        test_control_return(1);
    }
    error_callback_ignore = UX_FALSE;

    status = _ux_host_stack_class_unregister(ux_host_class_storage_entry);
#if UX_MAX_CLASS_DRIVER > 1
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: 0x%x\n", line, __LINE__, status);
        test_control_return(1);
    }
#endif

    status = _ux_host_stack_class_unregister(ux_host_class_hid_entry);
#if UX_MAX_CLASS_DRIVER > 1
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: 0x%x\n", line, __LINE__, status);
        test_control_return(1);
    }
#endif
}

static void ux_test_host_stack_hcd_register(INT line)
{
UINT        status;

    /* Register HCD.  */
    status = _ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d: 0x%x\n", line, __LINE__, status);
        test_control_return(1);
    }
}

static void ux_test_host_stack_hcd_unregister(INT line)
{
UINT                status;
UX_DEVICE           *device;

    /* Unregister HCD.  */
    status = _ux_host_stack_hcd_unregister(_ux_system_host_hcd_simulator_name, 0, 0);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Check host device.  */
    status = _ux_host_stack_device_get(0, &device);
    if (status != UX_DEVICE_HANDLE_UNKNOWN)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}

static void ux_test_wait_connect(INT line)
{

UINT                        status;
INT                         i;
UX_DEVICE                   *device;
UX_HOST_CLASS               *host_class;

    host_dpump = UX_NULL;
    host_storage = UX_NULL;
    host_hid = UX_NULL;

    /* Wait for connection.  */
    for (i = 0; i < 100; i ++)
    {
        tx_thread_sleep(UX_MS_TO_TICK_NON_ZERO(10));
        status = _ux_host_stack_device_get(0, &device);
        if (status != UX_SUCCESS)
            continue;
        if (dpump_device == UX_NULL)
            continue;

        /* DPUMP.  */
        ux_host_stack_class_get(_ux_system_host_class_dpump_name, &host_class);
        status = ux_host_stack_class_instance_get(host_class, 0, (VOID **)&host_dpump);
        if (status != UX_SUCCESS)
            continue;
        if (host_dpump -> ux_host_class_dpump_state != UX_HOST_CLASS_INSTANCE_LIVE)
            continue;

        /* Storage.  */
        ux_host_stack_class_get(_ux_system_host_class_storage_name, &host_class);
        status = ux_host_stack_class_instance_get(host_class, 0, (VOID **)&host_storage);
        if (status != UX_SUCCESS)
            continue;
        if (host_storage -> ux_host_class_storage_state != UX_HOST_CLASS_INSTANCE_LIVE)
            continue;

        /* HID.  */
        ux_host_stack_class_get(_ux_system_host_class_hid_name, &host_class);
        status = ux_host_stack_class_instance_get(host_class, 0, (VOID **)&host_hid);
        if (status != UX_SUCCESS)
            continue;
        if (host_hid -> ux_host_class_hid_state != UX_HOST_CLASS_INSTANCE_LIVE)
            continue;

        /* All ready.  */
        break;
    }
    if (dpump_device == UX_NULL)
    {
        printf("ERROR #%d.%d\n", line, __LINE__);
        test_control_return(1);
    }
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d.%d\n", line, __LINE__);
        test_control_return(1);
    }
}

static void  ux_test_thread_simulation_0_entry(ULONG arg)
{

UINT                status;
ULONG               rfree;
INT                 i;
UX_DEVICE           *device;

    /* Initialize RAM disk.  */
    ux_test_ram_disk_initialize(__LINE__);

    /* Initialize host stack.  */
    status = ux_host_stack_initialize(UX_NULL);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /************************** Register & unregister (no device connected).  */

    /* Log memory level.  */
    rfree = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    ux_test_host_stack_class_register(__LINE__);
    ux_test_host_stack_class_unregister(__LINE__);

    /* Check memory level.  */
    if (rfree != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
    {
        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /************************** Unregister (after device connect & disconnect).  */

    error_callback_ignore = UX_TRUE;

    // ux_test_dcd_sim_slave_disconnect();
    // ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);

    ux_test_host_stack_class_register(__LINE__);
    ux_test_host_stack_hcd_register(__LINE__);
    ux_test_wait_connect(__LINE__);

    /* Unregister.  */
    error_callback_ignore = UX_TRUE;
    ux_test_host_stack_hcd_unregister(__LINE__);
    error_callback_ignore = UX_FALSE;

    ux_test_host_stack_hcd_register(__LINE__);
    ux_test_wait_connect(__LINE__);

    /* Unregister.  */
    error_callback_ignore = UX_TRUE;
    ux_test_host_stack_hcd_unregister(__LINE__);
    ux_test_host_stack_class_unregister(__LINE__);
    error_callback_ignore = UX_FALSE;

    // ux_test_dcd_sim_slave_disconnect();
    // ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);

    /* Log memory level.  */
    rfree = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    ux_test_host_stack_class_register(__LINE__);
    ux_test_host_stack_hcd_register(__LINE__);
    ux_test_wait_connect(__LINE__);

    /* Unregister.  */
    error_callback_ignore = UX_TRUE;
    ux_test_host_stack_hcd_unregister(__LINE__);
    ux_test_host_stack_class_unregister(__LINE__);

    /* Check memory level.  */
    if (rfree != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
    {
        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
    error_callback_ignore = UX_FALSE;

    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* Test error.  */
        printf("ERROR #%d: total %ld errors\n", __LINE__, error_counter);
        test_control_return(1);
    }
    else
    {

        /* Successful test.  */
        printf("SUCCESS!\n");
        test_control_return(0);
    }
}


static VOID  ux_test_dpump_instance_activate(VOID *dpump_instance)
{

    /* Save the DPUMP instance.  */
    dpump_device = (UX_SLAVE_CLASS_DPUMP *) dpump_instance;
}

static VOID  ux_test_dpump_instance_deactivate(VOID *dpump_instance)
{

    /* Reset the DPUMP instance.  */
    dpump_device = UX_NULL;
}

static UINT ux_test_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{

static UCHAR lun_init_done[2] = {0, 0};
UINT         status;
ULONG        mstatus = UX_SLAVE_CLASS_STORAGE_SENSE_KEY_NO_SENSE;


    (void)storage;
    (void)media_id;


    if (lun > 0)
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

UINT ux_test_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
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

UINT ux_test_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
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
    return(status);
}
