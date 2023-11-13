/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "fx_api.h"

#include "ux_device_class_cdc_acm.h"
#include "ux_device_class_storage.h"
#include "ux_device_stack.h"

#include "ux_host_class_cdc_acm.h"
#include "ux_host_class_storage.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

#include "ux_host_stack.h"

/* Define constants.  */
#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024
#define                             UX_DEMO_BUFFER_SIZE (UX_SLAVE_REQUEST_DATA_MAX_LENGTH + 1)
#define                             UX_DEMO_XMIT_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE 512
#define                             UX_DEMO_FILE_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE 64
#define                             UX_DEMO_MEMORY_SIZE     (80*1024)
#define                             UX_DEMO_FILE_SIZE       (128 * 1024)
#define                             UX_RAM_DISK_SIZE        (256 * 1024)
#define                             UX_TEST_MAX_SECTOR_SIZE  512

/* Storage test strings. */

UCHAR test_ux_system_slave_class_storage_vendor_id[] =      "Test VID";
UCHAR test_ux_system_slave_class_storage_product_id[] =     "UX Storage DEV  ";
UCHAR test_ux_system_slave_class_storage_product_rev[] =    "1010";
UCHAR test_ux_system_slave_class_storage_product_serial[] = "01234567890123456789";

/* HID reports.  */

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
#define HID_MOUSE_REPORT_LENGTH sizeof(hid_mouse_report)

static UCHAR hid_keyboard_report[] = { 

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)

    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)

    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)

    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)

    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)

    0xc0                           // END_COLLECTION
};
#define HID_KEYBOARD_REPORT_LENGTH sizeof(hid_keyboard_report)

#define     LSB(x) ( (x) & 0x00ff)
#define     MSB(x) (((x) & 0xff00) >> 8)

/* Configuration descriptor 9 bytes */
#define CFG_DESC(wTotalLength, bNumInterfaces, bConfigurationValue)\
    /* Configuration 1 descriptor 9 bytes */\
    0x09, 0x02, LSB(wTotalLength), MSB(wTotalLength),\
    (bNumInterfaces), (bConfigurationValue), 0x00,\
    0x40, 0x00,
#define CFG_DESC_LEN 9

#define IAD_DESC(bIfc) \
    /* Interface association descriptor. 8 bytes.  */\
    0x08, 0x0b, (bIfc), 0x02, 0x02, 0x02, 0x00, 0x00,
#define IAD_DESC_LEN 8

#define CDC_IFC_DESC_ALL(bIfc, bIntIn, bBulkIn, bBulkOut)\
    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */\
    0x09, 0x04, (bIfc), 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,\
    /* Header Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x00, 0x10, 0x01,\
    /* ACM Functional Descriptor 4 bytes */\
    0x04, 0x24, 0x02, 0x0f,\
    /* Union Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x06, (bIfc), (bIfc + 1),\
    /* Call Management Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x01, 0x03, (bIfc + 1),\
    /* Endpoint interrupt in descriptor 7 bytes */\
    0x07, 0x05, (bIntIn), 0x03, 0x40, 0x00, 0x10,\
    /* Data Class Interface Descriptor Requirement 9 bytes */\
    0x09, 0x04, (bIfc + 1), 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,\
    /* Endpoint bulk in descriptor 7 bytes */\
    0x07, 0x05, (bBulkIn), 0x02, 0x40, 0x00, 0x01,\
    /* Endpoint bulk out descriptor 7 bytes */\
    0x07, 0x05, (bBulkOut), 0x02, 0x40, 0x00, 0x01,
#define CDC_IFC_DESC_ALL_LEN (9+5+4+5+5+7+ 9+7+7)

#define STORAGE_IFC_DESC_ALL(bIfc, bBulkIn, bBulkOut)\
    /* Interface descriptor */\
    0x09, 0x04, (bIfc), 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,\
    /* Endpoint descriptor (Bulk In) */\
    0x07, 0x05, (bBulkIn), 0x02, 0x40, 0x00, 0x00,\
    /* Endpoint descriptor (Bulk Out) */\
    0x07, 0x05, (bBulkOut), 0x02, 0x40, 0x00, 0x00,
