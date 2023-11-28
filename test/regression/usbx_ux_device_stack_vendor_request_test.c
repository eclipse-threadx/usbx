/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "fx_api.h"

#include "ux_device_stack.h"
#include "ux_device_class_dummy.h"

#include "ux_host_stack.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

/* Define constants.  */
#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024
#define                             UX_DEMO_BUFFER_SIZE (UX_SLAVE_REQUEST_DATA_MAX_LENGTH + 1)
#define                             UX_DEMO_XMIT_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE 512
#define                             UX_DEMO_FILE_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE 64
#define                             UX_DEMO_MEMORY_SIZE     (96*1024)
#define                             UX_DEMO_FILE_SIZE       (128 * 1024)
#define                             UX_RAM_DISK_MEMORY      (256 * 1024)

#define                             UX_DEMO_VENDOR_REQUEST 0x54

/* Define local/extern function prototypes.  */
static VOID                                test_thread_entry(ULONG);
static TX_THREAD                           tx_test_thread_simulation0;
static TX_THREAD                           tx_test_thread_simulation1;
static VOID                                tx_test_thread_simulation0_entry(ULONG);
static VOID                                tx_test_thread_simulation1_entry(ULONG);

static UINT                                test_ms_vendor_request(ULONG request, ULONG request_value, ULONG request_index, ULONG request_length,
                                                                  UCHAR *transfer_request_buffer, ULONG *transfer_request_length);


/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

static UX_DEVICE_CLASS_DUMMY_PARAMETER     *parameter;

static UX_DEVICE                           *host_device = UX_NULL;

static UCHAR                               buffer[UX_DEMO_BUFFER_SIZE];

static ULONG                               error_counter;

static ULONG                               set_cfg_counter;

static ULONG                               rsc_mem_alloc_cnt_on_set_cfg;
static ULONG                               rsc_sem_on_set_cfg;
static ULONG                               rsc_sem_get_on_set_cfg;
static ULONG                               rsc_mutex_on_set_cfg;

static ULONG                               rsc_enum_mem_alloc_count;
static ULONG                               rsc_enum_sem_usage;
static ULONG                               rsc_enum_sem_get_count;
static ULONG                               rsc_enum_mutex_usage;

static UINT                                class_entry_rc = UX_SUCCESS;

static UINT                                vendor_req_call_count = 0;
static UINT                                vendor_req_rc = UX_SUCCESS;
static UINT                                vendor_req_ret_len = 0;
static ULONG                               vendor_req_req_len;
static ULONG                               vendor_req_buf_len;

#define     LSB(x) ((x) & 0x00ff)
#define     MSB(x) (((x) & 0xff00) >> 8)

/* Storage related descriptors 9+7+7=23 bytes */
#define MS_IFC_DESC_ALL(ifc, bulk_in_epa, bulk_out_epa) \
    /* Interface descriptor */\
        0x09, 0x04, (ifc), 0x00, 0x03, 0x08, 0x06, 0x50, 0x00,\
    /* Endpoint descriptor (Bulk In) */\
        0x07, 0x05, (bulk_in_epa), 0x02, 0x40, 0x00, 0x00,\
    /* Endpoint descriptor (Bulk Out) */\
        0x07, 0x05, (bulk_out_epa), 0x02, 0x40, 0x00, 0x00,
#define MS_IFC_DESC_ALL_LEN 23

/* CDC IAD 8 bytes */
#define CDC_IAD_DESC(comm_ifc) \
    /* Interface association descriptor. 8 bytes.  */\
    0x08, 0x0b, (comm_ifc), 0x02, 0x02, 0x02, 0x00, 0x00,
#define CDC_IAD_DESC_LEN 8

