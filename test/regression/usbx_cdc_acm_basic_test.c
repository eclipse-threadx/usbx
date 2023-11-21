/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"

#include "fx_api.h"

#include "ux_device_class_cdc_acm.h"
#include "ux_device_stack.h"
#include "ux_host_class_cdc_acm.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

#define                             UX_DEMO_REQUEST_MAX_LENGTH \
    ((UX_HCD_SIM_HOST_MAX_PAYLOAD) > (UX_SLAVE_REQUEST_DATA_MAX_LENGTH) ? \
        (UX_HCD_SIM_HOST_MAX_PAYLOAD) : (UX_SLAVE_REQUEST_DATA_MAX_LENGTH))

/* Define constants.  */
#define                             UX_CDC_ACM_CONNECTION_DELAY ((UX_RH_ENUMERATION_RETRY + 1)*UX_HOST_CLASS_CDC_ACM_DEVICE_INIT_DELAY)
#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024
#define                             UX_DEMO_BUFFER_SIZE (UX_DEMO_REQUEST_MAX_LENGTH + 1)
#define                             UX_DEMO_XMIT_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE 512
#define                             UX_DEMO_FILE_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE 64
#define                             UX_DEMO_MEMORY_SIZE     (128*1024)
#define                             UX_DEMO_FILE_SIZE       (128 * 1024)
#define                             UX_RAM_DISK_MEMORY      (256 * 1024)

/* Define local/extern function prototypes.  */
static void        test_thread_entry(ULONG);
static TX_THREAD   tx_test_thread_host_simulation;
static TX_THREAD   tx_test_thread_slave_simulation;
static void        tx_test_thread_host_simulation_entry(ULONG);
static void        tx_test_thread_slave_simulation_entry(ULONG);
static void        test_thread_host_reception_callback(UX_HOST_CLASS_CDC_ACM *cdc_acm, UINT status, UCHAR *reception_buffer, ULONG reception_size);
static VOID        demo_cdc_instance_activate(VOID  *cdc_instance);
static VOID        demo_cdc_instance_deactivate(VOID *cdc_instance);
static VOID        demo_cdc_instance_parameter_change(VOID *cdc_instance);
static UINT        test_usbx_simulator_cdc_acm_host_send_command(UCHAR *string, ULONG length, ULONG no_ack);
static UINT        tx_test_thread_slave_simulation_response(UCHAR *string, ULONG length);

static UINT        ux_test_host_class_cdc_acm_command(UX_HOST_CLASS_CDC_ACM *cdc_acm, ULONG command, ULONG value, UCHAR *data_buffer, ULONG data_length, ULONG *actual_length);

static VOID        ux_test_host_class_cdc_acm_device_status_change_callback(struct UX_HOST_CLASS_CDC_ACM_STRUCT *cdc_acm,
                                                                            ULONG  notification_type, ULONG notification_value);

static VOID        ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);
static VOID        ux_test_hcd_entry_interaction_request_sem_put(UX_TEST_ACTION *action, VOID *params);
static VOID        ux_test_hcd_entry_interaction_invoked(UX_TEST_ACTION *action, VOID *params);
static VOID        ux_test_hcd_entry_interaction_wait_transfer_disconnection(UX_TEST_ACTION *action, VOID *params);

#define test_usbx_simulator_cdc_acm_host_send_at_command(s,l) test_usbx_simulator_cdc_acm_host_send_command(s,l,UX_FALSE)
#define test_usbx_simulator_cdc_acm_host_send_string(s,l)     test_usbx_simulator_cdc_acm_host_send_command(s,l,UX_TRUE)

/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UX_HOST_CLASS                       *class_driver;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_control;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_data;
static ULONG                               notification_count;
static ULONG                               command_received_count;
static UCHAR                               cdc_acm_reception_buffer[UX_DEMO_RECEPTION_BUFFER_SIZE];
static UCHAR                               cdc_acm_xmit_buffer[UX_DEMO_XMIT_BUFFER_SIZE];
static UX_HOST_CLASS_CDC_ACM_RECEPTION     cdc_acm_reception;
static UCHAR                               *global_reception_buffer;
static ULONG                               global_reception_size;
static UCHAR                               cdc_acm_reception_overflow = UX_FALSE;

static UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave;
static UCHAR                               cdc_acm_slave_change;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER    parameter;
static UCHAR                               buffer[UX_DEMO_BUFFER_SIZE * 4]; /* Large enough to avoid memory access exception.  */
static UCHAR                               cdc_acm_slave_bulk_read_write = UX_TRUE;

static ULONG                               error_counter;

static ULONG                               set_cfg_counter;

static ULONG                               rsc_mem_free_on_set_cfg;
static ULONG                               rsc_sem_on_set_cfg;
static ULONG                               rsc_sem_get_on_set_cfg;
static ULONG                               rsc_mutex_on_set_cfg;

static ULONG                               rsc_enum_sem_usage;
static ULONG                               rsc_enum_sem_get_count;
static ULONG                               rsc_enum_mutex_usage;
static ULONG                               rsc_enum_mem_usage;

static ULONG                               rsc_cdc_sem_usage;
static ULONG                               rsc_cdc_sem_get_count;
static ULONG                               rsc_cdc_mutex_usage;
static ULONG                               rsc_cdc_mem_usage;

static ULONG                               interaction_count;

static UCHAR                               error_callback_ignore = UX_TRUE;
static ULONG                               error_callback_counter;

static UCHAR                                _rsp_ok[UX_DEMO_BUFFER_SIZE] = {'O', 'K', '\r', '\n', '\0'};

/* Define device framework.  */

#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      (93 + 7)
#define             DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED      (103 + 7)
#define             STRING_FRAMEWORK_LENGTH                 47
#define             LANGUAGE_ID_FRAMEWORK_LENGTH            2

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
    0x01,

    /* Configuration 1 descriptor 9 bytes */
    0x09, 0x02, 0x52, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,
    0x00,
    0x02,
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

    /* Endpoint 0x04 descriptor 7 bytes */
    0x07, 0x05, 0x04,
    0x03,
    0x08, 0x00,
    15,

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

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02, /* @ 93 - 14 + 2 = 81 */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81, /* @ 93 - 7 + 2 = 88 */
    0x02,
    0x40, 0x00,
    0x00,

};

#define DEVICE_FRAMEWORK_EPA_POS_1_FS (DEVICE_FRAMEWORK_LENGTH_FULL_SPEED - 14 + 2)
#define DEVICE_FRAMEWORK_EPA_POS_2_FS (DEVICE_FRAMEWORK_LENGTH_FULL_SPEED - 7 + 2)

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
    0x01,

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02,
    0x02, 0x00, 0x00,
    0x40,
    0x01,
    0x00,

    /* Configuration 1 descriptor */
    0x09, 0x02, 0x52, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement */
    0x09, 0x04, 0x00,
    0x00,
    0x02,
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

    /* Endpoint 0x04 descriptor */
    0x07, 0x05, 0x04,
    0x03,
    0x08, 0x00,
    10,

    /* Endpoint 0x83 descriptor */
    0x07, 0x05, 0x83,
    0x03,
    0x08, 0x00,
    10,

    /* Data Class Interface Descriptor Requirement */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor */
    0x07, 0x05, 0x02, /* @ 103 - 14 + 2 = 91 */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor */
    0x07, 0x05, 0x81, /* @ 103 - 7 + 2 = 98 */
    0x02,
    0x40, 0x00,
    0x00,

};

#define DEVICE_FRAMEWORK_EPA_POS_1_HS (DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED - 14 + 2)
#define DEVICE_FRAMEWORK_EPA_POS_2_HS (DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED - 7 + 2)

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


    /* Multiple languages are supported on the device, to add
       a language besides english, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
static unsigned char language_id_framework[] = {

    /* English. */
        0x09, 0x04
    };

#define DEVICE_FRAMEWORK1_LENGTH            (18 +9 +8 +9+5+4+5+5+ 9+7+7) /* =86 */
#define DEVICE_FRAMEWORK1_CFG_TOTAL_LEN_POS (18+2)
#define DEVICE_FRAMEWORK1_IFC1_N_EPS_POS    (86-7-7-9+4)
#define DEVICE_FRAMEWORK1_IFC1_EPA1_POS     (86-7-7+2)
#define DEVICE_FRAMEWORK1_IFC1_EPA2_POS     (86-7+2)

static unsigned char device_framework_no_interrupt_ep[] = {

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
    0x01, 0x02, 0x03,
    0x01,

    /* Configuration 1 descriptor 9 bytes */
    0x09, 0x02, 0x44, 0x00, /* wTotalLength @ 21 */
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,
    0x00,
    0x00,
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

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x00,
    0x02, /* bNumEndpoints @ 86 - 14 - 9 + 4 = 67 */
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81, /* @ 86 - 14 + 2 = 73 */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02, /* @ 86 - 7 + 2 = 81 */
    0x02,
    0x40, 0x00,
    0x00,

};

static unsigned char replaced_cfg_descriptor[] =
{
    /* Configuration 1 descriptor 9 bytes */
    0x09, 0x02, 0x52, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,
    0x00,
    0x02,
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

    /* Endpoint 0x04 descriptor 7 bytes */
    0x07, 0x05, 0x04,
    0x03,
    0x08, 0x00,
    10,

    /* Endpoint 0x83 descriptor 7 bytes */
    0x07, 0x05, 0x83,
    0x03,
    0x08, 0x00,
    10,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02, /* @ 93 - 14 + 2 = 81 */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81, /* @ 93 - 7 + 2 = 88 */
    0x02,
    0x40, 0x00,
    0x00,
};

static unsigned char replaced_cfg_descriptor_no_bulk[] =
{
    /* Configuration 1 descriptor */
    0x09, 0x02, 0x52 - 7, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement */
    0x09, 0x04, 0x00,
    0x00,
    0x02,
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

    /* Endpoint 0x04 descriptor */
    0x07, 0x05, 0x04,
    0x03,
    0x08, 0x00,
    10,

    /* Endpoint 0x83 descriptor */
    0x07, 0x05, 0x83,
    0x03,
    0x08, 0x00,
    10,

    /* Data Class Interface Descriptor Requirement */
    0x09, 0x04, 0x01,
    0x00,
    0x01,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor */
    0x07, 0x05, 0x02, /* @ - 7 + 2 */
    0x02,
    0x40, 0x00,
    0x00,

};