#define STORAGE_IFC_DESC_ALL_LEN (9+7+7)

#define HID_MOUSE_IFC_DESC_ALL(bIfc, bInterruptIn)\
    /* Interface descriptor */\
    0x09, 0x04, (bIfc), 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,\
    /* HID descriptor */\
    0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_MOUSE_REPORT_LENGTH), MSB(HID_MOUSE_REPORT_LENGTH),\
    /* Endpoint descriptor (Interrupt) */\
    0x07, 0x05, (bInterruptIn), 0x03, 0x08, 0x00, 0x08
#define HID_MOUSE_IFC_DESC_ALL_LEN (9+9+7)

#define HID_KEYBOARD_IFC_DESC_ALL(bIfc, bInterruptIn)\
    /* Interface descriptor */\
    0x09, 0x04, (bIfc), 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,\
    /* HID descriptor */\
    0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_KEYBOARD_REPORT_LENGTH), MSB(HID_KEYBOARD_REPORT_LENGTH),\
    /* Endpoint descriptor (Interrupt) */\
    0x07, 0x05, (bInterruptIn), 0x03, 0x08, 0x00, 0x08
#define HID_KEYBOARD_IFC_DESC_ALL_LEN (9+9+7)

typedef struct TEST_DEVICE_CLASS_INFO_STRUCT
{
    UCHAR *class_name;
    UINT  (*class_entry)(UX_SLAVE_CLASS_COMMAND *command);
    ULONG class_interface_nb;
    VOID  *class_parameter;
} TEST_DEVICE_CLASS_INFO;

typedef struct TEST_DEVICE_FRAMEWORK_INFO_STRUCT
{
    UCHAR *framework;
    ULONG  length;
} TEST_DEVICE_FRAMEWORK_INFO;

typedef struct TEST_DEVICE_INFO_STRUCT
{
    UINT (*slave_change_function)(ULONG);
    TEST_DEVICE_FRAMEWORK_INFO *framework_high_speed;
    TEST_DEVICE_FRAMEWORK_INFO *framework_full_speed;
    TEST_DEVICE_FRAMEWORK_INFO *framework_string;
    TEST_DEVICE_FRAMEWORK_INFO *framework_language;
    TEST_DEVICE_CLASS_INFO *classes;
    ULONG                   nb_classes;
} TEST_DEVICE_INFO;

/* Define local/extern function prototypes.  */
static VOID                                test_thread_entry(ULONG);
static TX_THREAD                           tx_test_thread_host_simulation;
static TX_THREAD                           tx_test_thread_slave_simulation;
static VOID                                tx_test_thread_simulation_entry(ULONG);

static VOID                                test_slave_instance_activate(VOID  *cdc_instance);
static VOID                                test_slave_instance_deactivate(VOID *cdc_instance);

VOID                                        _fx_ram_driver(FX_MEDIA *media_ptr);
static UINT                                 default_device_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                                 default_device_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
static UINT                                 default_device_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status);

/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

static UX_HOST_CLASS_STORAGE               *storage;
static UX_HOST_CLASS_STORAGE_MEDIA         *storage_media;
static UX_SLAVE_CLASS_STORAGE_PARAMETER    storage_parameter;

static UCHAR                               inquiry_response[UX_HOST_CLASS_STORAGE_INQUIRY_RESPONSE_LENGTH];

static UX_SLAVE_CLASS_STORAGE              *storage_slave;

static FX_MEDIA                            ram_disk;
static UCHAR                               ram_disk_memory[UX_RAM_DISK_SIZE];
static UCHAR                               ram_disk_working_buffer[UX_TEST_MAX_SECTOR_SIZE];

static ULONG                               error_counter;

static ULONG                               error_callback_counter;
static UCHAR                               error_callback_ignore;

static UCHAR                               dcd_initialized = UX_FALSE;

/* Define device framework.  */