/* CDC Communication interface descriptors 9+5+4+5+5+7=35 bytes */
#define CDC_COMM_IFC_DESC_ALL(comm_ifc, data_ifc, interrupt_epa) \
    /* Communication Class Interface Descriptor. 9 bytes. */\
    0x09, 0x04, (comm_ifc), 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,\
    /* Header Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x00, 0x10, 0x01,\
    /* ACM Functional Descriptor 4 bytes */\
    0x04, 0x24, 0x02, 0x0f,\
    /* Union Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x06,\
    (comm_ifc),                          /* Master interface */\
    (data_ifc),                          /* Slave interface  */\
    /* Call Management Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x01, 0x03,\
    (data_ifc),                          /* Data interface   */\
    /* Endpoint 0x83 descriptor 7 bytes */\
    0x07, 0x05, (interrupt_epa),\
    0x03,\
    0x08, 0x00,\
    0xFF,
#define CDC_COMM_IFC_DESC_ALL_LEN 35

/* CDC Data interface descriptors 9+7+7=23 bytes */
#define CDC_DATA_IFC_DESC_ALL(ifc, bulk_in_epa, bulk_out_epa) \
    /* Data Class Interface Descriptor Requirement 9 bytes */\
    0x09, 0x04, (ifc),\
    0x00, /* bAlternateSetting */\
    0x02, /* bNumEndpoints */\
    0x0A, 0x00, 0x00,\
    0x00,\
    /* Endpoint bulk IN descriptor 7 bytes */\
    0x07, 0x05, (bulk_in_epa),\
    0x02,\
    0x40, 0x00,\
    0x00,\
    /* Endpoint bulk OUT descriptor 7 bytes */\
    0x07, 0x05, (bulk_out_epa),\
    0x02,\
    0x40, 0x00,\
    0x00,
#define CDC_DATA_IFC_DESC_ALL_LEN 23

/* Configuration descriptor 9 bytes */
#define CFG_DESC(wTotalLength, bNumInterfaces, bConfigurationValue)\
    /* Configuration 1 descriptor 9 bytes */\
    0x09, 0x02, LSB(wTotalLength), MSB(wTotalLength),\
    (bNumInterfaces), (bConfigurationValue), 0x00,\
    0x40, 0x00,
#define CFG_DESC_LEN 9

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

    /* Configuration 1 descriptor 9 bytes */
    0x09, 0x02, 0x4b, 0x00,
    0x02, 0x01, 0x00,
    0xE0, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor. 9 bytes.   */
    0x09, 0x04, 0x00,
    0x00,
    0x01,
    0x02, 0x02, 0x01,
    0x00,

    /* Header Functional Descriptor 5 bytes */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor 4 bytes */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor 5 bytes */
    0x05, 0x24, 0x06,
    0x00,                          /* Master interface */
    0x01,                          /* Slave interface  */

    /* Call Management Functional Descriptor 5 bytes */
    0x05, 0x24, 0x01,
    0x03,
    0x01,                          /* Data interface   */

    /* Endpoint 0x83 descriptor 7 bytes */
    0x07, 0x05, 0x83,
    0x03,
    0x08, 0x00,
    0xFF,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02,
    0x02,
    0x40, 0x00,
    0x00,

    /* Configuration 2 descriptor 9 bytes */
    0x09, 0x02, (0x4b + 28 + 46), 0x00,
    0x02, 0x02, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,
    0x00, /* bAlternateSetting */
    0x00, /* bNumEndpoints */
    0x02, 0x02, 0x01,
    0x00,

    /* Header Functional Descriptor 5 bytes */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor 4 bytes */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor 5 bytes */
    0x05, 0x24, 0x06,
    0x00,                          /* Master interface */
    0x01,                          /* Slave interface  */

    /* Call Management Functional Descriptor 5 bytes */
    0x05, 0x24, 0x01,
    0x03,
    0x01,                          /* Data interface   */

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,
    0x01, /* bAlternateSetting */
    0x01, /* bNumEndpoints */
    0x02, 0x02, 0x01,
    0x00,

    /* Header Functional Descriptor 5 bytes */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor 4 bytes */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor 5 bytes */
    0x05, 0x24, 0x06,
    0x00,                          /* Master interface */
    0x01,                          /* Slave interface  */

    /* Call Management Functional Descriptor 5 bytes */
    0x05, 0x24, 0x01,
    0x03,
    0x01,                          /* Data interface   */

    /* Endpoint 0x83 descriptor 7 bytes */
    0x07, 0x05, 0x83,
    0x03,
    0x08, 0x00,
    0xFF,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x00, /* bAlternateSetting */
    0x00, /* bNumEndpoints */
    0x0A, 0x00, 0x00,
    0x00,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x01, /* bAlternateSetting */
    0x02, /* bNumEndpoints */
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02,
    0x02,
    0x40, 0x00,
    0x00,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x02, /* bAlternateSetting */
    0x04, /* bNumEndpoints */
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x85 descriptor 7 bytes */
    0x07, 0x05, 0x85,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x04 descriptor 7 bytes */
    0x07, 0x05, 0x04,
    0x02,
    0x40, 0x00,
    0x00,

    /* Configuration 3: CDC + CDC */
    CFG_DESC(CFG_DESC_LEN+2*(CDC_IAD_DESC_LEN+CDC_COMM_IFC_DESC_ALL_LEN+CDC_DATA_IFC_DESC_ALL_LEN), 4, 3)
    CDC_IAD_DESC(0)
    CDC_COMM_IFC_DESC_ALL(0, 1, 0x83)
    CDC_DATA_IFC_DESC_ALL(1, 0x81, 0x02)
    CDC_IAD_DESC(2)
    CDC_COMM_IFC_DESC_ALL(2, 3, 0x86)
    CDC_DATA_IFC_DESC_ALL(3, 0x84, 0x05)
};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)

