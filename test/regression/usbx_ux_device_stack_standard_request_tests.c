/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "fx_api.h"

#include "ux_device_class_cdc_acm.h"
#include "ux_device_stack.h"

#include "ux_host_class_cdc_acm.h"
#include "ux_host_stack.h"

#include "ux_host_class_hid.h"
#include "ux_device_class_hid.h"

#include "ux_host_class_storage.h"
#include "ux_device_class_storage.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

/* Define constants.  */
#define                             UX_CDC_ACM_CONNECTION_DELAY ((UX_RH_ENUMERATION_RETRY + 1)*UX_HOST_CLASS_CDC_ACM_DEVICE_INIT_DELAY)
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
static VOID                                test_cdc_instance_activate(VOID  *cdc_instance);
static VOID                                test_cdc_instance_deactivate(VOID *cdc_instance);
static VOID                                test_cdc_instance_parameter_change(VOID *cdc_instance);

static VOID                                ux_test_hcd_entry_should_not_be_called(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_disconnect(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);

static UINT                                test_ux_device_class_cdc_acm_entry(UX_SLAVE_CLASS_COMMAND *command);

static UINT                                test_ms_vendor_request(ULONG request, ULONG request_value, ULONG request_index, ULONG request_length,
                                                                  UCHAR *transfer_request_buffer, ULONG *transfer_request_length);

static VOID                                ux_test_dcd_entry_is_called(UX_TEST_ACTION *action, VOID *params);

static UINT test_ux_device_class_cdc_acm_read_halt(UX_SLAVE_CLASS_CDC_ACM *cdc_acm);

static UINT test_ux_host_class_cdc_acm_write(UX_HOST_CLASS_CDC_ACM *cdc_acm, UCHAR *data_pointer,
                                    ULONG requested_length, ULONG *actual_length);
static UINT test_ux_host_class_cdc_acm_write_halt_clear(UX_HOST_CLASS_CDC_ACM *cdc_acm);


/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UX_HOST_CLASS                       *class_driver;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_control;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_data;

static UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave;
static UCHAR                               cdc_acm_slave_change;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER    parameter;
static UCHAR                               cdc_acm_slave_reading = UX_TRUE;

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

static UINT                                vendor_req_rc = UX_SUCCESS;

static UX_SLAVE_CLASS_HID_PARAMETER        hid_parameter;

/* HID mouse related descriptors */

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

#define     LSB(x) (x & 0x00ff)
#define     MSB(x) ((x & 0xff00) >> 8)

/* HID Mouse interface descriptors 9+9+7=25 bytes */
#define HID_MOUSE_IFC_DESC_ALL(ifc, interrupt_epa)     \
    /* Interface descriptor */\
        0x09, 0x04, (ifc), 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,\
    /* HID descriptor */\
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_MOUSE_REPORT_LENGTH),\
        MSB(HID_MOUSE_REPORT_LENGTH),\
    /* Endpoint descriptor (Interrupt) */\
        0x07, 0x05, (interrupt_epa), 0x03, 0x08, 0x00, 0x08,
#define HID_MOUSE_IFC_DESC_ALL_LEN 25

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
    15,
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

    /* Configuration 4: HID + HID */
    CFG_DESC(CFG_DESC_LEN+2*(HID_MOUSE_IFC_DESC_ALL_LEN), 2, 4)
    HID_MOUSE_IFC_DESC_ALL(0, 0x81)
    HID_MOUSE_IFC_DESC_ALL(1, 0x82)

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
    15,

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
    15,

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

    /* Configuration 4: HID + HID */
    CFG_DESC(CFG_DESC_LEN+2*(HID_MOUSE_IFC_DESC_ALL_LEN), 2, 4)
    HID_MOUSE_IFC_DESC_ALL(0, 0x81)
    HID_MOUSE_IFC_DESC_ALL(1, 0x82)

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

/* Simulation actions. */

static UX_TEST_SIM_ENTRY_ACTION dcd_transfer_is_called[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_DCD_TRANSFER_REQUEST, NULL,
        UX_FALSE, 0,
        0         , 0, UX_NULL, 0, 0,
        UX_ERROR , ux_test_dcd_entry_is_called},
{   0   }
};


/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}

static VOID ux_test_dcd_entry_is_called(UX_TEST_ACTION *action, VOID *params)
{
    error_counter ++;
}