static unsigned char device_framework_full_speed[] = {

    /* Device descriptor     18 bytes
       0x02 bDeviceClass:    CDC class code
       0x00 bDeviceSubclass: CDC class sub code
       0x00 bDeviceProtocol: CDC Device protocol

       idVendor & idProduct - http://www.linux-usb.org/usb.ids
    */
    0x12, 0x01, 0x10, 0x01,
    0xEF, 0x02, 0x01,
    0x08,
    0x84, 0x84, 0x00, 0x00,
    0x00, 0x01,
    0x01, 0x02, 03,
    0x01, /* bNumConfigurations */

    /* Configuration 1 descriptor 9 bytes.  */
    CFG_DESC(CFG_DESC_LEN + STORAGE_IFC_DESC_ALL_LEN, 2, 1)
    /* MSC BO interfaces */
    STORAGE_IFC_DESC_ALL(0, 0x81, 0x02)
};
#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      sizeof(device_framework_full_speed)

static unsigned char device_framework_high_speed[] = {

    /* Device descriptor     18 bytes
       0x02 bDeviceClass:    CDC class code
       0x00 bDeviceSubclass: CDC class sub code
       0x00 bDeviceProtocol: CDC Device protocol

       idVendor & idProduct - http://www.linux-usb.org/usb.ids
    */
    0x12, 0x01, 0x00, 0x02,
    0xEF, 0x02, 0x01,
    0x40,
    0x84, 0x84, 0x00, 0x00,
    0x00, 0x01,
    0x01, 0x02, 03,
    0x01, /* bNumConfigurations */

    /* Device qualifier descriptor 10 bytes */
    0x0a, 0x06, 0x00, 0x02,
    0x02, 0x00, 0x00,
    0x40,
    0x01,
    0x00,

    /* Configuration 1 descriptor 9 bytes.  */
    CFG_DESC(CFG_DESC_LEN + STORAGE_IFC_DESC_ALL_LEN, 2, 1)
    /* MSC BO interfaces */
    STORAGE_IFC_DESC_ALL(0, 0x81, 0x02)
};
#define             DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED      sizeof(device_framework_high_speed)

static unsigned char string_framework[] = {

    /* Manufacturer string descriptor : Index 1 - "Express Logic" */
    0x09, 0x04, 0x01, 0x0c,
    0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
    0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 - "EL Composite device" */
    0x09, 0x04, 0x02, 0x13,
    0x45, 0x4c, 0x20, 0x43, 0x6f, 0x6d, 0x70, 0x6f,
    0x73, 0x69, 0x74, 0x65, 0x20, 0x64, 0x65, 0x76,
    0x69, 0x63, 0x65,

    /* Serial Number string descriptor : Index 3 - "0001" */
    0x09, 0x04, 0x03, 0x04,
    0x30, 0x30, 0x30, 0x31
};
#define             STRING_FRAMEWORK_LENGTH                 sizeof(string_framework)

/* Multiple languages are supported on the device, to add
    a language besides english, the unicode language code must
    be appended to the language_id_framework array and the length
    adjusted accordingly. */
static unsigned char language_id_framework[] = {

    /* English. */
    0x09, 0x04
};
#define             LANGUAGE_ID_FRAMEWORK_LENGTH            sizeof(language_id_framework)

/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}

static UINT break_on_storage_all_ready(VOID)
{
UINT                                 status;
UX_HOST_CLASS                       *class;

    /* Find the main cdc_acm container */
    status = ux_host_stack_class_get(_ux_system_host_class_storage_name, &class);
    if (status != UX_SUCCESS)
        /* Do not break. */
        return 0;

    /* Find class instances. */
    status = ux_host_stack_class_instance_get(class, 0, (void **) &storage);
    if (status != UX_SUCCESS)
        /* Do not break. */
        return 0;

    if (storage->ux_host_class_storage_state != UX_HOST_CLASS_INSTANCE_LIVE)
        /* Do not break. */
        return 0;

    if (storage_slave == UX_NULL)
        /* Do not break. */
        return 0;

    /* All found, break. */
    return 1;
}

