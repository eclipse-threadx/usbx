#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_device_stack.h"
#include "ux_host_class_hub.h"
#include "ux_host_class_dpump.h"
#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"
#include "ux_test.h"
#include "ux_test_actions.h"
#include "ux_device_class_dummy.h"
#include "ux_dcd_sim_slave.h"
#include "ux_device_class_dummy_hub.h"

/* Define local constants.  */

#define UX_TEST_HUB_PORT_STATUS_FULL_SPEED  0x0000

#define UX_DEMO_STACK_SIZE                  (4*1024)
#define UX_USBX_MEMORY_SIZE                 (128*1024)

/* Define structs. */

typedef struct DEVICE_INIT_DATA_STRUCT
{
    UCHAR *framework;
    ULONG framework_length;
    UCHAR *hub_descriptor;
    ULONG hub_descriptor_length;
    UCHAR dont_enumerate;
} DEVICE_INIT_DATA;

/* Host */

static TX_THREAD                            g_thread_host;
static UCHAR                                g_thread_stack_host[UX_DEMO_STACK_SIZE];
static UX_HOST_CLASS                        *g_class_driver_host;
static UX_HOST_CLASS_HUB                    *g_hub_host;
static UX_HOST_CLASS_HUB                    *g_hub_host_from_system_change_function;
static UX_HOST_CLASS_DPUMP                  *g_dpump_host;
static UX_HOST_CLASS_DPUMP                  *g_dpump_host_from_system_change_function;

static TX_THREAD                            g_thread_cmd;
static UCHAR                                g_thread_cmd_stack[UX_DEMO_STACK_SIZE];
static ULONG                                g_thread_cmd_cmd = 0;
#define CMD_SET_AND_SEND_PORT_EVENT_1       0 /* 1 byte  port 1 change.  */
#define CMD_SET_AND_SEND_PORT_EVENT_2       1 /* 2 bytes port 1 change.  */
#define CMD_SET_AND_SEND_PORT_EVENT_3       2 /* 2 bytes device change | port 1 change.  */

#define UX_TEST_HOST_CHANGE_LOG_N           16
static struct {
    ULONG           event;
    UX_HOST_CLASS   *class;
    VOID            *instance;
}                                           g_host_change_logs[UX_TEST_HOST_CHANGE_LOG_N];
static ULONG                                g_host_change_count;

/* Device */

static TX_THREAD                            g_thread_device;
static UCHAR                                g_thread_stack_device[UX_DEMO_STACK_SIZE];
static UX_DEVICE_CLASS_HUB                  *g_hub_device;
static UX_DEVICE_CLASS_HUB_PARAMS           g_hub_device_parameter;

static UCHAR                                g_device_can_go;

/* Define local prototypes and definitions.  */

static void thread_entry_host(ULONG arg);
static void thread_entry_device(ULONG arg);
static void thread_entry_cmd(ULONG arg);
static void post_init_host();
static void post_init_device();

/* Define device framework.  */

static unsigned char default_device_framework[] = {

    /* Device Descriptor */
    0x12, /* bLength */
    0x01, /* bDescriptorType */
    0x00, 0x02, /* bcdUSB */
    0x09, /* bDeviceClass - Hub */
    0x00, /* bDeviceSubClass */
    0x01, /* bDeviceProtocol */
    0x40, /* bMaxPacketSize0 */
    0x24, 0x04, /* idVendor */
    0x12, 0x24, /* idProduct */
    0xb2, 0x0b, /* bcdDevice */
    0x00, /* iManufacturer */
    0x00, /* iProduct */
    0x00, /* iSerialNumber */
    0x01, /* bNumConfigurations */

    /* Configuration Descriptor */
    0x09, /* bLength */
    0x02, /* bDescriptorType */
    0x19, 0x00, /* wTotalLength */
    0x01, /* bNumInterfaces */
    0x01, /* bConfigurationValue */
    0x00, /* iConfiguration */
    0xe0, /* bmAttributes - Self-powered */
    0x01, /* bMaxPower */

    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x00, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x01, /* bNumEndpoints */
    0x09, /* bInterfaceClass - Hub */
    0x00, /* bInterfaceSubClass */
    0x00, /* bInterfaceProtocol */
    0x00, /* iInterface */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x81, /* bEndpointAddress */
    0x03, /* bmAttributes - Interrupt */
    0x02, 0x00, /* wMaxPacketSize */
    0x0c, /* bInterval */

};