static UINT test_ms_vendor_request(ULONG request, ULONG request_value, ULONG request_index, ULONG request_length,
                                   UCHAR *transfer_request_buffer, ULONG *transfer_request_length)
{
    return vendor_req_rc;
}


static UINT test_ux_device_class_cdc_acm_entry(UX_SLAVE_CLASS_COMMAND *command)
{
    if (class_entry_rc != UX_SUCCESS)
        return class_entry_rc;

    switch (command -> ux_slave_class_command_request)
    {
        case UX_SLAVE_CLASS_COMMAND_CHANGE:
            return UX_SUCCESS;
        default: break;
    }
    return _ux_device_class_cdc_acm_entry(command);
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_CDC_ACM *cdc_acm = (UX_HOST_CLASS_CDC_ACM *) inst;

    switch(event)
    {

        case UX_DEVICE_INSERTION:

            if (cdc_acm -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
                cdc_acm_host_control = cdc_acm;
            else
                cdc_acm_host_data = cdc_acm;
            break;

        case UX_DEVICE_REMOVAL:

            if (cdc_acm -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
                cdc_acm_host_control = UX_NULL;
            else
                cdc_acm_host_data = UX_NULL;
            break;

        default:
            break;
    }
    return 0;
}

static VOID test_cdc_instance_activate(VOID *cdc_instance)
{

    /* Save the CDC instance.  */
    cdc_acm_slave = (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;
}
static VOID test_cdc_instance_deactivate(VOID *cdc_instance)
{

    /* Reset the CDC instance.  */
    cdc_acm_slave = UX_NULL;
}

static VOID test_cdc_instance_parameter_change(VOID *cdc_instance)
{

    /* Set CDC parameter change flag. */
    cdc_acm_slave_change = UX_TRUE;
}

static VOID test_ux_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
}

static VOID ux_test_hcd_entry_should_not_be_called(UX_TEST_ACTION *action, VOID *params)
{

    error_counter ++;
}

static VOID ux_test_hcd_entry_disconnect(UX_TEST_ACTION *action, VOID *params)
{

    ux_test_dcd_sim_slave_disconnect();
}

static VOID ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params)
{

    set_cfg_counter ++;

    rsc_mem_alloc_cnt_on_set_cfg = ux_test_utility_sim_mem_alloc_count();

    rsc_sem_on_set_cfg = ux_test_utility_sim_sem_create_count();
    rsc_enum_sem_get_count = ux_test_utility_sim_sem_get_count();
    rsc_mutex_on_set_cfg = ux_test_utility_sim_mutex_create_count();
}

static UINT test_ux_device_class_cdc_acm_read_halt(UX_SLAVE_CLASS_CDC_ACM *cdc_acm)
{
UX_SLAVE_ENDPOINT           *endpoint;
UX_SLAVE_INTERFACE          *interface;

    /* This is the first time we are activated. We need the interface to the class.  */
    interface =  cdc_acm -> ux_slave_class_cdc_acm_interface;

    /* Locate the endpoints.  */
    endpoint =  interface -> ux_slave_interface_first_endpoint;

    /* Check the endpoint direction, if OUT we have the correct endpoint.  */
    if ((endpoint -> ux_slave_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) != UX_ENDPOINT_OUT)
    {

        /* So the next endpoint has to be the OUT endpoint.  */
        endpoint =  endpoint -> ux_slave_endpoint_next_endpoint;
    }

    return ux_device_stack_endpoint_stall(endpoint);
}

extern UINT  _ux_host_stack_endpoint_reset(UX_ENDPOINT *endpoint);
static UINT test_ux_host_class_cdc_acm_write_halt_clear(UX_HOST_CLASS_CDC_ACM *cdc_acm)
{
    return _ux_host_stack_endpoint_reset(cdc_acm->ux_host_class_cdc_acm_bulk_out_endpoint);
}

static UINT test_ux_host_class_cdc_acm_write(UX_HOST_CLASS_CDC_ACM *cdc_acm, UCHAR *data_pointer,
                                    ULONG requested_length, ULONG *actual_length)
{
UX_TRANSFER     *transfer_request;
UINT            status;
ULONG           transfer_request_length;

    /* Start by resetting the actual length of the transfer.  */
    *actual_length = 0;

    /* Get the pointer to the bulk out endpoint transfer request.  */
    transfer_request =  &cdc_acm -> ux_host_class_cdc_acm_bulk_out_endpoint -> ux_endpoint_transfer_request;

    /* Program the maximum authorized length for this transfer_request.  */
    if (requested_length > transfer_request -> ux_transfer_request_maximum_length)
        transfer_request_length =  transfer_request -> ux_transfer_request_maximum_length;
    else
        transfer_request_length =  requested_length;

    /* Initialize the transfer_request.  */
    transfer_request -> ux_transfer_request_data_pointer =  data_pointer;
    transfer_request -> ux_transfer_request_requested_length =  transfer_request_length;

    /* Perform the transfer.  */
    status =  ux_host_stack_transfer_request(transfer_request);

    /* If the transfer is successful, we need to wait for the transfer request to be completed.  */
    if (status == UX_SUCCESS)
    {

        /* Wait for the completion of the transfer request.  */
        status =  _ux_utility_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, UX_HOST_CLASS_CDC_ACM_CLASS_TRANSFER_TIMEOUT);

        /* Update the length of the actual data transferred. We do this after the
            abort of the transfer_request in case some data actually went out.  */
        *actual_length +=  transfer_request -> ux_transfer_request_actual_length;

        /* If the semaphore did not succeed we probably have a time out.  */
        if (status != UX_SUCCESS)
        {

            /* All transfers pending need to abort. There may have been a partial transfer.  */
            ux_host_stack_transfer_request_abort(transfer_request);

            /* Set the completion code.  */
            transfer_request -> ux_transfer_request_completion_code =  UX_TRANSFER_TIMEOUT;

            /* Error trap. */
            _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_TRANSFER_TIMEOUT);

            /* If trace is enabled, insert this event into the trace buffer.  */
            UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_TRANSFER_TIMEOUT, transfer_request, 0, 0, UX_TRACE_ERRORS, 0, 0)

        }

        /* Report completion code. */
        status = transfer_request -> ux_transfer_request_completion_code;
    }

    /* Return status. */
    return(status);
}


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_device_stack_standard_request_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;

    printf("Running Host & Device Standard Request Test......................... ");

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
    parameter.ux_slave_class_cdc_acm_instance_activate   =  test_cdc_instance_activate;
    parameter.ux_slave_class_cdc_acm_instance_deactivate =  test_cdc_instance_deactivate;
    parameter.ux_slave_class_cdc_acm_parameter_change    =  test_cdc_instance_parameter_change;

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status  =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, test_ux_device_class_cdc_acm_entry,
                                             1,0,  &parameter);
    status |=  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, test_ux_device_class_cdc_acm_entry,
                                             1,2,  &parameter);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    if(status!=UX_SUCCESS)
    {

        printf(" ERROR #7\n");
        test_control_return(1);
    }