static UINT break_on_removal(VOID)
{

UINT                     status;
UX_DEVICE               *device;

    status = ux_host_stack_device_get(0, &device);
    if (status == UX_SUCCESS)
        /* Do not break. */
        return UX_SUCCESS;

    storage = UX_NULL;

    return 1;
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_CDC_ACM *cdc_acm = (UX_HOST_CLASS_CDC_ACM *) inst;

    switch(event)
    {

        case UX_DEVICE_INSERTION:
            storage = (UX_HOST_CLASS_STORAGE *)inst;
            break;

        case UX_DEVICE_REMOVAL:
            storage = (UX_HOST_CLASS_STORAGE *)inst;
            break;

        default:
            break;
    }
    return 0;
}

static VOID    test_slave_instance_activate(VOID *instance)
{
    storage_slave = (UX_SLAVE_CLASS_STORAGE *)instance;
}
static VOID    test_slave_instance_deactivate(VOID *instance)
{
    storage_slave = UX_NULL;
}

static VOID test_ux_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        /* Exception: DCD Sim has no transfer abort. */
        if (system_context == UX_SYSTEM_CONTEXT_DCD && error_code == UX_FUNCTION_NOT_SUPPORTED)
            return;

        /* Failed test.  */
        printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
        // test_control_return(1);
    }
}

static UINT test_device_stack_storage_init(VOID)
{

UINT status;

    /* Reset ram disks memory.  */
    ux_utility_memory_set(ram_disk_memory, 0, UX_RAM_DISK_SIZE);

    /* Format the ram drive. */
    status =  fx_media_format(&ram_disk, _fx_ram_driver, ram_disk_memory, ram_disk_working_buffer, 512, "RAM DISK", 2, 512, 0, UX_RAM_DISK_SIZE/512, 512, 4, 1, 1);

    /* Check the media format status.  */
    if (status != FX_SUCCESS)
    {

        printf("ERROR #%d: code: %d\n", __LINE__, status);
        return status;
    }

    /* Open the ram_disk.  */
    status =  fx_media_open(&ram_disk, "RAM DISK", _fx_ram_driver, ram_disk_memory, ram_disk_working_buffer, 512);

    /* Check the media open status.  */
    if (status != FX_SUCCESS)
    {

        printf("ERROR #%d: code: %d\n", __LINE__, status);
        return status;
    }

    /* The code below is required for installing the device portion of USBX. 
       In this demo, DFU is possible and we have a call back for state change. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status != UX_SUCCESS)
    {

        printf("ERROR #%d: code: %d\n", __LINE__, status);
        return status;
    }

    /* Initilize the device storage class. The class is connected with interface 0 on configuration 1. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry, 
                                             1, 0, (VOID *)&storage_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d: code: %d\n", __LINE__, status);
        return status;
    }

    return UX_SUCCESS;
}

static VOID test_device_stack_storage_uninit(VOID)
{

    /* Unregister all class drivers.  */
    ux_device_stack_class_unregister(_ux_system_slave_class_storage_name, ux_device_class_storage_entry);

    /* Uninitialize stack.  */
    ux_device_stack_uninitialize();

    /* Close RAM Disk.  */
    fx_media_close(&ram_disk);
}