static unsigned char default_hub_descriptor[] = {

    /* Hub Descriptor */
    0x09, /* bLength */
    0x29, /* bDescriptorType */
    0x02, /* bNbrPorts */
    0x09, 0x00, /* wHubCharacteristics */
    0x32, /* bPwrOn2PwrGood */
    0x01, /* bHubContrCurrent */
    0x00, /* DeviceRemovable */
    0xff, /* PortPwrCtrlMask */

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

static UCHAR dpump_framework[] = {

    /* Device descriptor */
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Device qualifier descriptor */
        0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x01, 0x00,

    /* Configuration descriptor */
        0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x02, 0x99, 0x99, 0x99,
        0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x01, 0x02, 0x00, 0x02, 0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x82, 0x02, 0x00, 0x02, 0x00
    };

/* This function is invoked by the simulator when the control transfer is meant
   for a device on the hub. */
static UINT hub_control_request_handler(UX_SLAVE_TRANSFER *transfer_request)
{

UX_SLAVE_DCD                *dcd;
UX_SLAVE_DEVICE             *device;
ULONG                       request_type;
ULONG                       request;
ULONG                       request_value;
ULONG                       request_index;
ULONG                       request_length;
UINT                        status =  UX_ERROR;
UCHAR                       *original_framework;
ULONG                       original_framework_length;

    /* Get the pointer to the DCD.  */
    dcd =  &_ux_system_slave -> ux_system_slave_dcd;

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* Ensure that the Setup request has been received correctly.  */
    if (transfer_request -> ux_slave_transfer_request_completion_code == UX_SUCCESS)
    {

        /* Seems so far, the Setup request is valid. Extract all fields of
           the request.  */
        request_type   =   *transfer_request -> ux_slave_transfer_request_setup;
        request        =   *(transfer_request -> ux_slave_transfer_request_setup + UX_SETUP_REQUEST);
        request_value  =   _ux_utility_short_get(transfer_request -> ux_slave_transfer_request_setup + UX_SETUP_VALUE);
        request_index  =   _ux_utility_short_get(transfer_request -> ux_slave_transfer_request_setup + UX_SETUP_INDEX);
        request_length =   _ux_utility_short_get(transfer_request -> ux_slave_transfer_request_setup + UX_SETUP_LENGTH);

        /* Filter for GET_DESCRIPTOR/SET_DESCRIPTOR commands. If the descriptor to be returned is not a standard descriptor,
           treat the command as a CLASS command.  */
        if ((request == UX_GET_DESCRIPTOR || request == UX_SET_DESCRIPTOR) && (((request_value >> 8) & UX_REQUEST_TYPE) != UX_REQUEST_TYPE_STANDARD))
        {        

            /* This request is to be handled by the class layer.  */
            request_type &=  (UINT)~UX_REQUEST_TYPE;
            request_type |= UX_REQUEST_TYPE_CLASS;
        }                   

        /* Ensure it's not vendor. */
        UX_TEST_ASSERT((request_type & UX_REQUEST_TYPE) != UX_REQUEST_TYPE_VENDOR);

        /* We don't support any class commands right now. */
        UX_TEST_ASSERT((request_type & UX_REQUEST_TYPE) != UX_REQUEST_TYPE_CLASS);

        /* Here we proceed only the standard request we know of at the device level.  */
        switch (request)
        {

            case UX_GET_STATUS:

                UX_TEST_ASSERT(0);
                break;

            case UX_CLEAR_FEATURE:

                UX_TEST_ASSERT(0);
                break;

            case UX_SET_FEATURE:

                UX_TEST_ASSERT(0);
                break;

            case UX_SET_ADDRESS:

                /* Don't do anything. */
                break;

            case UX_GET_DESCRIPTOR:

                /* We need to trick USBX into sending a different framework than
                   the hub's. */

                original_framework = _ux_system_slave->ux_system_slave_device_framework;
                original_framework_length = _ux_system_slave->ux_system_slave_device_framework_length;

                /* USBX sends looks here for the framework, so just change it do
                   our liking. */
                _ux_system_slave->ux_system_slave_device_framework = dpump_framework;
                _ux_system_slave->ux_system_slave_device_framework_length = sizeof(dpump_framework);

                /* Kindly ask USBX to send our descriptor. */
                UX_TEST_CHECK_SUCCESS(_ux_device_stack_descriptor_send(request_value, request_index, request_length));

                _ux_system_slave->ux_system_slave_device_framework = original_framework;
                _ux_system_slave->ux_system_slave_device_framework_length = original_framework_length;

                break;

            case UX_SET_DESCRIPTOR:

                UX_TEST_ASSERT(0);
                break;

            case UX_GET_CONFIGURATION:

                UX_TEST_ASSERT(0);
                break;

            case UX_SET_CONFIGURATION:

                /* Do nothing. */
                break;

            case UX_GET_INTERFACE:

                UX_TEST_ASSERT(0);
                break;

            case UX_SET_INTERFACE:

                /* Do nothing. */
                break;

            case UX_SYNCH_FRAME:

                UX_TEST_ASSERT(0);
                break;

            default:

                UX_TEST_ASSERT(0);
                return(UX_ERROR);
        }
        return(UX_SUCCESS);
    }
    return(UX_ERROR);
}

static UINT  class_dpump_get(void)
{

UX_HOST_CLASS   *class;

    /* Find the main dpump container. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_class_get(_ux_system_host_class_dpump_name, &class));

    /* Get the class. */
    UX_TEST_CHECK_SUCCESS(ux_test_host_stack_class_instance_get(class, 0, (void **) &g_dpump_host));

    /* We still need to wait for the dpump status to be live. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uint(&g_dpump_host -> ux_host_class_dpump_state, UX_HOST_CLASS_INSTANCE_LIVE));

    /* In virtually all cases, we want the enumeration thread to be finished. */
    ux_test_wait_for_enum_thread_completion();
        
    /* Return success.  */
    return(UX_SUCCESS);
}

/* Leave all the fields null. */
static DEVICE_INIT_DATA default_device_init_data;

/* Define what the initial system looks like.  */

static void initialize_hub_with_device_init_data(void *first_unused_memory, DEVICE_INIT_DATA *device_init_data)
{

UCHAR *memory_pointer = (UCHAR *)first_unused_memory;

    stepinfo("\n");

    if (device_init_data->framework == UX_NULL)
    {
        device_init_data->framework = default_device_framework;
        device_init_data->framework_length = sizeof(default_device_framework);
    }

    if (device_init_data->hub_descriptor == UX_NULL)
    {
        device_init_data->hub_descriptor = default_hub_descriptor;
        device_init_data->hub_descriptor_length = sizeof(default_hub_descriptor);
    }

    /* Initialize USBX Memory. */
    ux_system_initialize(memory_pointer, UX_USBX_MEMORY_SIZE, UX_NULL, 0);
    memory_pointer += UX_USBX_MEMORY_SIZE;

    /* It looks weird if this doesn't have a comment! */
    ux_utility_error_callback_register(ux_test_error_callback);

    /* Create the host thread. */
    UX_TEST_CHECK_SUCCESS(tx_thread_create(&g_thread_host, "host thread", thread_entry_host, (ULONG)(ALIGN_TYPE)device_init_data,
                                           g_thread_stack_host, UX_DEMO_STACK_SIZE,
                                           30, 30, 1, TX_DONT_START));
    UX_THREAD_EXTENSION_PTR_SET(&g_thread_host, device_init_data)
    tx_thread_resume(&g_thread_host);

    /* Create the slave thread. */
    UX_TEST_CHECK_SUCCESS(tx_thread_create(&g_thread_device, "device thread", thread_entry_device, (ULONG)(ALIGN_TYPE)device_init_data,
                                           g_thread_stack_device, UX_DEMO_STACK_SIZE,
                                           30, 30, 1, TX_DONT_START));
    UX_THREAD_EXTENSION_PTR_SET(&g_thread_device, device_init_data)
    tx_thread_resume(&g_thread_device);

    /* Create the command thread.  */
    UX_TEST_CHECK_SUCCESS(tx_thread_create(&g_thread_cmd, "cmd thread", thread_entry_cmd, 0,
                                           g_thread_cmd_stack, UX_DEMO_STACK_SIZE,
                                           30, 30, 1, TX_DONT_START));
}

static void initialize_hub(void *first_unused_memory)
{

    initialize_hub_with_device_init_data(first_unused_memory, &default_device_init_data);
}

static UINT  class_hub_get(void)
{

UX_HOST_CLASS   *class;


    /* Find the main dpump container. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_class_get(_ux_system_host_class_hub_name, &class));

    /* Get the class. */
    UX_TEST_CHECK_SUCCESS(ux_test_host_stack_class_instance_get(class, 0, (void **) &g_hub_host));

    /* We still need to wait for the dpump status to be live. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uint(&g_hub_host -> ux_host_class_hub_state, UX_HOST_CLASS_INSTANCE_LIVE));
        
    /* Return success.  */
    return(UX_SUCCESS);
}

static VOID device_hub_instance_activate(VOID *instance)
{
    g_hub_device = (UX_DEVICE_CLASS_HUB *)instance;
}

static VOID device_hub_instance_deactivate(VOID *instance)
{
    g_hub_device = (UX_DEVICE_CLASS_HUB *)UX_NULL;
}

static UINT system_change_function(ULONG event, UX_HOST_CLASS *class, VOID *instance)
{
    if (g_host_change_count < UX_TEST_HOST_CHANGE_LOG_N)
    {
        g_host_change_logs[g_host_change_count].event = event;
        g_host_change_logs[g_host_change_count].class = class;
        g_host_change_logs[g_host_change_count].instance = instance;
        g_host_change_count ++;
    }

    if (class == UX_NULL)
        return(UX_SUCCESS);

    if (!memcmp(class->ux_host_class_name, _ux_system_host_class_hub_name, strlen(_ux_system_host_class_hub_name)))
    {
        if (event == UX_DEVICE_INSERTION)
        {
            g_hub_host_from_system_change_function = instance;
        }
        else if (event == UX_DEVICE_REMOVAL)
        {
            g_hub_host_from_system_change_function = UX_NULL;
        }
    }
    else if (!memcmp(class->ux_host_class_name, _ux_system_host_class_dpump_name, strlen(_ux_system_host_class_dpump_name)))
    {
        if (event == UX_DEVICE_INSERTION)
        {
            g_dpump_host_from_system_change_function = instance;
        }
        else if (event == UX_DEVICE_REMOVAL)
        {
            g_dpump_host_from_system_change_function = UX_NULL;
        }
    }
    return(UX_SUCCESS);
}
static void thread_entry_cmd(ULONG arg)
{
ULONG       temp;
    while(1)
    {
        if (g_hub_device)
        {
            switch(g_thread_cmd_cmd)
            {
            case CMD_SET_AND_SEND_PORT_EVENT_1:
                _ux_device_class_hub_notify_change(g_hub_device, 1, 1);
                break;
            case CMD_SET_AND_SEND_PORT_EVENT_2:
                _ux_device_class_hub_notify_change(g_hub_device, 1, 2);
                break;
            case CMD_SET_AND_SEND_PORT_EVENT_3:
                temp = 0x3u;
                _ux_device_class_hub_notify_changes(g_hub_device, (UCHAR*)&temp, 2);
                break;
            default:
                break;
            }
        }
        tx_thread_suspend(&g_thread_cmd);
    }
}
static VOID set_and_send_port_event(UINT port_status, UINT port_change)
{

    /* Setup the approriate fields in the hub instance. */
    g_hub_device->port_status = port_status;
    g_hub_device->port_change = port_change;

    g_thread_cmd_cmd = CMD_SET_AND_SEND_PORT_EVENT_1;
    tx_thread_resume(&g_thread_cmd);
}

static VOID connect_device_to_hub_speed(UINT speed)
{

    set_and_send_port_event(speed | UX_HOST_CLASS_HUB_PORT_STATUS_CONNECTION | UX_HOST_CLASS_HUB_PORT_STATUS_POWER, 
                            UX_HOST_CLASS_HUB_PORT_CHANGE_CONNECTION);
}

static VOID connect_device_to_hub()
{
    connect_device_to_hub_speed(UX_HOST_CLASS_HUB_PORT_STATUS_HIGH_SPEED);
}

static VOID connect_device_to_hub_short_with_hub()
{

    /* Setup the approriate fields in the hub instance. */
    g_hub_device->port_status = UX_HOST_CLASS_HUB_PORT_STATUS_CONNECTION | UX_HOST_CLASS_HUB_PORT_STATUS_POWER;
    g_hub_device->port_change = UX_HOST_CLASS_HUB_PORT_CHANGE_CONNECTION;

    g_thread_cmd_cmd = CMD_SET_AND_SEND_PORT_EVENT_3;
    tx_thread_resume(&g_thread_cmd);
}

static VOID disconnect_device_from_hub()
{

    /* Setup the approriate fields in the hub instance. */
    g_hub_device->port_status = 0; /* Make sure the connection bit is off. */
    g_hub_device->port_change = UX_HOST_CLASS_HUB_PORT_CHANGE_CONNECTION;

    g_thread_cmd_cmd = CMD_SET_AND_SEND_PORT_EVENT_1;
    tx_thread_resume(&g_thread_cmd);
}

static void thread_entry_host(ULONG device_init_data_ptr)
{

UX_DCD_SIM_SLAVE    *dcd_sim_slave;
DEVICE_INIT_DATA    *device_init_data;

    UX_THREAD_EXTENSION_PTR_GET(device_init_data, DEVICE_INIT_DATA, device_init_data_ptr)

    /* The code below is required for installing the device portion of USBX.
       In this demo, DFU is possible and we have a call back for state change. */
    UX_TEST_CHECK_SUCCESS(ux_device_stack_initialize(device_init_data->framework, device_init_data->framework_length,
                                                      device_init_data->framework, device_init_data->framework_length,
                                                      string_framework, sizeof(string_framework),
                                                      language_id_framework, sizeof(language_id_framework),
                                                      UX_NULL));

    g_hub_device_parameter.instance_activate = device_hub_instance_activate;
    g_hub_device_parameter.instance_deactivate = device_hub_instance_deactivate;
    g_hub_device_parameter.descriptor = device_init_data->hub_descriptor;
    g_hub_device_parameter.descriptor_length = device_init_data->hub_descriptor_length;

    /* Initialize the device hub class. */
    UX_TEST_CHECK_SUCCESS(ux_device_stack_class_register(_ux_device_class_hub_name, _ux_device_class_hub_entry, 1, 0, &g_hub_device_parameter));

    /* The code below is required for installing the host portion of USBX. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_initialize(system_change_function));

    /* Register hub class.  */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_class_register(_ux_system_host_class_hub_name, ux_host_class_hub_entry));

#if UX_MAX_CLASS_DRIVER > 1
    /* Register dpump class.  */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_class_register(_ux_system_host_class_dpump_name, ux_host_class_dpump_entry));
