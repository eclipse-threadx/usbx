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
#include "ux_host_class_hid.h"
#include "ux_host_class_storage.h"
#include "ux_host_stack.h"

#include "ux_device_class_cdc_acm.h"
#include "ux_device_class_hid.h"
#include "ux_device_class_storage.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

UCHAR test_ux_system_host_class_dummy0_name[] = "ux_host_class_dummy0";
UCHAR test_ux_system_host_class_dummy1_name[] = "ux_host_class_dummy1";
UCHAR test_ux_system_host_class_dummy2_name[] = "ux_host_class_dummy2";
UCHAR test_ux_system_host_class_dummy3_name[] = "ux_host_class_dummy3";
UCHAR test_ux_system_host_class_dummy4_name[] = "ux_host_class_dummy4";
UCHAR test_ux_system_host_class_dummy5_name[] = "ux_host_class_dummy5";
UCHAR test_ux_system_host_class_dummy6_name[] = "ux_host_class_dummy6";
UCHAR test_ux_system_host_class_dummy7_name[] = "ux_host_class_dummy7";
UCHAR test_ux_system_host_class_dummy8_name[] = "ux_host_class_dummy8";
static UINT test_ux_host_class_dummy0_entry(UX_HOST_CLASS_COMMAND *command){return UX_NO_CLASS_MATCH;}
static UINT test_ux_host_class_dummy1_entry(UX_HOST_CLASS_COMMAND *command){return UX_NO_CLASS_MATCH;}
static UINT test_ux_host_class_dummy2_entry(UX_HOST_CLASS_COMMAND *command){return UX_NO_CLASS_MATCH;}
static UINT test_ux_host_class_dummy3_entry(UX_HOST_CLASS_COMMAND *command){return UX_NO_CLASS_MATCH;}
static UINT test_ux_host_class_dummy4_entry(UX_HOST_CLASS_COMMAND *command){return UX_NO_CLASS_MATCH;}
static UINT test_ux_host_class_dummy5_entry(UX_HOST_CLASS_COMMAND *command){return UX_NO_CLASS_MATCH;}
static UINT test_ux_host_class_dummy6_entry(UX_HOST_CLASS_COMMAND *command){return UX_NO_CLASS_MATCH;}
static UINT test_ux_host_class_dummy7_entry(UX_HOST_CLASS_COMMAND *command){return UX_NO_CLASS_MATCH;}
static UINT test_ux_host_class_dummy8_entry(UX_HOST_CLASS_COMMAND *command){return UX_NO_CLASS_MATCH;}

typedef struct TEST_HOST_CLASS_2_REG_STRUCT {

    UCHAR *class_name;
    UINT (*class_entry_function)(struct UX_HOST_CLASS_COMMAND_STRUCT *);
} TEST_HOST_CLASS_2_REG ;
static TEST_HOST_CLASS_2_REG test_host_classes[] = {
    {test_ux_system_host_class_dummy0_name, test_ux_host_class_dummy0_entry},
    {_ux_system_host_class_cdc_acm_name, ux_host_class_cdc_acm_entry},
    {test_ux_system_host_class_dummy1_name, test_ux_host_class_dummy1_entry},
    {_ux_system_host_class_hid_name, ux_host_class_hid_entry},
    {_ux_system_host_class_storage_name, ux_host_class_storage_entry},
    {test_ux_system_host_class_dummy2_name, test_ux_host_class_dummy2_entry},
    {test_ux_system_host_class_dummy3_name, test_ux_host_class_dummy3_entry},
    {test_ux_system_host_class_dummy4_name, test_ux_host_class_dummy4_entry},
    {test_ux_system_host_class_dummy5_name, test_ux_host_class_dummy5_entry},
    {test_ux_system_host_class_dummy6_name, test_ux_host_class_dummy6_entry},
};

typedef struct TEST_HCD_2_REG_STRUCT {

    UCHAR *hcd_name;
    UINT (*hcd_init_function)(struct UX_HCD_STRUCT *);
    ULONG hcd_param1;
    ULONG hcd_param2;
} TEST_HCD_2_REG;
static TEST_HCD_2_REG test_hcds[] = {
    {_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize, 0, 0},
    {"dummy", _ux_test_hcd_sim_host_initialize, 0, 0},
    {"dummy", _ux_test_hcd_sim_host_initialize, 0, 0},
};