static UINT test_host_class_storage_inquiry(UX_HOST_CLASS_STORAGE *storage, UCHAR page_code, UCHAR *response, ULONG response_length)
{
UINT            status;
UCHAR           *cbw;
UINT            command_length;

    /* Use a pointer for the cbw, easier to manipulate.  */
    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;

    /* Get the Write Command Length.  */
    command_length =  UX_HOST_CLASS_STORAGE_INQUIRY_COMMAND_LENGTH_SBC;

    /* Initialize the CBW for this command.  */
    _ux_host_class_storage_cbw_initialize(storage, UX_HOST_CLASS_STORAGE_DATA_IN, UX_HOST_CLASS_STORAGE_INQUIRY_RESPONSE_LENGTH, command_length);
    
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

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_device_class_storage_vendor_strings_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;

    printf("Running ux_device_class_storage Vendor Strings Test................. ");

    /* Initialize the FX system.  */
    fx_system_initialize();

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(test_ux_error_callback);

    stepinfo(">>>>>>>>>>>>>>>> Test test_device_stack_storage_init default\n");

    /* Reset storage parameter. */
    _ux_utility_memory_set(&storage_parameter, 0, sizeof(storage_parameter));

    /* Store the number of LUN in this device storage instance.  */
    storage_parameter.ux_slave_class_storage_parameter_number_lun = 1;

    /* Initialize the storage class parameters for reading/writing to the Flash Disk.  */
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_last_lba        =  (UX_RAM_DISK_SIZE / 512) - 1;
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_block_length    =  512;
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_type            =  0;
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_removable_flag  =  0x80;
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read            =  default_device_media_read;
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write           =  default_device_media_write; 
    storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status          =  default_device_media_status;

    /* Initialize callbacks.  */
    storage_parameter.ux_slave_class_storage_instance_activate = test_slave_instance_activate;
    storage_parameter.ux_slave_class_storage_instance_deactivate = test_slave_instance_deactivate;

    status = test_device_stack_storage_init();
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register Storage class */
    status =  ux_host_stack_class_register(_ux_system_host_class_storage_name, ux_host_class_storage_entry);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register HCD for test */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_test_thread_host_simulation, "tx test simulation", tx_test_thread_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: code: %d\n", __LINE__, status);
        test_control_return(1);
    }
}