static unsigned char device_framework_high_speed[] = {

    /* Device descriptor
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

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02,
    0x02, 0x00, 0x00,
    0x40,
    0x01,
    0x00,

    /* Configuration 1 descriptor */
    0x09, 0x02, (0x4b+5), 0x00,
    0x02, 0x01, 0x00,
    0xE0, 0x00,

    /* OTG Descriptor.  */
    0x05, 0x09, 0x03, 0x02, 0x00,

    /* Interface association descriptor. */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement */
    0x09, 0x04, 0x00,
    0x00,
    0x01,
    0x02, 0x02, 0x01,
    0x00,

    /* Header Functional Descriptor */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor */
    0x05, 0x24, 0x06,
    0x00,
    0x01,

    /* Call Management Functional Descriptor */
    0x05, 0x24, 0x01,
    0x00,
    0x01,

    /* Endpoint 0x83 descriptor */
    0x07, 0x05, 0x83,
    0x03,
    0x08, 0x00,
    0xFF,

    /* Data Class Interface Descriptor Requirement */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor */
    0x07, 0x05, 0x81,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor */
    0x07, 0x05, 0x02,
    0x02,
    0x40, 0x00,
    0x00,

    /* Configuration 2 descriptor 9 bytes */
    0x09, 0x02, (0x4b + 28 + 46), 0x00,
    0x02, 0x02, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,
    0x00, /* bAlternateSetting */
    0x00, /* bNumEndpoints */
    0x02, 0x02, 0x01,
    0x00,

    /* Header Functional Descriptor 5 bytes */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor 4 bytes */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor 5 bytes */
    0x05, 0x24, 0x06,
    0x00,                          /* Master interface */
    0x01,                          /* Slave interface  */

    /* Call Management Functional Descriptor 5 bytes */
    0x05, 0x24, 0x01,
    0x03,
    0x01,                          /* Data interface   */

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,
    0x01, /* bAlternateSetting */
    0x01, /* bNumEndpoints */
    0x02, 0x02, 0x01,
    0x00,

    /* Header Functional Descriptor 5 bytes */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor 4 bytes */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor 5 bytes */
    0x05, 0x24, 0x06,
    0x00,                          /* Master interface */
    0x01,                          /* Slave interface  */

    /* Call Management Functional Descriptor 5 bytes */
    0x05, 0x24, 0x01,
    0x03,
    0x01,                          /* Data interface   */

    /* Endpoint 0x83 descriptor 7 bytes */
    0x07, 0x05, 0x83,
    0x03,
    0x08, 0x00,
    0xFF,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x00,
    0x00,
    0x0A, 0x00, 0x00,
    0x00,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x01,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02,
    0x02,
    0x40, 0x00,
    0x00,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x02, /* bAlternateSetting */
    0x04, /* bNumEndpoints */
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x85 descriptor 7 bytes */
    0x07, 0x05, 0x85,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x04 descriptor 7 bytes */
    0x07, 0x05, 0x04,
    0x02,
    0x40, 0x00,
    0x00,

    /* Configuration 3: CDC + CDC */
    CFG_DESC(CFG_DESC_LEN+2*(CDC_IAD_DESC_LEN+CDC_COMM_IFC_DESC_ALL_LEN+CDC_DATA_IFC_DESC_ALL_LEN), 4, 3)
    CDC_IAD_DESC(0)
    CDC_COMM_IFC_DESC_ALL(0, 1, 0x83)
    CDC_DATA_IFC_DESC_ALL(1, 0x81, 0x02)
    CDC_IAD_DESC(2)
    CDC_COMM_IFC_DESC_ALL(2, 3, 0x86)
    CDC_DATA_IFC_DESC_ALL(3, 0x84, 0x05)
};
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)