typedef struct TEST_DEVICE_CLASS_2_REG_STRUCT {

    UCHAR *class_name;
    UINT (*class_entry_function)(struct UX_SLAVE_CLASS_COMMAND_STRUCT *);
    ULONG configuration_number;
    ULONG interface_number;
} TEST_DEVICE_CLASS_2_REG ;
static TEST_DEVICE_CLASS_2_REG test_device_classes[] = {
    {_ux_system_slave_class_dpump_name, ux_device_class_cdc_acm_entry, 1, 0},
    {_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry, 1, 2},
    {_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry, 2, 0},
    {_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry, 2, 2},
};

/* Define constants.  */
#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024
#define                             UX_DEMO_BUFFER_SIZE (UX_SLAVE_REQUEST_DATA_MAX_LENGTH + 1)
#define                             UX_DEMO_XMIT_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE 512
#define                             UX_DEMO_FILE_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE 64
#define                             UX_DEMO_MEMORY_SIZE     (128 * 1024)
#define                             UX_DEMO_FILE_SIZE       (128 * 1024)
#define                             UX_RAM_DISK_MEMORY      (256 * 1024)

/* Define local/extern function prototypes.  */
static VOID                                test_thread_entry(ULONG);
static TX_THREAD                           tx_test_thread_host_simulation;
static TX_THREAD                           tx_test_thread_slave_simulation;
static VOID                                tx_test_thread_host_simulation_entry(ULONG);
static VOID                                tx_test_thread_slave_simulation_entry(ULONG);
static VOID                                test_cdc_instance_activate(VOID  *cdc_instance);
static VOID                                test_cdc_instance_deactivate(VOID *cdc_instance);
static VOID                                test_cdc_instance_parameter_change(VOID *cdc_instance);

static UINT                                test_ux_device_class_entry(UX_SLAVE_CLASS_COMMAND *command);

/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UX_HOST_CLASS                       *class_driver;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_control;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_data;

static UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave;
static UCHAR                               cdc_acm_slave_change;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER    parameter;

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

static UINT                                device_class_entry_return;
static UINT                                device_class_entry_cmd_req;

/* Define device framework.  */

#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      93
#define             DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED      103
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
    0x09, 0x02, 0x4b, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
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
    0x07, 0x05, 0x81, /* @ 93 - 14 + 2 = 81 */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02, /* @ 93 - 7 + 2 = 88 */
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
    0x09, 0x02, 0x4b, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

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
    0x07, 0x05, 0x81, /* @ 103 - 14 + 2 = 91 */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor */
    0x07, 0x05, 0x02, /* @ 103 - 7 + 2 = 98 */
    0x02,
    0x40, 0x00,
    0x00,

};

#define DEVICE_FRAMEWORK_EPA_POS_1_HS (DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED - 14 + 2)
#define DEVICE_FRAMEWORK_EPA_POS_2_HS (DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED - 7 + 2)

static unsigned char device_framework_no_interface[] = {

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
    0x09, 0x02, 0x09, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,
};

static unsigned char device_framework_no_endpoint[] = {

    0x12, 0x01, 0x10, 0x01,
    0x00, 0x00, 0x00,
    0x08,
    0x84, 0x84, 0x00, 0x00,
    0x00, 0x01,
    0x01, 0x02, 03,
    0x01,

    /* Configuration 1 descriptor 9 bytes */
    0x09, 0x02, 0x12, 0x00,
    0x01, 0x01, 0x00,
    0x40, 0x00,

    /* Interface Descriptor */
    0x09, 0x04, 0x00,
    0x00,
    0x00,
    0xFF, 0x01, 0x00,
    0x00,
};