void  tx_test_thread_simulation_entry(ULONG arg)
{

UINT                                                status;
UX_DEVICE                                          *device;


    stepinfo("\n");


    /* Test connect. */
    stepinfo(">>>>>>>>>>>>>>>> Test connect\n");
    //ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    ux_test_breakable_sleep(1000, break_on_storage_all_ready);

    if (storage == UX_NULL || storage_slave == UX_NULL)
    {

        printf("ERROR #%d: connect fail\n", __LINE__);
        test_control_return(1);
    }

    /* Get device instance. */
    status = ux_host_stack_device_get(0, &device);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: device_get fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(standard)\n");

    status = test_host_class_storage_inquiry(storage, 0x00, inquiry_response, 36);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: inquiry(standard) error %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(standard) Response data\n");

    if (_ux_utility_memory_compare(inquiry_response + 8, _ux_system_slave_class_storage_vendor_id, 8) != UX_SUCCESS)
    {

        printf("ERROR #%d: vendor_id error %x\n", __LINE__, status);
        error_counter ++;
    }

    if (_ux_utility_memory_compare(inquiry_response + 16, _ux_system_slave_class_storage_product_id, 16) != UX_SUCCESS)
    {

        printf("ERROR #%d: product_id error %x\n", __LINE__, status);
        error_counter ++;
    }

    if (_ux_utility_memory_compare(inquiry_response + 32, _ux_system_slave_class_storage_product_rev, 4) != UX_SUCCESS)
    {

        printf("ERROR #%d: vendor id error %x\n", __LINE__, status);
        error_counter ++;
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(serial)\n");

    status = test_host_class_storage_inquiry(storage, 0x80, inquiry_response, 24);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: inquiry(serial) error %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(serial) Response data\n");

    if (_ux_utility_memory_compare(inquiry_response + 4, _ux_system_slave_class_storage_product_serial, 20) != UX_SUCCESS)
    {

        printf("ERROR #%d: vendor_id error %x\n", __LINE__, status);
        error_counter ++;
    }

    stepinfo(">>>>>>>>>>>>>>>> Test disconnect and deinitialize stack\n");

    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();

    test_device_stack_storage_uninit();

    stepinfo(">>>>>>>>>>>>>>>> Test modify parameter - vendor strings\n");

    storage_parameter.ux_slave_class_storage_parameter_vendor_id = test_ux_system_slave_class_storage_vendor_id;
    storage_parameter.ux_slave_class_storage_parameter_product_id = test_ux_system_slave_class_storage_product_id;
    storage_parameter.ux_slave_class_storage_parameter_product_rev = test_ux_system_slave_class_storage_product_rev;
    storage_parameter.ux_slave_class_storage_parameter_product_serial = test_ux_system_slave_class_storage_product_serial;

    stepinfo(">>>>>>>>>>>>>>>> Test test_device_stack_storage_init with new string\n");

    status = test_device_stack_storage_init();
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test connect\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    ux_test_breakable_sleep(100, break_on_storage_all_ready);
    if (storage == UX_NULL || storage_slave == UX_NULL)
    {

        printf("ERROR #%d: connect fail\n", __LINE__);
        test_control_return(1);
    }

    /* Get device instance. */
    status = ux_host_stack_device_get(0, &device);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: device_get fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(standard)\n");

    status = test_host_class_storage_inquiry(storage, 0x00, inquiry_response, 36);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: inquiry(standard) error %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(standard) Response data\n");

    if (_ux_utility_memory_compare(inquiry_response + 8, test_ux_system_slave_class_storage_vendor_id, 8) != UX_SUCCESS)
    {

        printf("ERROR #%d: vendor_id error %x\n", __LINE__, status);
        error_counter ++;
    }

    if (_ux_utility_memory_compare(inquiry_response + 16, test_ux_system_slave_class_storage_product_id, 16) != UX_SUCCESS)
    {

        printf("ERROR #%d: product_id error %x\n", __LINE__, status);
        error_counter ++;
    }

    if (_ux_utility_memory_compare(inquiry_response + 32, test_ux_system_slave_class_storage_product_rev, 4) != UX_SUCCESS)
    {

        printf("ERROR #%d: vendor id error %x\n", __LINE__, status);
        error_counter ++;
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(serial)\n");

    status = test_host_class_storage_inquiry(storage, 0x80, inquiry_response, 24);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: inquiry(serial) error %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test INQUIRY(serial) Response data\n");

    if (_ux_utility_memory_compare(inquiry_response + 4, test_ux_system_slave_class_storage_product_serial, 20) != UX_SUCCESS)
    {

        printf("ERROR #%d: vendor_id error %x\n", __LINE__, status);
        error_counter ++;
    }

    stepinfo(">>>>>>>>>>>>>>>> Dump results\n");

    if (error_counter > 0)
    {

        /* Test error.  */
        printf("ERROR #%d: total %ld errors\n", __LINE__, error_counter);
        test_control_return(1);
    }

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}


static UINT    default_device_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{

    /* The ATA drive never fails. This is just for demo only !!!! */
    return(UX_SUCCESS);
}

static UINT    default_device_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT status = UX_SUCCESS;


    if(lba == 0)
    {
        ram_disk.fx_media_driver_logical_sector =  0;
        ram_disk.fx_media_driver_sectors =  1;
        ram_disk.fx_media_driver_request =  FX_DRIVER_BOOT_READ;
        ram_disk.fx_media_driver_buffer =  data_pointer;
        _fx_ram_driver(&ram_disk);
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


        status = ram_disk.fx_media_driver_status;
    }        
    else
    {        
        while(number_blocks--)
        {
            status =  fx_media_read(&ram_disk,lba,data_pointer);
            data_pointer+=512;
            lba++;
        }
    }        
    return(status);
}

static UINT    default_device_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT status = UX_SUCCESS;

    if(lba == 0)
    {
        ram_disk.fx_media_driver_logical_sector =  0;
        ram_disk.fx_media_driver_sectors =  1;
        ram_disk.fx_media_driver_request =  FX_DRIVER_BOOT_WRITE;
        ram_disk.fx_media_driver_buffer =  data_pointer;
        _fx_ram_driver(&ram_disk);

        status = ram_disk.fx_media_driver_status;

    }        
    else
    {

        while(number_blocks--)
        {
            status =  fx_media_write(&ram_disk,lba,data_pointer);
            data_pointer+=512;
            lba++;
        }        
        return(status);
    }
    return(status);
}