static unsigned char string_framework[] = {

    /* Manufacturer string descriptor : Index 1 - "Express Logic" */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72, 0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 - "EL Composite device" */
        0x09, 0x04, 0x02, 0x13,
        0x45, 0x4c, 0x20, 0x43, 0x6f, 0x6d, 0x70, 0x6f,
        0x73, 0x69, 0x74, 0x65, 0x20, 0x64, 0x65, 0x76,
        0x69, 0x63, 0x65,

    /* Serial Number string descriptor : Index 3 - "0001" */
        0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31,

    /* Microsoft OS string descriptor : Index 0xEE. String is MSFT100.
       The last byte is the vendor code used to filter Vendor specific commands.
       The vendor commands will be executed in the class.
       This code can be anything but must not be 0x66 or 0x67 which are PIMA class commands.  */
        0x00, 0x00, 0xEE, 0x08,
        0x4D, 0x53, 0x46, 0x54,
        0x31, 0x30, 0x30,
        UX_DEMO_VENDOR_REQUEST
};
#define STRING_FRAMEWORK_LENGTH sizeof(string_framework)

/* Multiple languages are supported on the device, to add
    a language besides english, the unicode language code must
    be appended to the language_id_framework array and the length
    adjusted accordingly. */
static unsigned char language_id_framework[] = {

    /* English. */
        0x09, 0x04
};
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(language_id_framework)


/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}

static UINT test_ms_vendor_request(ULONG request, ULONG request_value, ULONG request_index, ULONG request_length,
                                   UCHAR *transfer_request_buffer, ULONG *transfer_request_length)
{
    vendor_req_req_len = request_length;
    vendor_req_buf_len = *transfer_request_length;
    *transfer_request_length = vendor_req_ret_len;
    return vendor_req_rc;
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

    switch(event)
    {

        case UX_DEVICE_INSERTION:
            break;

        case UX_DEVICE_REMOVAL:
            break;

        case UX_DEVICE_CONNECTION:
            host_device = (UX_DEVICE *)inst;
            break;
        case UX_DEVICE_DISCONNECTION:
            if (host_device == (UX_DEVICE *)inst)
                host_device = UX_NULL;
            break;

        default:
            break;
    }
    return 0;
}