static unsigned char device_framework_testing[] = {

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

    /* Configuration 3 descriptor 9 bytes */
    0x09, 0x02,
    34, 0x00,        /* wTotalLength */
    1,               /* bNumInterfaces */
    3,               /* bConfigurationValue */
    0x00,0x40,0x00,  /* iConfiguration, bmAttributes, bMaxPower */

    /* Interface 1.0 descriptor 9 bytes */
    0x09, 0x04,
    0x00,            /* bInterfaceNumber */
    0x00,            /* bAlternateSetting */
    0x00,            /* bNumEndpoints */
    0x02, 0x02, 0x01,/* bInterfaceClass,SubClass,Protocol */
    0x00,            /* iInterface */

    /* Interface 1.1 descriptor 9 bytes */
    0x09, 0x04,
    0x00,            /* bInterfaceNumber */
    0x01,            /* bAlternateSetting */
    0x01,            /* bNumEndpoints */
    0x02, 0x02, 0x01,/* bInterfaceClass,SubClass,Protocol */
    0x00,            /* iInterface */

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,/* ...bEndpointAddress */
    0x02,
    0x40, 0x00,
    0x00,

    /* Configuration 1 descriptor 9 bytes */
    0x09, 0x02,
    32, 0x00,        /* wTotalLength */
    1,               /* bNumInterfaces */
    1,               /* bConfigurationValue */
    0x00,0x40, 0x00, /* iConfiguration, bmAttributes, bMaxPower */

    /* Interface 1 descriptor 9 bytes */
    0x09, 0x04,
    0x00,            /* bInterfaceNumber */
    0x00,            /* bAlternateSetting */
    0x02,            /* bNumEndpoints */
    0x02, 0x02, 0x01,/* bInterfaceClass,SubClass,Protocol */
    0x00,            /* iInterface */

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,/* ...bEndpointAddress */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02,/* ...bEndpointAddress */
    0x02,
    0x40, 0x00,
    0x00,

    /* Configuration 2 descriptor 9 bytes */
    0x09, 0x02,
    39, 0x00,        /* wTotalLength */
    1,               /* bNumInterfaces */
    2,               /* bConfigurationValue */
    0x00,0x40, 0x00, /* iConfiguration, bmAttributes, bMaxPower */

    /* Interface 2 descriptor 9 bytes */
    0x09, 0x04,
    0x00,            /* bInterfaceNumber */
    0x00,            /* bAlternateSetting */
    0x03,            /* bNumEndpoints */
    0x02, 0x02, 0x01,/* bInterfaceClass,SubClass,Protocol */
    0x00,            /* iInterface */

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,/* ...bEndpointAddress */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02,/* ...bEndpointAddress */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x83 descriptor 7 bytes */
    0x07, 0x05, 0x83,/* ...bEndpointAddress */
    0x02,
    0x40, 0x00,
    0x00,

};


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

/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}