/* Setup requests */

static UX_TEST_SETUP _SetConfigure = UX_TEST_SETUP_SetConfigure;
static UX_TEST_SETUP _GetCfgDescr  = UX_TEST_SETUP_GetCfgDescr;
static UX_TEST_SETUP _SetAddress = UX_TEST_SETUP_SetAddress;
static UX_TEST_SETUP _GetDeviceDescriptor = UX_TEST_SETUP_GetDevDescr;
static UX_TEST_SETUP _GetConfigDescriptor = UX_TEST_SETUP_GetCfgDescr;

/* Interaction define */

static UX_TEST_HCD_SIM_ACTION check_ignore_next_transfer_request[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        0, 0, UX_NULL, 0, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   0   }
};

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

static UX_TEST_HCD_SIM_ACTION wait_disconn_on_transfer_0[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        0, 0, UX_NULL, 0, 0,
        UX_ERROR, ux_test_hcd_entry_interaction_wait_transfer_disconnection},
{   0   }
};

static UX_TEST_HCD_SIM_ACTION error_on_transfer_0[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER, 0, UX_NULL, 0, UX_ERROR,
        UX_ERROR},
{   0   }
};

static UX_TEST_HCD_SIM_ACTION error_on_transfer_1[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER, 0, UX_NULL, ~0, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked}, /* All requested */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER, 0, UX_NULL, 0, UX_ERROR,
        UX_ERROR, ux_test_hcd_entry_interaction_invoked}, /* Error */
{   0   }
};

static UX_TEST_HCD_SIM_ACTION good_on_transfer_1[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER, 0, UX_NULL, ~0, 0, /* Return all required */
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked}, /* All requested */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER, 0, UX_NULL, 0, 0, /* Return ZLP */
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked}, /* No Error */
{   0   }
};

static UX_TEST_HCD_SIM_ACTION good_on_transfer_0_ZLP[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER, 0, UX_NULL, 0, 0, /* Return ZLP */
        UX_SUCCESS, ux_test_hcd_entry_interaction_request_sem_put}, /* No Error */
{   0   }
};

static UX_TEST_HCD_SIM_ACTION error_on_transfer_interruptEP[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        UX_TEST_MATCH_EP, 0x83, UX_NULL, 0, 0,
        UX_ERROR, ux_test_hcd_entry_interaction_invoked}, /* Error */
{   0   }
};

static UX_TEST_HCD_SIM_ACTION replaced_GetCfgDescr[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_GetCfgDescr,
        UX_FALSE, 0,
        UX_TEST_SETUP_MATCH_REQ | UX_TEST_SIM_REQ_ANSWER, 0, replaced_cfg_descriptor, 9, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked}, /* Invoke callback & answer */
{   UX_HCD_TRANSFER_REQUEST, &_GetCfgDescr,
        UX_FALSE, 0,
        UX_TEST_SETUP_MATCH_REQ | UX_TEST_SIM_REQ_ANSWER, 0, replaced_cfg_descriptor, sizeof(replaced_cfg_descriptor), 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked}, /* Invoke callback & answer */
{   0   }
};

static UX_TEST_HCD_SIM_ACTION enum_replace_no_bulk[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_GetDeviceDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, device_framework_full_speed + 0, 8, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   UX_HCD_TRANSFER_REQUEST, &_GetDeviceDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, device_framework_full_speed + 0, 18, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},

        /* Note: Each enumeration does two GetConfigurations (the second one is in _ux_host_class_cdc_acm_capabilities_get.c) */

        /* 1st. */
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, UX_CONFIGURATION_DESCRIPTOR_LENGTH, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, sizeof(replaced_cfg_descriptor_no_bulk), 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, UX_CONFIGURATION_DESCRIPTOR_LENGTH, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, sizeof(replaced_cfg_descriptor_no_bulk), 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},

        /* 2nd. */
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, UX_CONFIGURATION_DESCRIPTOR_LENGTH, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, sizeof(replaced_cfg_descriptor_no_bulk), 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, UX_CONFIGURATION_DESCRIPTOR_LENGTH, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, sizeof(replaced_cfg_descriptor_no_bulk), 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},

        /* 3rd. */
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, UX_CONFIGURATION_DESCRIPTOR_LENGTH, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, sizeof(replaced_cfg_descriptor_no_bulk), 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, UX_CONFIGURATION_DESCRIPTOR_LENGTH, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, replaced_cfg_descriptor_no_bulk, sizeof(replaced_cfg_descriptor_no_bulk), 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_invoked},
{   0   }
};

/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);

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

static UINT  sleep_break_on_error(VOID)
{

    if (error_callback_counter >= 3)
        return error_callback_counter;

    return UX_SUCCESS;
}

/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}


static UINT demo_class_cdc_acm_get(void)
{

UINT                                status;
UX_HOST_CLASS                       *class;
UX_HOST_CLASS_CDC_ACM               *cdc_acm_host;


    /* Find the main cdc_acm container */
    status =  ux_host_stack_class_get(_ux_system_host_class_cdc_acm_name, &class);
    if (status != UX_SUCCESS)
        return(status);

    /* We get the first instance of the cdc_acm device */
    do
    {

        status =  ux_host_stack_class_instance_get(class, 0, (void **) &cdc_acm_host);
        tx_thread_sleep(10);
    } while (status != UX_SUCCESS);

    /* We still need to wait for the cdc_acm status to be live */
    while (cdc_acm_host -> ux_host_class_cdc_acm_state != UX_HOST_CLASS_INSTANCE_LIVE)
        tx_thread_sleep(10);

    /* Isolate both the control and data interfaces.  */
    if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_DATA_CLASS)
    {
        /* This is the data interface.  */
        cdc_acm_host_data = cdc_acm_host;

        /* In that case, the second one should be the control interface.  */
        status =  ux_host_stack_class_instance_get(class, 1, (void **) &cdc_acm_host);

        /* Check error.  */
        if (status != UX_SUCCESS)
            return(status);

        /* Check for the control interfaces.  */
        if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
        {

            /* This is the control interface.  */
            cdc_acm_host_control = cdc_acm_host;

            return(UX_SUCCESS);

        }
    }
    else
    {
        /* Check for the control interfaces.  */
        if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
        {

            /* This is the control interface.  */
            cdc_acm_host_control = cdc_acm_host;

            /* In that case, the second one should be the data interface.  */
            status =  ux_host_stack_class_instance_get(class, 1, (void **) &cdc_acm_host);

            /* Check error.  */
            if (status != UX_SUCCESS)
                return(status);

            /* Check for the data interface.  */
            if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_DATA_CLASS)
            {

                /* This is the data interface.  */
                cdc_acm_host_data = cdc_acm_host;

                return(UX_SUCCESS);

            }
        }
    }

    /* Return ERROR.  */
    return(UX_ERROR);
}

static UINT demo_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
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

static VOID    demo_cdc_instance_activate(VOID *cdc_instance)
{

    /* Save the CDC instance.  */
    cdc_acm_slave = (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;
}
static VOID    demo_cdc_instance_deactivate(VOID *cdc_instance)
{

    /* Reset the CDC instance.  */
    cdc_acm_slave = UX_NULL;
}

static VOID demo_cdc_instance_parameter_change(VOID *cdc_instance)
{

    /* Set CDC parameter change flag. */
    cdc_acm_slave_change = UX_TRUE;
}

static VOID test_swap_framework_bulk_ep_descriptors(VOID)
{
UCHAR tmp;

    tmp = device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_1_FS];
    device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_1_FS] = device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_2_FS];
    device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_2_FS] = tmp;

    tmp = device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_1_HS];
    device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_1_HS] = device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_2_HS];
    device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_2_HS] = tmp;
}

static VOID test_slave_cdc_acm_transfer_disconnect(UX_SLAVE_CLASS_CDC_ACM *cdc_acm, ULONG ep_dir)
{

UX_SLAVE_ENDPOINT           *endpoint;
UX_SLAVE_DEVICE             *device;
UX_SLAVE_INTERFACE          *interface;
UX_SLAVE_TRANSFER           *transfer_request;

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* This is the first time we are activated. We need the interface to the class.  */
    interface =  cdc_acm -> ux_slave_class_cdc_acm_interface;

    /* Locate the endpoints.  */
    endpoint =  interface -> ux_slave_interface_first_endpoint;

    /* Check the endpoint direction, if OUT we have the correct endpoint.  */
    if ((endpoint -> ux_slave_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) != ep_dir)
    {

        /* So the next endpoint has to be the OUT endpoint.  */
        endpoint =  endpoint -> ux_slave_endpoint_next_endpoint;
    }

    /* All CDC reading  are on the endpoint OUT, from the host.  */
    transfer_request =  &endpoint -> ux_slave_endpoint_transfer_request;

    /* Continue transfer. */
    transfer_request -> ux_slave_transfer_request_actual_length = endpoint -> ux_slave_endpoint_descriptor.wMaxPacketSize;

    /* Change device state. */
    device -> ux_slave_device_state = UX_DEVICE_ATTACHED;

    /* Inform hcd. */
    _ux_utility_semaphore_put(&transfer_request -> ux_slave_transfer_request_semaphore);

    /* Wait a while for transfer request handling. */
    tx_thread_sleep(50);

    /* Change device state. */
    device -> ux_slave_device_state = UX_DEVICE_CONFIGURED;
}

static VOID ux_test_hcd_entry_interaction_wait_transfer_disconnection(UX_TEST_ACTION *action, VOID *_params)
{
UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *params = _params;
UX_TRANSFER                         *transfer_request = (UX_TRANSFER *)params->parameter;
UX_ENDPOINT                         *endpoint;
UX_DEVICE                           *device;

    endpoint = transfer_request -> ux_transfer_request_endpoint;
    device = endpoint -> ux_endpoint_device;

    while(device -> ux_device_state > UX_DEVICE_RESET)
        tx_thread_sleep(10);
}