static VOID test_ux_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    if (error_code == UX_DEVICE_ENUMERATION_FAILURE ||
        error_code == UX_TRANSFER_STALLED)
    {
        /* It's normal.  */
        return;
    }
    printf("error 0x%x, 0x%x, 0x%x\n", system_level, system_context, error_code);
    test_control_return(1);
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_stack_vendor_request_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;

    printf("Running Device Stack Vendor Request Test............................ ");

    /* Reset testing counts. */
    ux_test_utility_sim_mem_alloc_count_reset();
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    /* Reset error generations */
    ux_test_utility_sim_mem_alloc_error_generation_stop();
    ux_test_utility_sim_sem_error_generation_stop();
    ux_test_utility_sim_mutex_error_generation_stop();
    ux_test_utility_sim_sem_get_error_generation_stop();

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #1\n");
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(test_ux_error_callback);

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf(" ERROR #5\n");
        test_control_return(1);
    }

    /* Set the parameters for callback when insertion/extraction of a CDC device.  */

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status  =  ux_device_stack_class_register(_ux_device_class_dummy_name, _ux_device_class_dummy_entry,
                                             1, 0,  &parameter);
    if(status!=UX_SUCCESS)
    {

        printf(" ERROR #7\n");
        test_control_return(1);
    }

    /* MS extensions.  */
    status = _ux_device_stack_microsoft_extension_register(UX_DEMO_VENDOR_REQUEST, test_ms_vendor_request);

    if(status!=UX_SUCCESS)
    {

        printf(" ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf(" ERROR #8\n");
        test_control_return(1);
    }

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(test_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #2\n");
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #4\n");
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_test_thread_simulation0, "tx test simulation 0", tx_test_thread_simulation0_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf(" ERROR #9\n");
        test_control_return(1);
    }

    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&tx_test_thread_simulation1, "tx test simulation 1", tx_test_thread_simulation1_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf(" ERROR #10\n");
        test_control_return(1);
    }
}

void  tx_test_thread_simulation0_entry(ULONG arg)
{
UINT                                                status;
ULONG                                               actual_length;
UX_DEVICE                                          *device;
UX_ENDPOINT                                        *control_endpoint;
UX_TRANSFER                                        *transfer_request;
UX_SLAVE_TRANSFER                                  *slave_transfer_request;

    stepinfo("\n");

    /* Wait for first enumeration to complete.  */
    stepinfo(">>>>>>>>>>>>>>>> Wait for first enumeration completion\n");
    while (host_device == UX_NULL)
        tx_thread_sleep(10);

    /* Wait for instances to be live. */
    tx_thread_sleep(10);

    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();

    /* Test connect. Note that we must switch to high speed for tests. */
    stepinfo(">>>>>>>>>>>>>>>> Test connect\n");
    ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);
    tx_thread_sleep(100);
    if (host_device == UX_NULL)
    {

        printf("ERROR #%d: connection not detected\n", __LINE__);
        test_control_return(1);
    }

    device = host_device;
    control_endpoint = &device->ux_device_control_endpoint;
    transfer_request = &control_endpoint->ux_endpoint_transfer_request;

    slave_transfer_request = &_ux_system_slave->ux_system_slave_device.ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

    stepinfo(">>>>>>>>>>>>>>>> Test Vendor Request\n");
    transfer_request -> ux_transfer_request_function =          0xEE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_VENDOR | UX_REQUEST_TARGET_DEVICE;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_TRANSFER_STALLED)
    {

        printf("ERROR #%d: Vendor request status %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test MS Vendor Request\n");
    transfer_request -> ux_transfer_request_function =          UX_DEMO_VENDOR_REQUEST;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_VENDOR | UX_REQUEST_TARGET_DEVICE;

    vendor_req_rc = UX_ERROR;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_TRANSFER_STALLED)
    {

        printf("ERROR #%d: Vendor request status %x\n", __LINE__, status);
        test_control_return(1);
    }
    vendor_req_rc = UX_SUCCESS;

    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: Vendor request status %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test vendor request NULL case\n");
    status = _ux_device_stack_microsoft_extension_register(UX_DEMO_VENDOR_REQUEST, UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d: Vendor callback clear fail\n", __LINE__);
        test_control_return(1);
    }
    vendor_req_call_count = 0;
    status = ux_host_stack_transfer_request(transfer_request);
    UX_TEST_ASSERT_MESSAGE(status == UX_TRANSFER_STALLED, "ERROR #%d: Vendor request status %x\n", __LINE__, status);
    UX_TEST_ASSERT(vendor_req_call_count == 0);

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);

}

void  tx_test_thread_simulation1_entry(ULONG arg)
{

UINT status;
ULONG                                               actual_length;

    while(1)
    {

        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
    }
}