static UINT test_ux_device_class_entry(UX_SLAVE_CLASS_COMMAND *command)
{
    device_class_entry_cmd_req = command->ux_slave_class_command_request;
    return device_class_entry_return;
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

static VOID    test_cdc_instance_activate(VOID *cdc_instance)
{

    /* Save the CDC instance.  */
    cdc_acm_slave = (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;
}
static VOID    test_cdc_instance_deactivate(VOID *cdc_instance)
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
/* TODO: _ux_system_uninitialize, patch implement to lib if necessary */
#if 0
UINT  _ux_system_uninitialize(VOID)
{

    /* Delete the Mutex object used by USBX to control critical sections. */
    if (_ux_system->ux_system_mutex.tx_mutex_id != TX_CLEAR_ID)
        _ux_utility_mutex_delete(&_ux_system -> ux_system_mutex);

    return(UX_SUCCESS);
}
#endif
/* TODO: _ux_device_stack_uninitialize, patch implement to lib if necessary */
#if 0
UINT  _ux_device_stack_uninitialize(VOID)
{
UX_SLAVE_DEVICE                 *device;
UX_SLAVE_ENDPOINT               *endpoints_pool;
UX_SLAVE_TRANSFER               *transfer_request;
ULONG                           endpoints_found;

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_STACK_INITIALIZE, 0, 0, 0, 0, UX_TRACE_DEVICE_STACK_EVENTS, 0, 0)

    /* Get the pointer to the device. */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* Free class memory. */
    if (_ux_system_slave -> ux_system_slave_class_array)
        _ux_utility_memory_free(_ux_system_slave -> ux_system_slave_class_array);

    /* Allocate some memory for the Control Endpoint.  First get the address of the transfer request for the
       control endpoint. */
    transfer_request =  &device -> ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

    /* Free memory for the control endpoint buffer. */
    if (transfer_request -> ux_slave_transfer_request_data_pointer)
        _ux_utility_memory_free(transfer_request -> ux_slave_transfer_request_data_pointer);

    /* Get the number of endoints found in the device framework.  */
    endpoints_found = device -> ux_slave_device_endpoints_pool_number;

    /* Get the endpoint pool address in the device container.  */
    endpoints_pool =  device -> ux_slave_device_endpoints_pool;

    /* Parse all endpoints and fee memory and semaphore. */
    while (endpoints_found-- != 0)
    {
        /* Free the memory for endpoint data pointer. */
        if (endpoints_pool -> ux_slave_endpoint_transfer_request.ux_slave_transfer_request_data_pointer)
            _ux_utility_memory_free(endpoints_pool -> ux_slave_endpoint_transfer_request.ux_slave_transfer_request_data_pointer);

        /* Remove the TX semaphore for the endpoint.  */
        if (endpoints_pool -> ux_slave_endpoint_transfer_request.ux_slave_transfer_request_semaphore.tx_semaphore_id != TX_CLEAR_ID)
            _ux_utility_semaphore_delete(&endpoints_pool -> ux_slave_endpoint_transfer_request.ux_slave_transfer_request_semaphore);

        /* Next endpoint.  */
        endpoints_pool++;
    }

    /* Free the endpoint pool address in the device container. */
    if (device -> ux_slave_device_endpoints_pool)
        _ux_utility_memory_free(device -> ux_slave_device_endpoints_pool);

    /* Free memory for interface pool.  */
    if (device -> ux_slave_device_interfaces_pool)
        _ux_utility_memory_free(device -> ux_slave_device_interfaces_pool);

    /* Return successful completion.  */
    return(UX_SUCCESS);
}
#endif
#if 0 /* _ux_host_stack_uninitialize, move implement to lib if necessary */
VOID _ux_host_stack_uninitialize(VOID)
{
    if (_ux_system_host->ux_system_host_hcd_thread.tx_thread_id != TX_CLEAR_ID)
    {

        tx_thread_terminate(&_ux_system_host->ux_system_host_hcd_thread);
        tx_thread_delete(&_ux_system_host->ux_system_host_hcd_thread);
    }

    if (_ux_system_host->ux_system_host_enum_thread.tx_thread_id != TX_CLEAR_ID)
    {

        tx_thread_terminate(&_ux_system_host->ux_system_host_enum_thread);
        tx_thread_delete(&_ux_system_host->ux_system_host_enum_thread);
    }

#if defined(UX_OTG_SUPPORT)
    if (_ux_system_host->ux_system_host_hnp_polling_thread.tx_thread_id != TX_CLEAR_ID)
    {
        tx_thread_terminate(&_ux_system_host->ux_system_host_hnp_polling_thread);
        tx_thread_delete(&_ux_system_host->ux_system_host_hnp_polling_thread);
    }
    if (_ux_system_host->ux_system_host_hnp_polling_thread_stack)
        ux_utility_memory_free(_ux_system_host->ux_system_host_hnp_polling_thread_stack);
#endif

    if (_ux_system_host->ux_system_host_hcd_semaphore.tx_semaphore_id != TX_CLEAR_ID)
        ux_utility_semaphore_delete(&_ux_system_host->ux_system_host_hcd_semaphore);

    if (_ux_system_host->ux_system_host_enum_semaphore.tx_semaphore_id != TX_CLEAR_ID)
        ux_utility_semaphore_delete(&_ux_system_host->ux_system_host_enum_semaphore);


    if (_ux_system_host -> ux_system_host_enum_thread_stack)
        ux_utility_memory_free(_ux_system_host -> ux_system_host_enum_thread_stack);

    if (_ux_system_host -> ux_system_host_device_array)
        ux_utility_memory_free(_ux_system_host -> ux_system_host_device_array);

    if (_ux_system_host -> ux_system_host_class_array)
        ux_utility_memory_free(_ux_system_host -> ux_system_host_class_array);

    if (_ux_system_host -> ux_system_host_hcd_array)
        ux_utility_memory_free(_ux_system_host -> ux_system_host_hcd_array);
}
#endif

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_host_device_initialize_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;
ULONG                   test_n;

ULONG                   mem_count;
ULONG                   sem_count;
ULONG                   thread_count;

    printf("Running Host & Device Init/Uninit Test.............................. ");

    /* Enable memory logging for tests. */
    ux_test_utility_sim_mem_alloc_log_enable(UX_TRUE);

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

    stepinfo(">>>>>>>>>>>>>>>> Test ux_system_initialize\n");

    /* Initialize with very small memory should report error */
    status = ux_system_initialize(memory_pointer, 8, UX_NULL, 0);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("ERROR #1: should report memory error when initialize with small buffer\n");
        test_control_return(1);
    }

    /* Initialize with mutex error */
    ux_test_utility_sim_mutex_error_generation_start(0);
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);
    if (status != UX_MUTEX_ERROR)
    {

        printf("ERROR #2: should report mutex error when system mutex not created\n");
        test_control_return(1);
    }
    ux_test_utility_sim_mutex_error_generation_stop();

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE / 2, memory_pointer + UX_DEMO_MEMORY_SIZE/ 2, UX_DEMO_MEMORY_SIZE/ 2);
    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #3\n");
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(test_ux_error_callback);

    stepinfo(">>>>>>>>>>>>>>>> Test ux_host_stack_initialize\n");

    /* Initialize to check resources usage */
    _ux_system_uninitialize();
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL, 0);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #4\n");
        test_control_return(1);
    }

    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_thread_create_count_reset();
    ux_test_utility_sim_mem_alloc_count_reset();

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(test_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #5\n");
        test_control_return(1);
    }
    mem_count = ux_test_utility_sim_mem_alloc_count();
    sem_count = ux_test_utility_sim_sem_create_count();
    thread_count = ux_test_utility_sim_thread_create_count();