static VOID ux_test_hcd_entry_interaction_invoked(UX_TEST_ACTION *action, VOID *params)
{

    interaction_count ++;
}

static VOID ux_test_hcd_entry_interaction_request_sem_put(UX_TEST_ACTION *action, VOID *params)
{

    interaction_count ++;

}

static VOID ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params)
{

    set_cfg_counter ++;

    rsc_mem_free_on_set_cfg = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    rsc_sem_on_set_cfg = ux_test_utility_sim_sem_create_count();
    rsc_mutex_on_set_cfg = ux_test_utility_sim_mutex_create_count();
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_cdc_acm_basic_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;
ULONG                   test_n;

    /* Inform user.  */
    printf("Running CDC ACM Basic Functionality Test............................ ");

    /* Reset testing counts. */
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

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #1\n");
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(demo_system_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #2\n");
        test_control_return(1);
    }

    /* Register CDC-ACM class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_cdc_acm_name, ux_host_class_cdc_acm_entry);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #3\n");
        test_control_return(1);
    }

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #5\n");
        test_control_return(1);
    }

    /* Set the parameters for callback when insertion/extraction of a CDC device.  */
    parameter.ux_slave_class_cdc_acm_instance_activate   =  demo_cdc_instance_activate;
    parameter.ux_slave_class_cdc_acm_instance_deactivate =  demo_cdc_instance_deactivate;
    parameter.ux_slave_class_cdc_acm_parameter_change    =  demo_cdc_instance_parameter_change;

    /* Mutex will be created on initialize to protect CDC Bulk IN/OUT */
    for (test_n = 0; test_n < 2; test_n ++)
    {
        ux_test_utility_sim_mutex_error_generation_start(test_n);
        status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                                 1,0,  &parameter);
        /* Mutex error should be reported */
        if(status != UX_MUTEX_ERROR)
        {

            printf("ERROR #46.%ld\n", test_n);
            test_control_return(1);
        }
    }
    ux_test_utility_sim_mutex_error_generation_stop();

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1,0,  &parameter);

    if(status!=UX_SUCCESS)
    {

        printf("ERROR #6\n");
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #7\n");
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #4\n");
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_test_thread_host_simulation, "tx demo host simulation", tx_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #8\n");
        test_control_return(1);
    }

    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&tx_test_thread_slave_simulation, "tx demo slave simulation", tx_test_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #9\n");
        test_control_return(1);
    }
}