#endif

    /* Initialize the hid class parameters.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_mouse_report;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_MOUSE_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = UX_NULL;
    hid_parameter.ux_slave_class_hid_instance_activate         = UX_NULL;

    /* Initilize the device hid class. The class is connected with interface 0 */
    status  =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                              4, 0, (VOID *)&hid_parameter);
    // status |=  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
    //                                           4, 1, (VOID *)&hid_parameter);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d, error code: %x\n", __LINE__, status);
        test_control_return(1);
    }
#endif

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

    /* Register CDC-ACM class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_cdc_acm_name, ux_host_class_cdc_acm_entry);
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #3\n");
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
    while (cdc_acm_host_control == UX_NULL || cdc_acm_host_data == UX_NULL || cdc_acm_slave == UX_NULL)
        tx_thread_sleep(10);

    /* Wait for instances to be live. */
    tx_thread_sleep(10);

    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();

    /* Test connect. Note that we must switch to high speed for tests. */
    stepinfo(">>>>>>>>>>>>>>>> Test connect\n");
    ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);
    tx_thread_sleep(UX_CDC_ACM_CONNECTION_DELAY);
    if (cdc_acm_host_control == UX_NULL || cdc_acm_host_data == UX_NULL || cdc_acm_slave == UX_NULL)
    {

        printf("ERROR #%d: connection not detected\n", __LINE__);
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

    slave_transfer_request = &_ux_system_slave->ux_system_slave_device.ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

    /* Test bulk stall handling */
    stepinfo(">>>>>>>>>>>>>>>> Test Bulk OUT STALL\n");
    /* Start waiting stall on device side */
    status = test_ux_host_class_cdc_acm_write(cdc_acm_host_data, "TSTALL\n", 7, &actual_length);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: Bulk OUT fail: %x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Check endpoint stalled */
    status = test_ux_host_class_cdc_acm_write(cdc_acm_host_data, "TEST\n", 5, &actual_length);
    if (status != UX_TRANSFER_STALLED)
    {

        printf("ERROR #%d: Bulk OUT not stalled: %x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Clear halt */
    status = test_ux_host_class_cdc_acm_write_halt_clear(cdc_acm_host_data);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: Stall clear fail: %x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Check endpoint good */
    status = test_ux_host_class_cdc_acm_write(cdc_acm_host_data, "TEST\n", 5, &actual_length);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: Bulk OUT failed: %x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Test normal requests */
    stepinfo(">>>>>>>>>>>>>>>> Test GetDeviceDescriptor\n");
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  64;
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             UX_DEVICE_DESCRIPTOR_ITEM << 8;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetDeviceDescriptor(64) fail\n", __LINE__);
        test_control_return(1);
    }
    if (transfer_request->ux_transfer_request_actual_length != 18)
    {

        printf("ERROR #%d: GetDeviceDescriptor(64) actual length is not 18\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test GetDeviceQualifierDescriptor\n");
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  8;
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             UX_DEVICE_QUALIFIER_DESCRIPTOR_ITEM << 8;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetDeviceQualifierDescriptor(8) fail\n", __LINE__);
        test_control_return(1);
    }
    if (transfer_request->ux_transfer_request_actual_length != 8)
    {

        printf("ERROR #%d: GetDeviceQualifierDescriptor(8) actual length is not 8\n", __LINE__);
        test_control_return(1);
    }

    transfer_request -> ux_transfer_request_requested_length =  64;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetDeviceQualifierDescriptor(64) fail\n", __LINE__);
        test_control_return(1);
    }
    if (transfer_request->ux_transfer_request_actual_length != 10)
    {

        printf("ERROR #%d: GetDeviceQualifierDescriptor(64) actual length is not 10\n", __LINE__);
        test_control_return(1);
    }

    _ux_system_slave -> ux_system_slave_device_framework =  _ux_system_slave -> ux_system_slave_device_framework_full_speed;
    _ux_system_slave -> ux_system_slave_device_framework_length =  _ux_system_slave -> ux_system_slave_device_framework_length_full_speed;

    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: GetDeviceQualifierDescriptor(64) should fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test GetOTGDescriptor\n");
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  64;
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             UX_OTG_DESCRIPTOR_ITEM << 8;
    transfer_request -> ux_transfer_request_index =             0;

    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: GetOTGDescriptor(64) should fail\n", __LINE__);
        test_control_return(1);
    }

    _ux_system_slave -> ux_system_slave_device_framework =  _ux_system_slave -> ux_system_slave_device_framework_high_speed;
    _ux_system_slave -> ux_system_slave_device_framework_length =  _ux_system_slave -> ux_system_slave_device_framework_length_high_speed;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetOTGDescriptor(64) fail: %x\n", __LINE__, status);
        test_control_return(1);
    }
    if (transfer_request->ux_transfer_request_actual_length != 5)
    {

        printf("ERROR #%d: GetOTGDescriptor(64) actual length is not 5\n", __LINE__);
        test_control_return(1);
    }

    transfer_request -> ux_transfer_request_requested_length =  5;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetOTGDescriptor(5) fail\n", __LINE__);
        test_control_return(1);
    }
    if (transfer_request->ux_transfer_request_actual_length != 5)
    {

        printf("ERROR #%d: GetOTGDescriptor(5) actual length is not 5\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test GetOtherSpeedDescriptor\n");
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  8;
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             UX_OTHER_SPEED_DESCRIPTOR_ITEM << 8;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetOtherSpeedDescriptor(8) fail\n", __LINE__);
        test_control_return(1);
    }

    transfer_request -> ux_transfer_request_requested_length =  256;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetOtherSpeedDescriptor(256) fail\n", __LINE__);
        test_control_return(1);
    }

    transfer_request -> ux_transfer_request_value =             (UX_OTHER_SPEED_DESCRIPTOR_ITEM << 8) | 10;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: GetOtherSpeedDescriptor(10, 256) must fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test GetConfigurationDescriptor\n");
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  256;
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_index =             0;

    /* Get existing configuration */
    transfer_request -> ux_transfer_request_value =             UX_CONFIGURATION_DESCRIPTOR_ITEM << 8;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetConfigurationDescriptor(0, 256) fail\n", __LINE__);
        test_control_return(1);
    }

    /* Get non-existing configuration */
    transfer_request -> ux_transfer_request_value =             (UX_CONFIGURATION_DESCRIPTOR_ITEM << 8) | 10;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: GetConfigurationDescriptor(10, 256) must fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test GetStringDescriptor\n");
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  64;
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;

    /* Get LangID with big buffer */
    transfer_request -> ux_transfer_request_value =             UX_STRING_DESCRIPTOR_ITEM << 8;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetStringDescriptor(0, 64) fail\n", __LINE__);
        test_control_return(1);
    }

    /* Get LangID with exactly size */
    transfer_request -> ux_transfer_request_requested_length =  4;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetStringDescriptor(0, 4) fail\n", __LINE__);
        test_control_return(1);
    }

    /* Get String with small buffer */
    transfer_request -> ux_transfer_request_index =             0x0409;
    transfer_request -> ux_transfer_request_value =             (UX_STRING_DESCRIPTOR_ITEM << 8) | 1;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetStringDescriptor(1, 4) fail\n", __LINE__);
        test_control_return(1);
    }

    /* Get String with large buffer */
    transfer_request -> ux_transfer_request_requested_length =  256;
    transfer_request -> ux_transfer_request_value =             (UX_STRING_DESCRIPTOR_ITEM << 8) | 2;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetStringDescriptor(2, 256) fail\n", __LINE__);
        test_control_return(1);
    }

    /* Get String not existing */
    transfer_request -> ux_transfer_request_value =             (UX_STRING_DESCRIPTOR_ITEM << 8) | 10;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: GetStringDescriptor(10, 256) must fail\n", __LINE__);
        test_control_return(1);
    }

    /* Get Manufacturer string descriptor. */
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  0xff;
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             (UX_STRING_DESCRIPTOR_ITEM << 8) | 0x01;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetStringDescriptor(10, 256) must fail\n", __LINE__);
        test_control_return(1);
    }

    UCHAR string_descriptor_manufacturer_length = *(string_framework + 3);
    UCHAR get_string_descriptor_manufacturer_expected[] = {
        /* bLength. '2 +' is for bLength and bDescriptorType, '0x0c' is the length of 
           the string, and '*2' is because it's 16-bit unicode, where each character 
           is 2 bytes, the LSB is the value, and MSB is 0 (for ascii anyways). */
        (UCHAR)(2 + string_descriptor_manufacturer_length*2),

        /* bDescriptorType */
        0x03,

        /* "Express Logic" in unicode. */
        0x45, 0x00,
        0x78, 0x00,
        0x70, 0x00,
        0x72, 0x00,
        0x65, 0x00, 
        0x73, 0x00,
        0x20, 0x00,
        0x4c, 0x00,
        0x6f, 0x00,
        0x67, 0x00,
        0x69, 0x00,
        0x63, 0x00,
    };

    /* Ensure the length is correct.  */
    if (transfer_request->ux_transfer_request_actual_length != sizeof(get_string_descriptor_manufacturer_expected))
    {

        printf("ERROR on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Now check the contents. */
    if (_ux_utility_memory_compare(transfer_request->ux_transfer_request_data_pointer, get_string_descriptor_manufacturer_expected, sizeof(get_string_descriptor_manufacturer_expected)))
    {

        printf("ERROR on line %d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test GetDescriptor(unknown)\n");
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  64;
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;

    /* Get unknown discriptor */
    transfer_request -> ux_transfer_request_value =             11 << 8;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: GetDescriptor(unknown) must fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test GetDescriptor(class)\n");
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  64;
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;

    /* Get class discriptor */
    transfer_request -> ux_transfer_request_value =             (11 + UX_REQUEST_TYPE_CLASS) << 8;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: GetDescriptor(class) must fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test SetFeature/ClearFeature/GetStatus\n");

    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_function =          UX_SET_FEATURE;

    /* SetDeviceFeature */
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             UX_REQUEST_FEATURE_DEVICE_REMOTE_WAKEUP;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetDeviceFeature fail\n", __LINE__);
        test_control_return(1);
    }

    /* SetInterfaceFeature */
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    status = ux_host_stack_transfer_request(transfer_request);

    /* Normal write, should be good. */
    status = test_ux_host_class_cdc_acm_write(cdc_acm_host_data, "TEST\n", 5, &actual_length);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: Bulk OUT status %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test SetEndpointFeature(0x81)\n");

    /* SetEndpointFeature to existing endpoint */
    transfer_request -> ux_transfer_request_index =             0x02;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_ENDPOINT;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetEndpointFeature(0x81) fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test SetEndpointFeature(0x82)\n");

    /* SetEndpointFeature to not existing one */
    transfer_request -> ux_transfer_request_index =             0x82;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_ENDPOINT;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: SetEndpointFeature(0x82) must fail\n", __LINE__);
        test_control_return(1);
    }

    /* GetDeviceStatus */
    transfer_request -> ux_transfer_request_requested_length =  64;
    transfer_request -> ux_transfer_request_function =          UX_GET_STATUS;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetDeviceStatus fail\n", __LINE__);
        test_control_return(1);
    }

    /* GetDeviceOTGStatus */
    transfer_request -> ux_transfer_request_index =             UX_OTG_STATUS_SELECTOR;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetDeviceOTGStatus fail\n", __LINE__);
        test_control_return(1);
    }

    /* GetInterfaceStatus may or may not implement */
    transfer_request -> ux_transfer_request_index =             0;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    status = ux_host_stack_transfer_request(transfer_request);

    /* GetEndpointStatus for existing endpoint */
    transfer_request -> ux_transfer_request_index =             0x02;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_ENDPOINT;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetEndpointStatus(0x02) fail\n", __LINE__);
        test_control_return(1);
    }
    if (buffer[0] != 1)
    {

        printf("ERROR #%d: GetEndpointStatus(0x02) should halt\n", __LINE__);
        test_control_return(1);

    }

    /* Write, should return halt. */
    status  = test_ux_host_class_cdc_acm_write(cdc_acm_host_data, "TEST\n", 5, &actual_length);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: Bulk OUT status %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test ClearEndpointFeature(0x02)\n");

    /* ClearEndpointFeature for existing endpoint */
    transfer_request -> ux_transfer_request_function =          UX_CLEAR_FEATURE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_ENDPOINT;
    transfer_request -> ux_transfer_request_index =             0x02;
    transfer_request -> ux_transfer_request_requested_length =  0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: ClearEndpointFeature(0x02) fail\n", __LINE__);
        test_control_return(1);
    }

    /* GetEndpointStatus for existing endpoint to check if clear is done */
    transfer_request -> ux_transfer_request_function =          UX_GET_STATUS;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_ENDPOINT;
    transfer_request -> ux_transfer_request_index =             0x02;
    transfer_request -> ux_transfer_request_requested_length =  2;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetEndpointStatus(0x02) fail\n", __LINE__);
        test_control_return(1);
    }
    if (buffer[0] != 0)
    {

        printf("ERROR #%d: GetEndpointStatus(0x02) should clear\n", __LINE__);
        test_control_return(1);

    }

    /* Write, should return OK. */
    status = test_ux_host_class_cdc_acm_write(cdc_acm_host_data, "TEST\n", 5, &actual_length);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: Bulk OUT status %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test ClearEndpointFeature(0x05)\n");

    /* ClearEndpointFeature on not existing endpoint */
    transfer_request -> ux_transfer_request_function =          UX_CLEAR_FEATURE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_ENDPOINT;
    transfer_request -> ux_transfer_request_index =             0x05;
    transfer_request -> ux_transfer_request_requested_length =  0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: GetEndpointStatus(0x05) must fail\n", __LINE__);
        test_control_return(1);
    }

    /* ClearDeviceFeature */
    transfer_request -> ux_transfer_request_function =          UX_CLEAR_FEATURE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_index =             0;
    transfer_request -> ux_transfer_request_requested_length =  0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: ClearDeviceFeature() fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test ClearInterfaceFeature(0x00)\n");

    /* ClearInterfaceFeature, may or may not implement */
    transfer_request -> ux_transfer_request_function =          UX_CLEAR_FEATURE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_index =             0;
    transfer_request -> ux_transfer_request_requested_length =  0;
    status = ux_host_stack_transfer_request(transfer_request);

    stepinfo(">>>>>>>>>>>>>>>> Test GetConfiguration\n");
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  1;
    transfer_request -> ux_transfer_request_function =          UX_GET_CONFIGURATION;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             0;
    /* GetConfiguration should OK */
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetConfiguration fail\n", __LINE__);
        test_control_return(1);
    }
    if (buffer[0] != 1)
    {

        printf("ERROR #%d: GetConfiguration should return 1\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test SetConfiguration\n");
    transfer_request -> ux_transfer_request_data_pointer =      UX_NULL;
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_SET_CONFIGURATION;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             1;
    transfer_request -> ux_transfer_request_index =             0;
    /* SetConfiguration should OK */
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetConfiguration fail\n", __LINE__);
        test_control_return(1);
    }

    /* SetConfiguration to invalid should fail */
    transfer_request -> ux_transfer_request_value =             10;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: SetConfiguration(10) must fail\n", __LINE__);
        test_control_return(1);
    }

    /* SetConfiguration */
    transfer_request -> ux_transfer_request_value =             3;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetConfiguration(3) must pass\n", __LINE__);
        test_control_return(1);
    }

    /* SetConfiguration */
    transfer_request -> ux_transfer_request_value =             4;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetConfiguration(4) must pass\n", __LINE__);
        test_control_return(1);
    }

    /* Stop reading on device side */
    cdc_acm_slave_reading = UX_FALSE;

    stepinfo(">>>>>>>>>>>>>>>> Test SetInterface/GetInterface\n");
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  0;

    /* Unonfiguration */
    transfer_request -> ux_transfer_request_function =          UX_SET_CONFIGURATION;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetConfiguration(0) fail\n", __LINE__);
        test_control_return(1);
    }


    /* SetInterface must report error */
    transfer_request -> ux_transfer_request_function =          UX_SET_INTERFACE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: SetInterface must fail when not configured\n", __LINE__);
        test_control_return(1);
    }

    /* GetInterface must report error */
    transfer_request -> ux_transfer_request_requested_length =  1;
    transfer_request -> ux_transfer_request_function =          UX_GET_INTERFACE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: GetInterface must fail when not configured\n", __LINE__);
        test_control_return(1);
    }

    /* Back to normal configuration */
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_SET_CONFIGURATION;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             1;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetConfiguration(1) fail\n", __LINE__);
        test_control_return(1);
    }

    transfer_request -> ux_transfer_request_function =          UX_SET_INTERFACE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;

    /* Good interface */
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetInterface(0.0) should be OK\n", __LINE__);
        test_control_return(1);
    }

    /* Get interface */
    transfer_request -> ux_transfer_request_requested_length =  1;
    transfer_request -> ux_transfer_request_function =          UX_GET_INTERFACE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetInterface fail %x\n", __LINE__, status);
        test_control_return(1);
    }
    if (buffer[0] != 0)
    {

        printf("ERROR #%d: GetInterface must return 0 but not %x\n", __LINE__, buffer[0]);
        test_control_return(1);
    }

    /* Invalid interface */
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_SET_INTERFACE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;

    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             2;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: SetInterface must fail\n", __LINE__);
        test_control_return(1);
    }

    /* Invalid alternate setting */
    transfer_request -> ux_transfer_request_value =             2;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: SetInterface should fail\n", __LINE__);
        test_control_return(1);
    }

    /* SetConfiguration(2) */
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_SET_CONFIGURATION;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             2;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetConfiguration(2) fail\n", __LINE__);
        test_control_return(1);
    }

    /* SetInterface(2, 0, 1) */
    transfer_request -> ux_transfer_request_function =          UX_SET_INTERFACE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             1;
    transfer_request -> ux_transfer_request_index =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetInterface(0,1) should be OK\n", __LINE__);
        test_control_return(1);
    }

    /* GetInterface(2, 0) */
    transfer_request -> ux_transfer_request_requested_length =  1;
    transfer_request -> ux_transfer_request_function =          UX_GET_INTERFACE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetInterface must be OK\n", __LINE__);
        test_control_return(1);
    }
    if (buffer[0] != 1)
    {

        printf("ERROR #%d: GetInterface must return 1 but not %x\n", __LINE__, buffer[0]);
        test_control_return(1);
    }

    /* GetInterface(2, 1) */
    transfer_request -> ux_transfer_request_index =             1;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetInterface must be OK\n", __LINE__);
        test_control_return(1);
    }
    if (buffer[0] != 0)
    {

        printf("ERROR #%d: GetInterface must return 0 but not %x\n", __LINE__, buffer[0]);
        test_control_return(1);
    }

    /* SetInterface(1, 1), OK or STALL due to no class driver on device side. */
    transfer_request -> ux_transfer_request_function =          UX_SET_INTERFACE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             1;
    transfer_request -> ux_transfer_request_index =             1;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {

        printf("ERROR #%d: SetInterface(1,1) status %x\n", __LINE__, status);
        test_control_return(1);
    }

    /* SetInterface(1, 2), OK or STALL due to no class driver on device side. */
    transfer_request -> ux_transfer_request_value =             2;
    transfer_request -> ux_transfer_request_index =             1;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {

        printf("ERROR #%d: SetInterface(1,2) status %x\n", __LINE__, status);
        test_control_return(1);
    }

    /* SetInterface(1, 0), OK or STALL due to no class driver on device side. */
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             1;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {

        printf("ERROR #%d: SetInterface(1,0) status %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test SetDescriptor\n");
    transfer_request -> ux_transfer_request_data_pointer =      UX_NULL;
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_SET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             0;
    /* SetDescriptor should OK or STALL */
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {

        printf("ERROR #%d: SetDescriptor status %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test SyncFrame\n");
    transfer_request -> ux_transfer_request_data_pointer =      UX_NULL;
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_SYNCH_FRAME;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             0;
    /* SyncFrame should OK or STALL */
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {

        printf("ERROR #%d: SyncFrame status %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test Unknown Request\n");
    /* Unknown request should STALL */
    transfer_request -> ux_transfer_request_function =          20;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_TRANSFER_STALLED)
    {

        printf("ERROR #%d: Unknown request status %x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test Class Request\n");
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_CDC_ACM_REQ_GET_LINE_CODING;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_DEVICE;
    /* Check DCD transfer call */
    error_counter = 0;
    ux_test_dcd_sim_slave_set_actions(dcd_transfer_is_called);
    /* Process request */
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: Class request status %x\n", __LINE__, status);
        test_control_return(1);
    }
    if (error_counter == 0)
    {

        printf("ERROR #%d: Class request no DCD transfer\n", __LINE__);
        test_control_return(1);
    }

    class_entry_rc = UX_ERROR;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_TRANSFER_STALLED)
    {

        printf("ERROR #%d: Vendor request status %x\n", __LINE__, status);
        test_control_return(1);
    }

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

    stepinfo(">>>>>>>>>>>>>>>> ux_slave_transfer_request_status_phase_ignore\n");
    /* DCD should not been called */
    slave_transfer_request->ux_slave_transfer_request_status_phase_ignore = UX_TRUE;
    error_counter = 0;
    ux_test_dcd_sim_slave_set_actions(dcd_transfer_is_called);
    status = _ux_device_stack_transfer_request(slave_transfer_request, 18, 18);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: Request not success: %x\n", __LINE__, status);
        test_control_return(1);
    }
    if (error_counter != 0)
    {

        printf("ERROR #%d: Unexpected call\n", __LINE__);
        test_control_return(1);
    }
    slave_transfer_request->ux_slave_transfer_request_status_phase_ignore = UX_FALSE;


    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);

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
        while(cdc_acm_slave != UX_NULL && cdc_acm_slave_reading)
        {

            status = ux_device_class_cdc_acm_read(cdc_acm_slave, buffer, 64, &actual_length);

            if (status == UX_SUCCESS && actual_length)
            {

                if (ux_utility_memory_compare("TSTALL\n", buffer, 7) == UX_SUCCESS)
                {

                    status = test_ux_device_class_cdc_acm_read_halt(cdc_acm_slave);
                    if (status != UX_SUCCESS)
                    {

                        printf("ERROR #%d: set halt fail: %x\n", __LINE__, status);
                        test_control_return(1);
                    }
                }
            }
        }

        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
    }
}