#if 1
    /* Test memory allocation errors. */
    for (test_n = 0; test_n < mem_count; test_n ++)
    {

        stepinfo(" .memTest %2ld / %2ld\n", test_n, mem_count - 1);

        /* Re-initialize memory! */
        if (status == UX_SUCCESS)
            _ux_host_stack_uninitialize();
        _ux_system_uninitialize();
        status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL, 0);
        if (status != UX_SUCCESS)
        {

            printf("ERROR #6.%ld\n", test_n);
            test_control_return(1);
        }

        /* Start error simulation */
        ux_test_utility_sim_mem_alloc_error_generation_start(test_n);
        status = ux_host_stack_initialize(test_host_change_function);
        ux_test_utility_sim_mem_alloc_error_generation_stop();
        if (status != UX_MEMORY_INSUFFICIENT)
        {

            printf("ERROR #7.%ld: memory error should be reported\n", test_n);
            test_control_return(1);
        }
    }
#endif
#if 1
    /* Test semaphore creation errors. */
    for (test_n = 0; test_n < sem_count; test_n ++)
    {

        stepinfo(" .semTest %2ld / %2ld\n", test_n, sem_count - 1);

        /* Re-initialize memory! */
        if (status == UX_SUCCESS)
            _ux_host_stack_uninitialize();
        _ux_system_uninitialize();
        status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL, 0);
        if (status != UX_SUCCESS)
        {

            printf("ERROR #8.%ld\n", test_n);
            test_control_return(1);
        }

        /* Start error simulation */
        ux_test_utility_sim_sem_error_generation_start(test_n);
        status = ux_host_stack_initialize(test_host_change_function);
        ux_test_utility_sim_sem_error_generation_stop();
        if (status != UX_SEMAPHORE_ERROR)
        {

            printf("ERROR #9.%ld: semaphore error should be reported\n", test_n);
            test_control_return(1);
        }
    }