void  tx_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
UX_HOST_CLASS_CDC_ACM_LINE_CODING                   line_coding_host;
UX_HOST_CLASS_CDC_ACM_LINE_STATE                    line_state_host;
ULONG                                               actual_length;
UCHAR                                               at_cmd[16];
UX_SLAVE_CLASS_CDC_ACM *                            cdc_acm_slave_bak;
UX_HOST_CLASS_CDC_ACM *                             cdc_acm_host_ctrl_bak;
UX_HOST_CLASS_CDC_ACM *                             cdc_acm_host_data_bak;
ULONG                                               test_n;
UX_HOST_CLASS_COMMAND                               command;
UX_HOST_CLASS_COMMAND                               command1;
ULONG                                               mem_free;

    stepinfo("\n");

    /* Find the cdc_acm class and wait for the link to be up.  */
    status =  demo_class_cdc_acm_get();
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #10\n");
        test_control_return(1);
    }

    /* Reception parameter */
    cdc_acm_reception.ux_host_class_cdc_acm_reception_block_size = UX_DEMO_RECEPTION_BLOCK_SIZE;
    cdc_acm_reception.ux_host_class_cdc_acm_reception_data_buffer = cdc_acm_reception_buffer;
    cdc_acm_reception.ux_host_class_cdc_acm_reception_data_buffer_size = UX_DEMO_RECEPTION_BUFFER_SIZE;
    cdc_acm_reception.ux_host_class_cdc_acm_reception_callback = test_thread_host_reception_callback;

    /* Save slave instance for later tests. */
    cdc_acm_slave_bak = cdc_acm_slave;
    /* Save host instances for later tests. */
    cdc_acm_host_ctrl_bak = cdc_acm_host_control;
    cdc_acm_host_data_bak = cdc_acm_host_data;

    /* Test disconnect. */
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();

    /* Test reception on instance inactive */
    stepinfo(">>>>>>>>>>>> Start reception when interface is inactive\n");
    status = ux_host_class_cdc_acm_reception_start(cdc_acm_host_data_bak, &cdc_acm_reception);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #86: error must be reported when invoking reception while interface is not ready\n");
        test_control_return(1);
    }

    /* Test stop reception on inactive interface */
    stepinfo(">>>>>>>>>>>> Stop reception when interface is inactive\n");
    status = ux_host_class_cdc_acm_reception_stop(cdc_acm_host_data_bak, &cdc_acm_reception);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #91: error not reported when stop reception on inactive interface\n");
        test_control_return(1);
    }

    /* Reset testing counts. */
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_hcd_sim_host_set_actions(log_on_SetCfg);
    /* Save free memory usage. */
    mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    tx_thread_sleep(100);
    /* Log create counts for further tests. */
    rsc_enum_mutex_usage = rsc_mutex_on_set_cfg;
    rsc_enum_sem_usage = rsc_sem_on_set_cfg;
    rsc_enum_mem_usage = mem_free - rsc_mem_free_on_set_cfg;
    /* Log create counts when instances active for further tests. */
    rsc_cdc_mutex_usage = ux_test_utility_sim_mutex_create_count() - rsc_enum_mutex_usage;
    rsc_cdc_sem_usage = ux_test_utility_sim_sem_create_count() - rsc_enum_sem_usage;
    rsc_cdc_mem_usage = rsc_mem_free_on_set_cfg - _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);

    /* Start the reception on control interface.  */
    stepinfo(">>>>>>>>>>>> Start reception on control interface\n");
    status = ux_host_class_cdc_acm_reception_start(cdc_acm_host_control, &cdc_acm_reception);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #87: error must be reported when invoking reception on control interface\n");
        test_control_return(1);
    }

    /* Start and stop immediately.  */
    stepinfo(">>>>>>>>>>>> Start reception and stop it immediately\n");
    status  = ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_reception);
    status |= ux_host_class_cdc_acm_reception_stop(cdc_acm_host_data, &cdc_acm_reception);
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Transfer error. */
    stepinfo(">>>>>>>>>>>> Start reception request error\n");
    ux_test_hcd_sim_host_set_actions(error_on_transfer_0);
    status = ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_reception);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #88: error must be reported when transfer request error\n");
        test_control_return(1);
    }

    /* Start the reception for test.  */
    status = ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_reception);
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #89: Start reception fail\n");
        test_control_return(1);
    }

    /* Test stop reception on wrong interface */
    status = ux_host_class_cdc_acm_reception_stop(cdc_acm_host_control, &cdc_acm_reception);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #90: error not reported when stop reception on wrong interface\n");
        test_control_return(1);
    }

    /* Stop reception for test */
    status = ux_host_class_cdc_acm_reception_stop(cdc_acm_host_data, &cdc_acm_reception);
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #92: stop reception failed\n");
        test_control_return(1);
    }

    /* Start the reception for cdc_acm.  */
    status = ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_reception);

    /* Get the current data rate.  */
    stepinfo(">>>>>>>>>>>> ATB: get baud\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATB",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #11\n");
        test_control_return(1);
    }

    /* Get the current stop bit rate.  */
    stepinfo(">>>>>>>>>>>> ATS: get stop bits\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATS",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #12\n");
        test_control_return(1);
    }

    /* Get the current parity rate.  */
    stepinfo(">>>>>>>>>>>> ATP: get parity\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATP",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #13\n");
        test_control_return(1);
    }

    /* Get the current data bit rate.  */
    stepinfo(">>>>>>>>>>>> ATD: data bits\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATD",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #14\n");
        test_control_return(1);
    }

    /* Get the current RTS state.  */
    stepinfo(">>>>>>>>>>>> ATR: RTS\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATR",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #15\n");
        test_control_return(1);
    }

    /* Get the current DTR rate.  */
    stepinfo(">>>>>>>>>>>> ATT: DTR\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATT",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #16\n");
        test_control_return(1);
    }

    /* Stop after receive.  */
    stepinfo(">>>>>>>>>>>> Start reception and stop it immediately\n");
    status |= ux_host_class_cdc_acm_reception_stop(cdc_acm_host_data, &cdc_acm_reception);
    status  = ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_reception);
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Read 64 bytes and next device side expect 64 byte */
    stepinfo(">>>>>>>>>>>> ATOP: 64/64\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATOP", 4);
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Set the line coding. */
    stepinfo(">>>>>>>>>>>> ATL: Set LineCoding\n");
    at_cmd[0] = 'A';
    at_cmd[1] = 'T';
    at_cmd[2] = 'L';
    at_cmd[3] = 0x00;
    at_cmd[4] = 0xC2;
    at_cmd[5] = 0x01;
    at_cmd[6] = 0x00; /* 115200: 0x0001C200 */
    at_cmd[7] = UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_STOP_BIT;
    at_cmd[8] = UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_PARITY;
    at_cmd[9] = 7;
    status = test_usbx_simulator_cdc_acm_host_send_at_command(at_cmd,62);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #33\n");
        test_control_return(1);
    }

    /* Change the line coding values.  */
    stepinfo(">>>>>>>>>>>> IOCTRL: SetLineCoding\n");
    line_coding_host.ux_host_class_cdc_acm_line_coding_dter = 9600;
    line_coding_host.ux_host_class_cdc_acm_line_coding_stop_bit = UX_HOST_CLASS_CDC_ACM_LINE_CODING_STOP_BIT_15;
    line_coding_host.ux_host_class_cdc_acm_line_coding_parity = UX_HOST_CLASS_CDC_ACM_LINE_CODING_PARITY_EVEN;
    line_coding_host.ux_host_class_cdc_acm_line_coding_data_bits = 5;

    status = _ux_host_class_cdc_acm_ioctl(cdc_acm_host_control, UX_HOST_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING,
                                &line_coding_host);

    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #17\n");
        test_control_return(1);
    }

    /* Slave should invoke parameter change callback */
    if (cdc_acm_slave_change != UX_TRUE)
    {

        /* CDC ACM basic test error. */
        printf("ERROR #25\n");
        test_control_return(1);
    }
    /* Reset slave change flag */
    cdc_acm_slave_change = UX_FALSE;

    /* Change the line state values.  */
    stepinfo(">>>>>>>>>>>> IOCTRL: SetLineState\n");
    line_state_host.ux_host_class_cdc_acm_line_state_rts = 0;
    line_state_host.ux_host_class_cdc_acm_line_state_dtr = 0;

    status = _ux_host_class_cdc_acm_ioctl(cdc_acm_host_control, UX_HOST_CLASS_CDC_ACM_IOCTL_SET_LINE_STATE,
                                &line_state_host);

    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #18\n");
        test_control_return(1);
    }

    /* Slave should invoke parameter change callback */
    if (cdc_acm_slave_change != UX_TRUE)
    {

        /* CDC ACM basic test error. */
        printf("ERROR #26\n");
        test_control_return(1);
    }
    /* Reset slave change flag */
    cdc_acm_slave_change = UX_FALSE;

    /* Reobtain the cdc acm line values.  */
    /* Get the current data rate.  */
    stepinfo(">>>>>>>>>>>> ATB: baud rate\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATB",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #19\n");
        test_control_return(1);
    }

    /* Get the current stop bit rate.  */
    stepinfo(">>>>>>>>>>>> ATS: stop bits\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATS",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #20\n");
        test_control_return(1);
    }

    /* Get the current parity rate.  */
    stepinfo(">>>>>>>>>>>> ATP: parity\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATP",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #21\n");
        test_control_return(1);
    }

    /* Get the current data bit rate.  */
    stepinfo(">>>>>>>>>>>> ATD: data bits\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATD",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #22\n");
        test_control_return(1);
    }

    /* Get the current RTS state.  */
    stepinfo(">>>>>>>>>>>> ATR: RTS\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATR",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #23\n");
        test_control_return(1);
    }

    /* Test enumeration different descriptors */

    /* Initialize to disconnect */
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();

    /* Test enumeration no interrupt EP */
    stepinfo(">>>>>>>>>>>> Enumerate device wihtout interrupt EP\n");
    _ux_system_slave->ux_system_slave_device_framework_full_speed = device_framework_no_interrupt_ep;
    _ux_system_slave->ux_system_slave_device_framework_length_full_speed = sizeof(device_framework_no_interrupt_ep);
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    ux_utility_delay_ms(UX_CDC_ACM_CONNECTION_DELAY);
    /* Instances should be ready */
    if (!cdc_acm_slave || !cdc_acm_host_control || !cdc_acm_host_data)
    {

        printf("ERROR #57: instance should ready after reconnect without interrupt EP\n");
        test_control_return(1);
    }
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    /* Instances should be removed */
    if (cdc_acm_slave || cdc_acm_host_control || cdc_acm_host_data)
    {

        printf("ERROR #60: instance should removed after disconnect without interrupt EP\n");
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> Enumerate device wihtout one of bulk EP\n");
    /* Pause bulk read/write since there is no bulk EP. */
    cdc_acm_slave_bulk_read_write = UX_FALSE;

    /* Test enumeration no bulk EP */

    ux_test_hcd_sim_host_set_actions(enum_replace_no_bulk);
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    ux_utility_delay_ms(UX_CDC_ACM_CONNECTION_DELAY);
    /* Data instances should not be ready */
    if (cdc_acm_host_data)
    {

        printf("ERROR #58: instance should NOT ready after reconnect without one of bulk EP\n");
        test_control_return(1);
    }
    UX_TEST_ASSERT(ux_test_check_actions_empty());

    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    if (cdc_acm_slave || cdc_acm_host_control)
    {

        printf("ERROR #61: instance should be removed after disconnect without one of bulk EP\n");
        test_control_return(1);
    }

    replaced_cfg_descriptor_no_bulk[sizeof(replaced_cfg_descriptor_no_bulk) - 7 + 2] ^= 0x80;
    ux_test_hcd_sim_host_set_actions(enum_replace_no_bulk);
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    ux_utility_delay_ms(UX_CDC_ACM_CONNECTION_DELAY);
    /* Data instances should not be ready */
    if (cdc_acm_host_data)
    {

        printf("ERROR #59: instance should NOT ready after reconnect without one of bulk EP\n");
        test_control_return(1);
    }

    /* Restore frameworks */
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    _ux_system_slave->ux_system_slave_device_framework_full_speed = device_framework_full_speed;
    _ux_system_slave->ux_system_slave_device_framework_length_full_speed = DEVICE_FRAMEWORK_LENGTH_FULL_SPEED;
    ux_test_hcd_sim_host_set_actions(UX_NULL);
    cdc_acm_slave_bulk_read_write = UX_TRUE;

    /* Swap EP address for different EP sequence. */
    stepinfo(">>>>>>>>>>>> Enumerate device swap bulk EP addresses\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    test_swap_framework_bulk_ep_descriptors();
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    ux_utility_delay_ms(UX_CDC_ACM_CONNECTION_DELAY);
    /* Instances should be ready */
    if (!cdc_acm_slave || !cdc_acm_host_control || !cdc_acm_host_data)
    {

        printf("ERROR #53: instance not ready after reconnect (%p,%p,%p)\n", cdc_acm_slave, cdc_acm_host_control, cdc_acm_host_data);
        test_control_return(1);
    }

    if (cdc_acm_host_control->ux_host_class_cdc_acm_interrupt_endpoint)
    {

        stepinfo(">>>>>>>>>>>> Notification reception test\n");

        /* Set notification callback */
        status = _ux_host_class_cdc_acm_ioctl(cdc_acm_host_control, UX_HOST_CLASS_CDC_ACM_IOCTL_NOTIFICATION_CALLBACK,
                                    (VOID*)ux_test_host_class_cdc_acm_device_status_change_callback);
        if (status != UX_SUCCESS)
        {

            printf("ERROR #55: notification callback set fail\n");
            test_control_return(1);
        }
        interaction_count = 0;
        ux_test_hcd_sim_host_set_actions(check_ignore_next_transfer_request);
        /* Simulate notification! */
        cdc_acm_host_control->ux_host_class_cdc_acm_interrupt_endpoint->ux_endpoint_transfer_request.ux_transfer_request_completion_code = UX_SUCCESS;
        _ux_host_class_cdc_acm_transfer_request_completed(&cdc_acm_host_control->ux_host_class_cdc_acm_interrupt_endpoint->ux_endpoint_transfer_request);
        /* There should be call of transfer to re-start notification monitoring */
        if (interaction_count == 0)
        {

            printf("ERROR #56: notification monitoring not reactivated\n");
            test_control_return(1);
        }

    }

    /* Start the reception for cdc_acm.  */
    status = ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_reception);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #54: reception start error\n");
        test_control_return(1);
    }

    /* Get the current DTR rate.  */
    stepinfo(">>>>>>>>>>>> ATT: get DTR\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATT",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #24\n");
        test_control_return(1);
    }

    /* Try invalid command on IOCTL.  */
    stepinfo(">>>>>>>>>>>> ATF: invalid device IOCTRL command\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATF",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #36\n");
        test_control_return(1);
    }

    /* Try slave ABORT command on IOCTL.  */
    stepinfo(">>>>>>>>>>>> ATA: slave IOCTRL ABORT test\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATA",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #37\n");
        test_control_return(1);
    }

    /* Start reception again if it's aborted */
    ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_reception);
    /* Read until there is no data from slave */
    do
    {
        status = command_received_count;
        tx_thread_sleep(20);
    } while(status != command_received_count);

    /* Try overflow */
    stepinfo(">>>>>>>>>>>> Reception overflow test\n");
    cdc_acm_reception_overflow = UX_TRUE;
    /* Start writing long buffer in device side */
    test_usbx_simulator_cdc_acm_host_send_string("ATO\0", 4);
    /* Waiting overflow detection */
    status = 50;
    while(cdc_acm_reception_overflow && status --)
    {
        tx_thread_sleep(20);
    }
    if (cdc_acm_reception_overflow)
    {

        printf("ERROR #93: Reception overflow not detected\n");
        cdc_acm_reception_overflow = UX_FALSE;
        test_control_return(1);
    }
    /* Continue reception */
    ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_reception);

    /* GetLineCoding with larger buffer size
       response should be OK with correct bytes */
    stepinfo(">>>>>>>>>>>> GetLineCoding with larger buffer than line coding data\n");
    status = ux_test_host_class_cdc_acm_command(cdc_acm_host_control, UX_HOST_CLASS_CDC_ACM_REQ_GET_LINE_CODING,
                                                0, cdc_acm_reception_buffer, UX_HOST_CLASS_CDC_ACM_LINE_CODING_LENGTH + 1,
                                                &actual_length);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #27\n");
        test_control_return(1);
    }
    /* Returned number of bytes should be correct */
    if (actual_length != UX_HOST_CLASS_CDC_ACM_LINE_CODING_LENGTH)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #28\n");
        test_control_return(1);
    }

    /* Host command test */

    /* Undefined command */
    stepinfo(">>>>>>>>>>>> Invalid command\n");
    status = _ux_host_class_cdc_acm_command(cdc_acm_host_control, UX_HOST_CLASS_CDC_ACM_REQ_GET_LINE_CODING + 7,
                                            0, cdc_acm_reception_buffer, UX_HOST_CLASS_CDC_ACM_LINE_CODING_LENGTH + 1);

    /* The device may be extracted after we start sending\receiving.  */
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #29\n");
        test_control_return(1);
    }

    /* Semaphore protection error test */
    stepinfo(">>>>>>>>>>>> Semaphore get error on command\n");
    ux_test_utility_sim_sem_get_error_generation_start(0);
    status = _ux_host_class_cdc_acm_command(cdc_acm_host_control, UX_HOST_CLASS_CDC_ACM_REQ_GET_LINE_CODING,
                                            0, cdc_acm_reception_buffer, UX_HOST_CLASS_CDC_ACM_LINE_CODING_LENGTH);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #64: no error reported on control semaphore error\n");
        test_control_return(1);
    }
    ux_test_utility_sim_sem_get_error_generation_stop();

    /* Host entry test */

    /* Invalid command */
    stepinfo(">>>>>>>>>>>> Host Entry - Invalid command\n");
    command.ux_host_class_command_request =  0xFF;
    status = ux_host_class_cdc_acm_entry(&command);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #62: no error report on invalid command entry\n");
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> Host Entry - Query\n");

    /* Query test - wrong class */
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_QUERY;
    command.ux_host_class_command_usage = UX_HOST_CLASS_COMMAND_USAGE_CSP;
    command.ux_host_class_command_class = 0xFF;
    status = ux_host_class_cdc_acm_entry(&command);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #63: no error report when Query class wrong\n");
        test_control_return(1);
    }

    /* Query test - CTRL & DLC */
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_QUERY;
    command.ux_host_class_command_usage = UX_HOST_CLASS_COMMAND_USAGE_CSP;
    command.ux_host_class_command_class = UX_HOST_CLASS_CDC_CONTROL_CLASS;
    command.ux_host_class_command_subclass = UX_HOST_CLASS_CDC_DLC_SUBCLASS;
    command.ux_host_class_command_iad_class = 0;
    command.ux_host_class_command_iad_subclass = 0;
    status = ux_host_class_cdc_acm_entry(&command);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #64: error report when Query class OK\n");
        test_control_return(1);
    }

    /* Query test - wrong IAD */
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_QUERY;
    command.ux_host_class_command_usage = UX_HOST_CLASS_COMMAND_USAGE_CSP;
    command.ux_host_class_command_class = UX_HOST_CLASS_CDC_CONTROL_CLASS;
    command.ux_host_class_command_subclass = UX_HOST_CLASS_CDC_DLC_SUBCLASS;
    command.ux_host_class_command_iad_class = 0xFE;
    status = ux_host_class_cdc_acm_entry(&command);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #65: no error report when Query class IAD wrong");
        test_control_return(1);
    }

    /* Host class deactivate & activate */
    stepinfo(">>>>>>>>>>>> Host CDC deactivate & activate\n");
    command.ux_host_class_command_container = cdc_acm_host_control->ux_host_class_cdc_acm_interface;
    command.ux_host_class_command_class_ptr = cdc_acm_host_control->ux_host_class_cdc_acm_class;
    command.ux_host_class_command_instance = cdc_acm_host_control;
    command.ux_host_class_command_request =  UX_HOST_CLASS_COMMAND_DEACTIVATE;

    command1.ux_host_class_command_container = cdc_acm_host_data->ux_host_class_cdc_acm_interface;
    command1.ux_host_class_command_class_ptr = cdc_acm_host_data->ux_host_class_cdc_acm_class;
    command1.ux_host_class_command_instance = cdc_acm_host_data;
    command1.ux_host_class_command_request =  UX_HOST_CLASS_COMMAND_DEACTIVATE;

    ux_host_class_cdc_acm_entry(&command);
    /* Instance should be removed */
    if (cdc_acm_host_control)
    {

        printf("ERROR #67: control instance not deactivate\n");
        test_control_return(1);
    }

    ux_host_class_cdc_acm_entry(&command1);
    /* Instance should be removed */
    if (cdc_acm_host_data)
    {

        printf("ERROR #67: control instance not deactivate\n");
        test_control_return(1);
    }

    command.ux_host_class_command_request =  UX_HOST_CLASS_COMMAND_ACTIVATE;
    ux_host_class_cdc_acm_entry(&command);
    /* Instance should be back */
    if (!cdc_acm_host_control)
    {

        printf("ERROR #68: control instance not activate\n");
        test_control_return(1);
    }

    command1.ux_host_class_command_request =  UX_HOST_CLASS_COMMAND_ACTIVATE;
    ux_host_class_cdc_acm_entry(&command1);
    /* Instance should be back */
    if (!cdc_acm_host_data)
    {

        printf("ERROR #68: control instance not activate\n");
        test_control_return(1);
    }

    /* Start the reception for cdc_acm.  */
    status = ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_reception);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #70: reception start error\n");
        test_control_return(1);
    }

    /* Get the current stop bit rate. */
    stepinfo(">>>>>>>>>>>> ATS: stop bits\n");
    status = test_usbx_simulator_cdc_acm_host_send_at_command("ATS",3);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #69\n");
        test_control_return(1);
    }

    /* Sim: slave lost configure while reading pending. */
    stepinfo(">>>>>>>>>>>> Slave lost connection while reading\n");
    ux_utility_memory_set(cdc_acm_xmit_buffer, 0x00, 3);
    status = ux_host_class_cdc_acm_write(cdc_acm_host_data, cdc_acm_xmit_buffer, 128, &actual_length);
    /* Simulate disconnect after first packet sent */
    test_slave_cdc_acm_transfer_disconnect(cdc_acm_slave_bak, UX_ENDPOINT_OUT);
    status = ux_host_class_cdc_acm_write(cdc_acm_host_data, cdc_acm_xmit_buffer, 64, &actual_length);
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    tx_thread_sleep(50);

    /* Sim: slave lost configure while writing pending. */
    stepinfo(">>>>>>>>>>>> Slave lost connection while writing\n");
    status = test_usbx_simulator_cdc_acm_host_send_string("\0",1);
    status = test_usbx_simulator_cdc_acm_host_send_string("ATW",3);
    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #42\n");
        test_control_return(1);
    }
    tx_thread_sleep(20);
    test_slave_cdc_acm_transfer_disconnect(cdc_acm_slave_bak, UX_ENDPOINT_IN);

    /* Now disconnect the device.  */
    ux_test_dcd_sim_slave_disconnect();

    /* Read/write through disconnected slave should return error */
    stepinfo(">>>>>>>>>>>> R/W on deactivated slave interface instance\n");
    /* Try read, error must be returned */
    status = ux_device_class_cdc_acm_read(cdc_acm_slave_bak, buffer, UX_DEMO_BUFFER_SIZE, &actual_length);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #40\n");
        test_control_return(1);

    }

    /* Try write, error must be returned */
    status = ux_device_class_cdc_acm_write(cdc_acm_slave_bak, buffer, UX_DEMO_BUFFER_SIZE, &actual_length);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #41\n");
        test_control_return(1);

    }

    stepinfo(">>>>>>>>>>>> R/W on invalid interface instance\n");

    /* Try host read on control interface, error must be returned. */
    status = ux_host_class_cdc_acm_read(cdc_acm_host_control, buffer, 64, &actual_length);
    if (status != UX_HOST_CLASS_INSTANCE_UNKNOWN)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #47\n");
        test_control_return(1);
    }

    /* Try host write, error must be returned. */
    status = ux_host_class_cdc_acm_write(cdc_acm_host_control, buffer, 64, &actual_length);
    if (status != UX_HOST_CLASS_INSTANCE_UNKNOWN)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #48\n");
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> R/W on deactivated host interface instance\n");

    ux_test_hcd_sim_host_disconnect();
    tx_thread_sleep(50);

    /* Try host read on disconnected interface, error must be returned. */
    status = ux_host_class_cdc_acm_read(cdc_acm_host_data_bak, buffer, 64, &actual_length);
    if (status != UX_HOST_CLASS_INSTANCE_UNKNOWN)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #49\n");
        test_control_return(1);
    }

    /* Try host write, error must be returned. */
    status = ux_host_class_cdc_acm_write(cdc_acm_host_data_bak, buffer, 64, &actual_length);
    if (status != UX_HOST_CLASS_INSTANCE_UNKNOWN)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #43\n");
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> IOCTRL on deactivated host interface instance\n");
    status = ux_host_class_cdc_acm_ioctl(cdc_acm_host_data_bak, 0, UX_NULL);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #73: IOCTRL on deactivated interface should report error\n");
        test_control_return(1);
    }

    ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);
    while(!cdc_acm_host_control || !cdc_acm_host_data)
        tx_thread_sleep(10);

    /* Host IOCTRL tests */

    /* Try invalid IOCTRL command */
    stepinfo(">>>>>>>>>>>> IOCTRL invalid command\n");
    status = ux_host_class_cdc_acm_ioctl(cdc_acm_host_control, 0xFF, UX_NULL);
    if (status != UX_FUNCTION_NOT_SUPPORTED)
    {

        printf("ERROR #74: IOCTRL with invalid command should report UX_FUNCTION_NOT_SUPPORTED\n");
        test_control_return(1);
    }

    /* Try IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_GET_DEVICE_STATUS */
    stepinfo(">>>>>>>>>>>> IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_GET_DEVICE_STATUS\n");
    status = ux_host_class_cdc_acm_ioctl(cdc_acm_host_control, UX_HOST_CLASS_CDC_ACM_IOCTL_GET_DEVICE_STATUS, &test_n);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #75: IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_GET_DEVICE_STATUS should be OK\n");
        test_control_return(1);
    }

    /* Try IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_ABORT_OUT_PIPE */
    stepinfo(">>>>>>>>>>>> IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_ABORT_OUT_PIPE\n");
    status = ux_host_class_cdc_acm_ioctl(cdc_acm_host_data, UX_HOST_CLASS_CDC_ACM_IOCTL_ABORT_OUT_PIPE, UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #76: IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_ABORT_OUT_PIPE should be OK\n");
        test_control_return(1);
    }

    /* Try IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_ABORT_IN_PIPE */
    stepinfo(">>>>>>>>>>>> IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_ABORT_IN_PIPE\n");
    status = ux_host_class_cdc_acm_ioctl(cdc_acm_host_data, UX_HOST_CLASS_CDC_ACM_IOCTL_ABORT_IN_PIPE, UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #77: IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_ABORT_IN_PIPE should be OK\n");
        test_control_return(1);
    }

    /* Try IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_SEND_BREAK */
    stepinfo(">>>>>>>>>>>> IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_SEND_BREAK\n");
    status = ux_host_class_cdc_acm_ioctl(cdc_acm_host_control, UX_HOST_CLASS_CDC_ACM_IOCTL_SEND_BREAK, &test_n);
    /* Not supported by simulator */
    if (status == UX_SUCCESS)
    {

        printf("ERROR #78: IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_SEND_BREAK should fail\n");
        test_control_return(1);
    }

    /* Try IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_GET/SET_LINE_CODING */
    stepinfo(">>>>>>>>>>>> IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_GET/SET_LINE_CODING without enough memory\n");

    /* Use out memories */
    ux_test_utility_sim_mem_allocate_until(UX_HOST_CLASS_CDC_ACM_LINE_CODING_LENGTH);

    /* UX_HOST_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING */
    status = ux_host_class_cdc_acm_ioctl(cdc_acm_host_data, UX_HOST_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING, &test_n);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("ERROR #80: IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING should be fail if no memory\n");
        test_control_return(1);
    }

    /* UX_HOST_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING */
    status = ux_host_class_cdc_acm_ioctl(cdc_acm_host_data, UX_HOST_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING, &line_coding_host);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("ERROR #81: IOCTRL UX_HOST_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING should be fail if no memory\n");
        test_control_return(1);
    }

    /* Free memory */
    ux_test_utility_sim_mem_free_all();

    /* Try host read/write with semaphore error. */
    stepinfo(">>>>>>>>>>>> R/W semaphore error\n");

    /* Try host write, error must be returned. */
    ux_test_utility_sim_sem_get_error_exception_add(UX_NULL, UX_WAIT_FOREVER);
    ux_test_utility_sim_sem_get_error_generation_start(0);

    /* Write small size */
    status = ux_host_class_cdc_acm_write(cdc_acm_host_data, buffer, 64, &actual_length);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #52: write semaphore error not reported\n");
        test_control_return(1);
    }

    /* Read large size */
    status = ux_host_class_cdc_acm_read(cdc_acm_host_data, buffer, UX_DEMO_BUFFER_SIZE, &actual_length);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #51: read semaphore error not reported\n");
        test_control_return(1);
    }
    ux_test_utility_sim_sem_get_error_generation_stop();
    ux_test_utility_sim_sem_get_error_exception_reset();

    /* Try host read/write with transfer error. */
    stepinfo(">>>>>>>>>>>> R/W transfer error\n");

    ux_test_hcd_sim_host_set_actions(error_on_transfer_0);
    status = ux_host_class_cdc_acm_write(cdc_acm_host_data, buffer, 64, &actual_length);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #82: write transfer request error not reported\n");
        test_control_return(1);
    }

    /* Stop reception for test */
    status = ux_host_class_cdc_acm_reception_stop(cdc_acm_host_data, &cdc_acm_reception);

    ux_test_hcd_sim_host_set_actions(error_on_transfer_0);
    /* Read large size */
    status = ux_host_class_cdc_acm_read(cdc_acm_host_data, buffer, UX_DEMO_BUFFER_SIZE, &actual_length);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #83: read transfer request error not reported\n");
        test_control_return(1);
    }

    /* Error on second transfer request */
    ux_test_hcd_sim_host_set_actions(error_on_transfer_1);

    /* Put a semaphore for first transfer request. */
    _ux_utility_semaphore_put(&cdc_acm_host_data->ux_host_class_cdc_acm_bulk_in_endpoint->ux_endpoint_transfer_request.ux_transfer_request_semaphore);

    /* Read large size */
    status = ux_host_class_cdc_acm_read(cdc_acm_host_data, buffer, UX_DEMO_BUFFER_SIZE, &actual_length);
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #84: read transfer request error not reported\n");
        test_control_return(1);
    }
    if (actual_length == 0)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #85: actual length should not be 0\n");
        test_control_return(1);
    }

    /* Flush device. */
    status = ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_reception);
    status = test_usbx_simulator_cdc_acm_host_send_string("ATO0",4);
    status = test_usbx_simulator_cdc_acm_host_send_string("ATO0",4);
    _tx_thread_sleep(10);

    /* Stop reception for read test. */
    status = ux_host_class_cdc_acm_reception_stop(cdc_acm_host_data, &cdc_acm_reception);

    stepinfo(">>>>>>>>>>>> Read transfer good\n");

    /* ATO1: expect return OK */
    status = test_usbx_simulator_cdc_acm_host_send_string("ATO1",4);

    /* Read, expect short package. */
    status = ux_host_class_cdc_acm_read(cdc_acm_host_data, cdc_acm_reception_buffer, 64, &actual_length);
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #86: read transfer request error should not be reported\n");
        test_control_return(1);
    }
    if (actual_length != 2)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #87: actual length should be 2\n");
        test_control_return(1);
    }

    /* ATO1: expect return OK */
    status = test_usbx_simulator_cdc_acm_host_send_string("ATO1",4);

    /* Read, expect exact size. */
    status = ux_host_class_cdc_acm_read(cdc_acm_host_data, cdc_acm_reception_buffer, 2, &actual_length);
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #86: read transfer request error should not be reported\n");
        test_control_return(1);
    }
    if (actual_length != 2)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #87: actual length should be 2\n");
        test_control_return(1);
    }

    /* ATO0: expect ZLP */
    test_usbx_simulator_cdc_acm_host_send_string("ATO0",4);
    status = ux_host_class_cdc_acm_read(cdc_acm_host_data, cdc_acm_reception_buffer, 64, &actual_length);
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #94: read transfer request error should not be reported\n");
        test_control_return(1);
    }
    if (actual_length != 0)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #95: actual length should be 0 but not %ld\n", actual_length);
        test_control_return(1);
    }