#endif

    /* Initialize the simulated device controller.  */
    UX_TEST_CHECK_SUCCESS(_ux_test_dcd_sim_slave_initialize());

    /* Get the DCD sim slave. */
    dcd_sim_slave = (UX_DCD_SIM_SLAVE *)_ux_system_slave->ux_system_slave_dcd.ux_slave_dcd_controller_hardware;

    /* Set the hub control request handler. */
    dcd_sim_slave->ux_dcd_sim_slave_dcd_control_request_process_hub = hub_control_request_handler;

    if (!device_init_data->dont_enumerate)
    {

        /* Register all the USB host controllers available in this system. */
        UX_TEST_CHECK_SUCCESS(ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize, 0, 0));

        /* Sim HCD init will put() the enum thread semaphore. */
        ux_test_wait_for_enum_thread_completion();

        /* Get the hub instance. */
        class_hub_get();
    }

    /* Inform device. */
    g_device_can_go = 1;

    /* Call application. */
    post_init_host();

    /* Disconnect. */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();

    UX_TEST_ASSERT(ux_test_check_actions_empty() == UX_TRUE);

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}

static void thread_entry_device(ULONG input)
{

    /* Wait for host to give us the go ahead. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&g_device_can_go, 1));

    /* Call the application. */
    post_init_device();
}