#endif
#if 1
    /* Test thread creation errors. */
    for (test_n = 0; test_n < thread_count; test_n ++)
    {

        stepinfo(" .threadTest %2ld / %2ld\n", test_n, thread_count - 1);

        /* Re-initialize memory! */
        if (status == UX_SUCCESS)
            _ux_host_stack_uninitialize();
        _ux_system_uninitialize();
        status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL, 0);
        if (status != UX_SUCCESS)
        {

            printf("ERROR #10.%ld\n", test_n);
            test_control_return(1);
        }

        /* Start error simulation */
        ux_test_utility_sim_thread_error_generation_start(test_n);
        status = ux_host_stack_initialize(test_host_change_function);
        ux_test_utility_sim_thread_error_generation_stop();
        if (status != UX_THREAD_ERROR)
        {

            printf("ERROR #11.%ld: thread error should be reported\n", test_n);
            test_control_return(1);
        }
    }
#endif

    _ux_host_stack_uninitialize();
    _ux_system_uninitialize();
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL, 0);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: ux_system_initialize fail\n", __LINE__);
        test_control_return(1);
    }
    status = ux_host_stack_initialize(test_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: ux_host_stack_initialize fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test ux_host_stack_class_register\n");

    for (test_n = 0; test_n < UX_MAX_CLASS_DRIVER; test_n ++)
    {
        stepinfo(" .classReg %2ld / %2d : %s\n", test_n, UX_MAX_CLASS_DRIVER - 1, test_host_classes[test_n].class_name);

        status = ux_host_stack_class_register(test_host_classes[test_n].class_name, test_host_classes[test_n].class_entry_function);
        if (status != UX_SUCCESS)
        {

            printf("ERROR #%d\n", __LINE__);
            test_control_return(1);
        }
    }

    /* Register again report error */
#if UX_MAX_CLASS_DRIVER == 1
    status =  ux_host_stack_class_register(test_ux_system_host_class_dummy0_name, test_ux_host_class_dummy0_entry);
#else
    status =  ux_host_stack_class_register(_ux_system_host_class_cdc_acm_name, ux_host_class_cdc_acm_entry);
#endif
    if (status != UX_HOST_CLASS_ALREADY_INSTALLED)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register more report error */
    status = ux_host_stack_class_register(test_host_classes[UX_MAX_CLASS_DRIVER].class_name, test_host_classes[UX_MAX_CLASS_DRIVER].class_entry_function);
    if (status != UX_MEMORY_ARRAY_FULL)
    {

        printf("ERROR #%d: register more than %d class should report error\n", __LINE__, UX_MAX_CLASS_DRIVER);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test ux_device_stack_initialize\n");

#if !defined(UX_DEVICE_INITIALIZE_FRAMEWORK_SCAN_DISABLE)
    /* No interface found, error should be reported. */
    status = ux_device_stack_initialize(device_framework_no_interface, sizeof(device_framework_no_interface),
                                        device_framework_no_interface, sizeof(device_framework_no_interface),
                                        string_framework, STRING_FRAMEWORK_LENGTH,
                                        language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
#endif

    /* Try a testing framework. */
    status = ux_device_stack_initialize(device_framework_testing, sizeof(device_framework_testing),
                                        device_framework_testing, sizeof(device_framework_testing),
                                        string_framework, STRING_FRAMEWORK_LENGTH,
                                        language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: %x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Uninitialize the stack to change framework to the one used for enumeration. */
    ux_device_stack_uninitialize();

    /* Test framework with no endpoint. */
    status = ux_device_stack_initialize(device_framework_no_endpoint, sizeof(device_framework_no_endpoint),
                                        device_framework_no_endpoint, sizeof(device_framework_no_endpoint),
                                        string_framework, STRING_FRAMEWORK_LENGTH,
                                        language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Uninitialize the stack to change framework to the one used for enumeration. */
    ux_device_stack_uninitialize();

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    ux_test_utility_sim_mem_alloc_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    mem_count = ux_test_utility_sim_mem_alloc_count();
    sem_count = ux_test_utility_sim_sem_create_count();
    ux_test_utility_sim_mem_alloc_log_lock();
#if 1 /* FIXME: _ux_device_stack_uninitialize must check resources before free them */
    for (test_n = 0; test_n < mem_count; test_n ++)
    {
        stepinfo(" .dStackMEM %2ld / %2ld\n", test_n, mem_count - 1);

        /* Confirm uninitialize (no need since stack_initialize fixed) */
        // ux_device_stack_uninitialize();

        /* Start error generation */
        ux_test_utility_sim_mem_alloc_error_generation_start(test_n);

        /* Check error */
        status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                        device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                        string_framework, STRING_FRAMEWORK_LENGTH,
                                        language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
        /* Stop error generation */
        ux_test_utility_sim_mem_alloc_error_generation_stop();
        if(status != UX_MEMORY_INSUFFICIENT)
        {

            printf("ERROR #%d.%ld: memory error should be reported\n", __LINE__, test_n);
            test_control_return(1);
        }
    }
#endif
#if 1 /* FIXME: _ux_device_stack_uninitialize must check resources before free them */
    for (test_n = 0; test_n < sem_count; test_n ++)
    {
        stepinfo(" .dStackSEM %2ld / %2ld\n", test_n, sem_count - 1);

        /* Confirm uninitialize (no need since stack_initialize fixed) */
        // ux_device_stack_uninitialize();

        /* Start error generation */
        ux_test_utility_sim_sem_error_generation_start(test_n);

        /* Check error */
        status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                        device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                        string_framework, STRING_FRAMEWORK_LENGTH,
                                        language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
        /* Stop error generation */
        ux_test_utility_sim_sem_error_generation_stop();
        if(status != UX_SEMAPHORE_ERROR)
        {

            printf("ERROR #%d.%ld: semaphore error should be reported\n", __LINE__, test_n);
            test_control_return(1);
        }
    }
#endif
    /* Do a good initialize for enumeration test. */
    ux_device_stack_uninitialize();
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test ux_device_stack_class_register\n");

    /* Set the parameters for callback when insertion/extraction of a CDC device.  */
    parameter.ux_slave_class_cdc_acm_instance_activate   =  test_cdc_instance_activate;
    parameter.ux_slave_class_cdc_acm_instance_deactivate =  test_cdc_instance_deactivate;
    parameter.ux_slave_class_cdc_acm_parameter_change    =  test_cdc_instance_parameter_change;

    for (test_n = 0; test_n < UX_MAX_SLAVE_CLASS_DRIVER; test_n ++)
    {
        stepinfo(" .dStackClassReg %2ld / %2d\n", test_n, UX_MAX_SLAVE_CLASS_DRIVER - 1);

        /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
        status =  ux_device_stack_class_register(test_device_classes[test_n].class_name, test_device_classes[test_n].class_entry_function,
                                                 test_device_classes[test_n].configuration_number, test_device_classes[test_n].interface_number,
                                                 &parameter);

        if(status!=UX_SUCCESS)
        {

            printf("ERROR #%d\n", __LINE__);
            test_control_return(1);
        }
    }

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                            1,0,  &parameter);

    if(status == UX_SUCCESS)
    {

        printf("ERROR #%d: register more than %d class should report error\n", __LINE__, UX_MAX_SLAVE_CLASS_DRIVER);
        test_control_return(1);
    }

    for (test_n = 0; test_n < UX_MAX_SLAVE_CLASS_DRIVER; test_n ++)
    {
        stepinfo(" .dStackClassUnReg %2ld / %2d\n", test_n, UX_MAX_SLAVE_CLASS_DRIVER - 1);

        /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
        status =  ux_device_stack_class_unregister(test_device_classes[UX_MAX_SLAVE_CLASS_DRIVER - 1 - test_n].class_name,
                                                   test_device_classes[UX_MAX_SLAVE_CLASS_DRIVER - 1 - test_n].class_entry_function);

        if(status!=UX_SUCCESS)
        {

            printf("ERROR #%d\n", __LINE__);
            test_control_return(1);
        }
    }

    status =  ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);

    if(status == UX_SUCCESS)
    {

        printf("ERROR #%d: unregister none exist class should report error\n", __LINE__);
        test_control_return(1);
    }

    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, test_ux_device_class_entry,
                                             1,0,  &parameter);
    if(status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    device_class_entry_return = UX_ERROR;
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, test_ux_device_class_entry);
    if(status == UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    device_class_entry_return = UX_SUCCESS;
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);
    if(status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                            1,0,  &parameter);

    if(status != UX_SUCCESS)
    {

        printf("ERROR #%d: register more than %d class should report error\n", __LINE__, UX_MAX_SLAVE_CLASS_DRIVER);
        test_control_return(1);
    }

#if 0 /* FIXME: ? WHY Segmentation faul on _ux_utility_mutex_create(&_ux_system -> ux_system_mutex, "ux_mutex") */
    stepinfo(">>>>>>>>>>>>>>>> Uninitialize all\n");
    /* Missing HCD uninitialize (since initialize called on registering) */
    /* No need ux_host_stack_class_unregister, since there is no entry call */
    ux_device_stack_uninitialize();
    _ux_host_stack_uninitialize();
    _ux_system_uninitialize();

    stepinfo(">>>>>>>>>>>>>>>> ux_system_initialize\n");
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL, 0);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    stepinfo(">>>>>>>>>>>>>>>> ux_host_stack_initialize\n");
    status =  ux_host_stack_initialize(test_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    stepinfo(">>>>>>>>>>>>>>>> ux_host_stack_class_register\n");
    status =  ux_host_stack_class_register(_ux_system_host_class_cdc_acm_name, ux_host_class_cdc_acm_entry);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    stepinfo(">>>>>>>>>>>>>>>> ux_host_stack_hcd_register\n");
    status =  ux_host_stack_hcd_register(test_hcds[test_n].hcd_name, test_hcds[test_n].hcd_init_function, test_hcds[test_n].hcd_param1, test_hcds[test_n].hcd_param2);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    stepinfo(">>>>>>>>>>>>>>>> ux_device_stack_initialize\n");
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    stepinfo(">>>>>>>>>>>>>>>> ux_device_stack_class_register\n");
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1,0,  &parameter);

    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
#endif

    stepinfo(">>>>>>>>>>>>>>>> Test _ux_test_dcd_sim_slave_initialize\n");

    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test ux_host_stack_hcd_register\n");

    for (test_n = 0; test_n < UX_MAX_HCD; test_n ++)
    {
        stepinfo(" .hcdReg %2ld / %2d\n", test_n, UX_MAX_HCD - 1);

        status =  ux_host_stack_hcd_register(test_hcds[test_n].hcd_name, test_hcds[test_n].hcd_init_function, test_hcds[test_n].hcd_param1, test_hcds[test_n].hcd_param2);
        if (status != UX_SUCCESS)
        {

            printf("ERROR #%d\n", __LINE__);
            test_control_return(1);
        }
    }

    /* Register more will report error */
    status =  ux_host_stack_hcd_register(test_hcds[UX_MAX_HCD].hcd_name, test_hcds[UX_MAX_HCD].hcd_init_function, test_hcds[UX_MAX_HCD].hcd_param1, test_hcds[UX_MAX_HCD].hcd_param2);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: error should reported if try to add more than %d HCDs\n", __LINE__, UX_MAX_HCD);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_test_thread_host_simulation, "tx test host simulation", tx_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&tx_test_thread_slave_simulation, "tx test slave simulation", tx_test_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}

void  tx_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
ULONG                                               test_n;
ULONG                                               mem_free;

    stepinfo("\n");

#if UX_MAX_CLASS_DRIVER > 1 /* CDC is not first class ... */
    /* Test connect. */
    stepinfo(">>>>>>>>>>>>>>>> Test connect\n");
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    tx_thread_sleep(100);
    if (cdc_acm_host_control == UX_NULL || cdc_acm_host_data == UX_NULL || cdc_acm_slave == UX_NULL)
    {

        printf("ERROR #%d: connection not detected\n", __LINE__);
        test_control_return(1);
    }
    mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    test_n = 10;
    while((cdc_acm_host_control || cdc_acm_host_data || cdc_acm_slave) && test_n --)
        tx_thread_sleep(10);

    if (cdc_acm_host_control || cdc_acm_host_data || cdc_acm_slave)
    {

        printf("ERROR #%d: instance not removed when disconnect, %p %p %p\n", __LINE__, cdc_acm_host_control, cdc_acm_host_data, cdc_acm_slave);
        test_control_return(1);
    }
    if (_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available <= mem_free)
    {

        printf("ERROR #%d: memory not freed when disconnect\n", __LINE__);
        test_control_return(1);
    }
#endif
    /* Deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);

}

void  tx_test_thread_slave_simulation_entry(ULONG arg)
{

    while(1)
    {

        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
    }
}