#if defined(UX_DEVICE_CLASS_CDC_ACM_WRITE_AUTO_ZLP)
    test_usbx_simulator_cdc_acm_host_send_string("ATO2",4);
    status = ux_host_class_cdc_acm_read(cdc_acm_host_data, cdc_acm_reception_buffer, UX_DEMO_RECEPTION_BUFFER_SIZE, &actual_length);
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #%d: read transfer request error should not be reported\n", __LINE__);
        test_control_return(1);
    }
    if (actual_length != 64)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #%d: actual length should be 64 but not %ld\n", __LINE__, actual_length);
        test_control_return(1);
    }
#endif

    /* Start the reception for cdc_acm.  */
    status = ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_reception);

    /* Swap bulk IN/OUT endpoint position.
       Simulate detach and attach for HS enumeration,
       and test possible mutex creation error handlings.
     */
    if (rsc_cdc_mutex_usage) stepinfo(">>>>>>>>>>>> Enumerate mutex error\n");
    for (test_n = 0; test_n < rsc_cdc_mutex_usage; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_cdc_mutex_usage - 1);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Swap EP address. */
        test_swap_framework_bulk_ep_descriptors();

        /* Generate error while the test_n and after mutex are requested */
        ux_test_utility_sim_mutex_error_generation_start(test_n + rsc_enum_mutex_usage);

        /* Connect. */
        ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);
        tx_thread_sleep(100);

        if (cdc_acm_host_control && cdc_acm_host_data)
        {

            printf("ERROR #97: at least one interface should fail\n");
            test_control_return(1);
        }
    }
    ux_test_utility_sim_mutex_error_generation_stop();

    /* Simulate detach and attach for FS enumeration,
       and test possible semaphore creation error handlings.
     */
    ux_test_utility_sim_sem_get_error_exception_add(&_ux_system_host -> ux_system_host_hcd_semaphore, UX_WAIT_FOREVER);
    if (rsc_cdc_sem_usage) stepinfo(">>>>>>>>>>>> Enumerate semaphore error\n");
    for (test_n = 0; test_n < rsc_cdc_sem_usage; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_cdc_sem_usage - 1);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Generate error while the test_n and after semaphore are requested */
        ux_test_utility_sim_sem_error_generation_start(test_n + rsc_enum_sem_usage);

        /* Connect. */
        error_callback_counter = 0;
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
        #if 0
        tx_thread_sleep(100);
        #else
        /* Wait until error detected. */
        ux_test_breakable_sleep(100, sleep_break_on_error);
        #endif

        if (cdc_acm_host_control && cdc_acm_host_data)
        {

            printf("ERROR #97: at least one interface should fail\n");
            test_control_return(1);
        }
    }
    ux_test_utility_sim_sem_error_generation_stop();
    ux_test_utility_sim_sem_get_error_exception_reset();

    stepinfo(">>>>>>>>>>>> Enumerate interrupt EP transfer request error\n");
    /* Disconnect. */
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    /* Transfer error on interrupt IN 0x83 */
    ux_test_hcd_sim_host_set_actions(error_on_transfer_interruptEP);
    /* Connect. */
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    tx_thread_sleep(100);
    if (cdc_acm_host_control)
    {

        printf("ERROR #96: control interface should fail\n");
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> Capabilities get\n");

    /* Confirm connection */
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    ux_test_hcd_sim_host_set_actions(UX_NULL);
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    test_n = 10;
    while(cdc_acm_host_control == UX_NULL && test_n --)
        tx_thread_sleep(10);
    if (cdc_acm_host_control == UX_NULL)
    {

        printf("ERROR #99: CDC ACM control interface is not ready\n");
        test_control_return(1);
    }

    /* Some of descriptor length is too small */
    replaced_cfg_descriptor[9+8+9+5] = 0;
    ux_test_hcd_sim_host_set_actions(replaced_GetCfgDescr);
    status = _ux_host_class_cdc_acm_capabilities_get(cdc_acm_host_control);
    if (status != UX_DESCRIPTOR_CORRUPTED)
    {

        printf("ERROR #100: descriptor error should be reported\n");
        test_control_return(1);
    }

    /* Some of descriptor length is too large */
    replaced_cfg_descriptor[9+8+9+5] = sizeof(replaced_cfg_descriptor);
    ux_test_hcd_sim_host_set_actions(replaced_GetCfgDescr);
    status = _ux_host_class_cdc_acm_capabilities_get(cdc_acm_host_control);
    if (status != UX_DESCRIPTOR_CORRUPTED)
    {

        printf("ERROR #101: no descriptor error reported\n");
        test_control_return(1);
    }

    /* Restore descriptor size */
    replaced_cfg_descriptor[9+8+9+5] = device_framework_full_speed[18 + 9+8+9+5];
    /* Set descriptor sub class to DLC */
    replaced_cfg_descriptor[9+8 + 6] = UX_HOST_CLASS_CDC_DLC_SUBCLASS;
    ux_test_hcd_sim_host_set_actions(replaced_GetCfgDescr);
    status = _ux_host_class_cdc_acm_capabilities_get(cdc_acm_host_control);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #102: no error expected\n");
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> Deactivate while writing\n");
    test_usbx_simulator_cdc_acm_host_send_string("ATK",3); /* Disconnect after 10 tick */
    ux_test_hcd_sim_host_set_actions(wait_disconn_on_transfer_0);
    status = ux_host_class_cdc_acm_write(cdc_acm_host_data, cdc_acm_xmit_buffer, 16384, &actual_length);

    stepinfo(">>>>>>>>>>>> All Done\n");

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


UINT test_usbx_simulator_cdc_acm_host_send_command(UCHAR *string, ULONG length, ULONG no_ack)
{

UINT        status;
ULONG       actual_length;

    /* Perform a write to the modem to echo values. And wait for the answer. */
    ux_utility_memory_copy(cdc_acm_xmit_buffer, string,length);
    cdc_acm_xmit_buffer[length] = 0x0d;
    cdc_acm_xmit_buffer[length+1] = 0x0a;

    /* Update the length. */
    length += 2;

    /* Send the AT command.  */
    status = ux_host_class_cdc_acm_write(cdc_acm_host_data, cdc_acm_xmit_buffer, length, &actual_length);

    /* The device may be extracted after we start sending\receiving.  */
    if (status != UX_SUCCESS)
        return(status);

    /* Wait for the answer. */
    if (!no_ack)
        while(command_received_count == 0)
            tx_thread_sleep(10);

    /* Reset receive count. */
    command_received_count = 0;

    /* Return status.  */
    return(status);
}

void    test_thread_host_reception_callback(UX_HOST_CLASS_CDC_ACM *cdc_acm, UINT status, UCHAR *reception_buffer, ULONG reception_size)
{

    /* Incase target to test overflow case, buffers are not moved. */
    if (!cdc_acm_reception_overflow)
    {
        /* And move to the next reception buffer.  Check if we are at the end of the application buffer.  */
        if (cdc_acm_reception.ux_host_class_cdc_acm_reception_data_tail + cdc_acm_reception.ux_host_class_cdc_acm_reception_block_size >=
            cdc_acm_reception.ux_host_class_cdc_acm_reception_data_buffer + cdc_acm_reception.ux_host_class_cdc_acm_reception_data_buffer_size)

            /* We are at the end of the buffer. Move back to the beginning.  */
            cdc_acm_reception.ux_host_class_cdc_acm_reception_data_tail =  cdc_acm_reception.ux_host_class_cdc_acm_reception_data_buffer;

        else

            /* Program the tail to be after the current buffer.  */
            cdc_acm_reception.ux_host_class_cdc_acm_reception_data_tail +=  cdc_acm_reception.ux_host_class_cdc_acm_reception_block_size;
    }
    if (status == UX_BUFFER_OVERFLOW)
        cdc_acm_reception_overflow = UX_FALSE;

    /* Keep the buffer pointer and length received.  */
    global_reception_buffer = reception_buffer;
    global_reception_size = reception_size;

    /* We have received a response.  */
    command_received_count++;

    return;
}

static VOID ux_test_host_class_cdc_acm_device_status_change_callback(
    struct UX_HOST_CLASS_CDC_ACM_STRUCT *cdc_acm,
    ULONG  notification_type,
    ULONG notification_value)
{

    /* We received a notification */
    notification_count ++;
}


void  tx_test_thread_slave_simulation_entry(ULONG arg)
{

UINT                                                status;
ULONG                                               requested_length;
ULONG                                               actual_length;
UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_PARAMETER        line_coding;
UX_SLAVE_CLASS_CDC_ACM_LINE_STATE_PARAMETER         line_state;
UX_SLAVE_CLASS_COMMAND                              class_command;

UCHAR                                               data_bit[16];

ULONG read_size = 64;
ULONG write_size;

    /* The stack/class code always invoke ux_device_class_cdc_acm_entry correct.
       Do a command request error test here. */
    class_command.ux_slave_class_command_request = 0xFF;
    status = ux_device_class_cdc_acm_entry(&class_command);
    /* Error should be reported */
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #30\n");
        test_control_return(1);
    }

    /* On CDC ACM driver initialize, there is memory allocation for:
       - instance of the device cdc_acm class
       - mute for each of endpoint (EP IN and EP OUT)
         mute creation never fails so there is no tests.
     */
    /* Use out memories */
    ux_test_utility_sim_mem_allocate_until(sizeof(UX_SLAVE_CLASS_CDC_ACM));

    /* Try initialize CDC ACM instance */
    class_command.ux_slave_class_command_request = UX_SLAVE_CLASS_COMMAND_INITIALIZE;
    status = ux_device_class_cdc_acm_entry(&class_command);
    /* Error should be reported */
    if (status == UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #32\n");
        test_control_return(1);
    }

    /* Free memory after test */
    ux_test_utility_sim_mem_free_all();

    while(1)
    {

        /* Ensure the CDC class is mounted.  */
        while(cdc_acm_slave != UX_NULL && cdc_acm_slave_bulk_read_write == UX_TRUE)
        {

            /* Read from the CDC class.  */
            status = ux_device_class_cdc_acm_read(cdc_acm_slave, buffer, read_size, &actual_length);

            if (status != UX_SUCCESS)
            {

                break;
            }

            /* Change read size for different read cases */
            if (read_size <= 64)
                read_size = UX_SLAVE_REQUEST_DATA_MAX_LENGTH;

            /* The actual length becomes the requested length.  */
            requested_length = actual_length;

            /* Check for AT command. */
            if (*buffer == 'A' && *(buffer + 1) == 'T')
            {

                /* This is a AT command.  Decode next byte. */
                switch (*(buffer + 2))
                {
                    case 'K' : /* Break! */

                        tx_thread_sleep(10);
                        ux_test_hcd_sim_host_disconnect();
                        break;

                    case 'O' :

                        /* Start writing. */
                        switch(*(buffer + 3))
                        {
                            case '0': /* ZLP */
                                write_size = 0;
                                break;
                            case '1': /* Short packet of 2 */
                                write_size = 2;
                                break;
                            case '2': /* Full packet of 64 */
                                write_size = 64;
                                break;
                            case '3': /* Full packet of 512 */
                                write_size = 512;
                                break;
                            case '4': /* Full packet of 4096 */
                                write_size = 4096;
                                break;
                            case '5': /* Full packet of 8128 */
                                write_size = 8128;
                                break;
                            case 'P': /* Full packet of 64 */
                                write_size = 64;
                                read_size = 64; /* Next read is 64 */
                                break;
                            default:
                                write_size = UX_DEMO_BUFFER_SIZE * 4;
                                break;
                        }

                        status = ux_device_class_cdc_acm_write(cdc_acm_slave, buffer, write_size, &actual_length);
                        if (status == UX_TRANSFER_BUS_RESET)
                            status = ux_device_class_cdc_acm_write(cdc_acm_slave, buffer, write_size, &actual_length);

                        break;

                    case 'W' :

                        /* Start writing. */
                        status = ux_device_class_cdc_acm_write(cdc_acm_slave, buffer, UX_DEMO_BUFFER_SIZE, &actual_length);
                        if (status == UX_SUCCESS)
                        {

                            /* CDC ACM basic test error.  */
                            printf("ERROR #50\n");
                            test_control_return(1);
                        }

                        break;

                    case 'A' :

                        /* This is to try abort XMIT. Pending or next XMIT aborted. */
                        status = _ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_ABORT_PIPE, (VOID*)UX_SLAVE_CLASS_CDC_ACM_ENDPOINT_XMIT);
                        /* Error should not be reported */
                        if (status != UX_SUCCESS)
                        {

                            /* CDC ACM basic test error.  */
                            printf("ERROR #35\n");
                            test_control_return(1);
                        }

                        /* This is to try abort RCV. */
                        status = _ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_ABORT_PIPE, (VOID*)UX_SLAVE_CLASS_CDC_ACM_ENDPOINT_RCV);
                        /* Error should not be reported */
                        if (status != UX_SUCCESS)
                        {

                            /* CDC ACM basic test error.  */
                            printf("ERROR #39\n");
                            test_control_return(1);
                        }

                        /* This is to try abort unknown. */
                        status = _ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_ABORT_PIPE, (VOID*)0xFF);
                        /* Error should be reported */
                        if (status == UX_SUCCESS)
                        {

                            /* CDC ACM basic test error.  */
                            printf("ERROR #47\n");
                            test_control_return(1);
                        }

                        status = tx_test_thread_slave_simulation_response(_rsp_ok, UX_DEMO_BUFFER_SIZE - 2);
                        if (status != UX_SUCCESS)
                        {
                            /* Try again if it's aborted */
                            status = tx_test_thread_slave_simulation_response(_rsp_ok, UX_DEMO_BUFFER_SIZE - 2);
                        }
                        if (status != UX_SUCCESS)
                        {

                            /* CDC ACM basic test error.  */
                            printf("ERROR #44: response sent error %x\n", status);
                            test_control_return(1);
                        }

                        break;

                    case 'F' :

                        /* This is to try invalid IOCTRL code */
                        status = _ux_device_class_cdc_acm_ioctl(cdc_acm_slave, 0xFF, 0);

                        /* Error should be reported */
                        if (status == UX_SUCCESS)
                        {

                            /* CDC ACM basic test error.  */
                            printf("ERROR #34\n");
                            test_control_return(1);
                        }
                        /* Send response any case */
                        status = tx_test_thread_slave_simulation_response("OK", 2);
                        if (status == UX_TRANSFER_BUS_RESET)
                            status = tx_test_thread_slave_simulation_response("OK", 2);
                        /* Send ZLP */
                        ux_device_class_cdc_acm_write(cdc_acm_slave, buffer, 0, &actual_length);
                        break;

                    case 'L' :

                        /* This is to set line coding. */
                        line_coding.ux_slave_class_cdc_acm_parameter_baudrate = buffer[3] + (buffer[4] << 8) + (buffer[5] << 16) + (buffer[6] << 24);
                        line_coding.ux_slave_class_cdc_acm_parameter_stop_bit = *(buffer + 7);
                        line_coding.ux_slave_class_cdc_acm_parameter_parity = *(buffer + 8);
                        line_coding.ux_slave_class_cdc_acm_parameter_data_bit = *(buffer + 9);
                        status = _ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING, &line_coding);

                        if (status != UX_SUCCESS)
                        {

                            /* CDC ACM basic test error.  */
                            printf("ERROR #38\n");
                            test_control_return(1);
                        }
                        /* Send response any case */
                        status = tx_test_thread_slave_simulation_response("OK", 2);
                        break;

                    case 'B' :

                        /* This is to retrieve BAUD rate.  */
                        status = _ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING, &line_coding);

                        /* Any error ? */
                        if (status == UX_SUCCESS)
                        {
                            /* Decode BAUD rate. */
                            switch (line_coding.ux_slave_class_cdc_acm_parameter_baudrate)
                            {

                                case 300 :
                                    status = tx_test_thread_slave_simulation_response("300", 3);
                                    break;

                                case 1200 :
                                    status = tx_test_thread_slave_simulation_response("1200", 4);
                                    break;

                                case 2400 :
                                    status = tx_test_thread_slave_simulation_response("2400", 4);
                                    break;

                                case 4800 :
                                    status = tx_test_thread_slave_simulation_response("4800", 4);
                                    break;

                                case 9600 :
                                    status = tx_test_thread_slave_simulation_response("9600", 4);
                                    break;

                                case 14400 :
                                    status = tx_test_thread_slave_simulation_response("14400", 5);
                                    break;

                                case 19200 :
                                    status = tx_test_thread_slave_simulation_response("19200", 5);
                                    break;

                                case 28800 :
                                    status = tx_test_thread_slave_simulation_response("28800", 5);
                                    break;

                                case 38400 :
                                    status = tx_test_thread_slave_simulation_response("38400", 5);
                                    break;

                                case 57600 :
                                    status = tx_test_thread_slave_simulation_response("57600", 5);
                                    break;

                                case 115200 :
                                    status = tx_test_thread_slave_simulation_response("115200", 6);
                                    break;

                                case 230400 :
                                    status = tx_test_thread_slave_simulation_response("230400", 6);
                                    break;
                            }

                        }
                        break;


                    case 'S' :

                        /* This is to retrieve stop bit rate.  */
                        status = _ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING, &line_coding);

                        /* Any error ? */
                        if (status == UX_SUCCESS)
                        {

                            /* Decode stop bit. */
                            switch (line_coding.ux_slave_class_cdc_acm_parameter_stop_bit)
                            {

                                case 0 :
                                    status = tx_test_thread_slave_simulation_response("0 Stop bit", 10);
                                    break;

                                case 1 :
                                    status = tx_test_thread_slave_simulation_response("1.5 Stop bit", 12);
                                    break;

                                case 2 :
                                    status = tx_test_thread_slave_simulation_response("2 Stop bit", 10);
                                    break;
                            }
                        }
                        break;


                    case 'P' :

                        /* This is to retrieve parity rate.  */
                        status = _ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING, &line_coding);

                        /* Any error ? */
                        if (status == UX_SUCCESS)
                        {

                            /* Decode Parity bit. */
                            switch (line_coding.ux_slave_class_cdc_acm_parameter_parity)
                            {

                                case 0 :
                                    status = tx_test_thread_slave_simulation_response("Parity none", 11);
                                    break;

                                case 1 :
                                    status = tx_test_thread_slave_simulation_response("Parity odd", 10);
                                    break;

                                case 2 :
                                    status = tx_test_thread_slave_simulation_response("Parity even", 11);
                                    break;

                                case 3 :
                                    status = tx_test_thread_slave_simulation_response("Parity mark", 11);
                                    break;

                                case 4 :
                                    status = tx_test_thread_slave_simulation_response("Parity space", 12);
                                    break;


                            }
                        }
                        break;

                    case 'D' :

                        /* This is to retrieve Data Bit.  */
                        status = _ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING, &line_coding);

                        /* Any error ? */
                        if (status == UX_SUCCESS)
                        {
                            /* Copy generic string.  */
                            ux_utility_memory_copy(data_bit, "Data Bit x",10);

                            /* Put data bit value.  */
                            data_bit[9] = line_coding.ux_slave_class_cdc_acm_parameter_data_bit + '0';

                            /* Send data.  */
                            status = tx_test_thread_slave_simulation_response(data_bit, 10);
                        }
                        break;

                    case 'R' :

                        /* This is to retrieve RTS state.  */
                        status = _ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE, &line_state);

                        /* Any error ? */
                        if (status == UX_SUCCESS)
                        {
                            /* Check state.  */
                            if (line_state.ux_slave_class_cdc_acm_parameter_rts == UX_TRUE)

                                /* State is ON.  */
                                status = tx_test_thread_slave_simulation_response("RTS ON", 6);

                            else

                                /* State is OFF.  */
                                status = tx_test_thread_slave_simulation_response("RTS OFF", 7);

                        }
                        break;

                    case 'T' :

                        /* This is to retrieve DTR state.  */
                        status = _ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE, &line_state);

                        /* Any error ? */
                        if (status == UX_SUCCESS)
                        {
                            /* Check state.  */
                            if (line_state.ux_slave_class_cdc_acm_parameter_dtr == UX_TRUE)

                                /* State is ON.  */
                                status = tx_test_thread_slave_simulation_response("DTR ON", 6);

                            else

                                /* State is OFF.  */
                                status = tx_test_thread_slave_simulation_response("DTR OFF", 7);

                        }
                        break;
                }
            }
            else
            {

                /* Not an AT command, just echo back.  */
                /* Check the status.  If OK, we will write to the CDC instance.  */
                status = ux_device_class_cdc_acm_write(cdc_acm_slave, buffer, requested_length, &actual_length);

                /* Check for CR/LF.  */
                if (buffer[requested_length - 1] == '\r')
                {

                    /* Copy LF value into user buffer.  */
                    ux_utility_memory_copy(buffer, "\n",  1);

                    /* And send it again.  */
                    status = ux_device_class_cdc_acm_write(cdc_acm_slave, buffer, 1, &actual_length);

                }
            }
        }

        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
    }
}

UINT  tx_test_thread_slave_simulation_response(UCHAR *string, ULONG length)
{

UINT        status;
ULONG       actual_length;

    /* Perform a write to the modem to echo values. And wait for the answer. */
    ux_utility_memory_copy(buffer, string,length);
    buffer[length] = 0x0d;
    buffer[length+1] = 0x0a;

    /* Update the length. */
    length += 2;

    /* Check the status.  If OK, we will write to the CDC instance.  */
    status = ux_device_class_cdc_acm_write(cdc_acm_slave, buffer, length, &actual_length);

    /* Return status. */
    return(status);
}

static UINT ux_test_host_class_cdc_acm_command(UX_HOST_CLASS_CDC_ACM *cdc_acm, ULONG command,
                                               ULONG value, UCHAR *data_buffer, ULONG data_length,
                                               ULONG *actual_length)
{

UX_ENDPOINT     *control_endpoint;
UX_TRANSFER     *transfer_request;
UINT            status;
ULONG           request_direction;

    /* We need to get the default control endpoint transfer request pointer.  */
    control_endpoint =  &cdc_acm -> ux_host_class_cdc_acm_device -> ux_device_control_endpoint;
    transfer_request =  &control_endpoint -> ux_endpoint_transfer_request;
    switch(command)
    {
        case UX_HOST_CLASS_CDC_ACM_REQ_GET_ENCAPSULATED_COMMAND:
        case UX_HOST_CLASS_CDC_ACM_REQ_GET_COMM_FEATURE:
        case UX_HOST_CLASS_CDC_ACM_REQ_GET_LINE_CODING:
        case UX_HOST_CLASS_CDC_ACM_REQ_GET_RINGER_PARMS:
        case UX_HOST_CLASS_CDC_ACM_REQ_GET_OPERATION_PARMS:
        case UX_HOST_CLASS_CDC_ACM_REQ_GET_LINE_PARMS:

            request_direction = UX_REQUEST_IN;
            break;

        default:

            request_direction = UX_REQUEST_OUT;
    }

    /* Protect the control endpoint semaphore here.  It will be unprotected in the
       transfer request function.  */
    status =  _ux_utility_semaphore_get(&cdc_acm -> ux_host_class_cdc_acm_device -> ux_device_protection_semaphore, UX_WAIT_FOREVER);

    /* Check for status.  */
    if (status != UX_SUCCESS)

        /* Something went wrong. */
        return(status);

    /* Create a transfer_request for the request.  */
    transfer_request -> ux_transfer_request_data_pointer     =  data_buffer;
    transfer_request -> ux_transfer_request_requested_length =  data_length;
    transfer_request -> ux_transfer_request_function         =  command;
    transfer_request -> ux_transfer_request_type             =  request_direction | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value            =  value;
    transfer_request -> ux_transfer_request_index            =  cdc_acm -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Send request to HCD layer.  */
    status =  ux_host_stack_transfer_request(transfer_request);

    /* Fill actual length */
    if (actual_length) {
        *actual_length = transfer_request -> ux_transfer_request_actual_length;
    }

    /* Return completion status.  */
    return(status);
}

