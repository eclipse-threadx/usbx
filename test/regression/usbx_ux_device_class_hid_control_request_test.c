/* This file tests _ux_device_class_hid_control_request(). Note that the 4 requests we're testing
(GET_IDLE, SET_IDLE, GET_PROTOCOL, and SET_PROTOCOL) have yet to be implemented on the device;
Therefore, right now, not much is done in the way of checking output. */

#include "usbx_test_common_hid.h"

#include "ux_host_class_hid_keyboard.h"
#include "ux_device_class_hid.h"


#define DUMMY_USBX_MEMORY_SIZE (64*1024)
static UCHAR dummy_usbx_memory[DUMMY_USBX_MEMORY_SIZE];

static UX_SLAVE_CLASS_HID * hid_device;

static UCHAR    event_buffer[8];
static UCHAR    dummy_report[7];
static UCHAR    buffer[64];
static UINT     set_report_test_started;
static UCHAR    error_is_expected = UX_FALSE;

static UCHAR hid_report_descriptor[] = {

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
#define HID_REPORT_LENGTH sizeof(hid_report_descriptor)/sizeof(hid_report_descriptor[0])


#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED 52
static UCHAR device_framework_full_speed[DEVICE_FRAMEWORK_LENGTH_FULL_SPEED] = {

    /* Device descriptor */
        0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
        0x81, 0x0A, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,

    /* Configuration descriptor */
        0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x02, 0x00, 0x01, 0x03, 0x00, 0x00,
        0x00,

    /* HID descriptor */
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_REPORT_LENGTH),
        MSB(HID_REPORT_LENGTH),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };


#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED 62
static UCHAR device_framework_high_speed[DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED] = {

    /* Device descriptor */
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Device qualifier descriptor */
        0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x01, 0x00,

    /* Configuration descriptor */
        0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x02, 0x00, 0x01, 0x03, 0x00, 0x00,
        0x00,

    /* HID descriptor */
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_REPORT_LENGTH),
        MSB(HID_REPORT_LENGTH),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };


    /* String Device Framework :
     Byte 0 and 1 : Word containing the language ID : 0x0904 for US
     Byte 2       : Byte containing the index of the descriptor
     Byte 3       : Byte containing the length of the descriptor string
    */

#define STRING_FRAMEWORK_LENGTH 40
static UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
        0x09, 0x04, 0x02, 0x0c,
        0x55, 0x53, 0x42, 0x20, 0x4b, 0x65, 0x79, 0x62,
        0x6f, 0x61, 0x72, 0x64,

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


UINT  _ux_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter);


static UINT ux_system_host_change_function(ULONG a, UX_HOST_CLASS *b, VOID *c)
{
    return 0;
}

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    /* Failed test.  */
    if (!error_is_expected)
    {
        /* Ignore protocol errors. */
        if (error_code == UX_TRANSFER_STALLED)
            return;

        printf("Error on line %d, system_level: %d, system_context: %d, error code: 0x%x\n", __LINE__, system_level, system_context, error_code);
        test_control_return(1);
    }
}

static VOID demo_hid_instance_activate(VOID *inst)
{
    hid_device = (UX_SLAVE_CLASS_HID *)inst;
}
static VOID demo_hid_instance_deactivate(VOID *inst)
{
    if ((VOID *)hid_device == inst)
        hid_device = UX_NULL;
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_class_hid_control_request_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Inform user.  */
    printf("Running ux_device_hid_control_request Test.......................... ");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(ux_system_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name, ux_host_class_hid_keyboard_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
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

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the hid class parameters.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_report_descriptor;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;
    hid_parameter.ux_slave_class_hid_instance_activate         = demo_hid_instance_activate;
    hid_parameter.ux_slave_class_hid_instance_deactivate       = demo_hid_instance_deactivate;

    /* Initilize the device hid class. The class is connected with interface 2 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,2, (VOID *)&hid_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

#if defined(UX_DEVICE_STANDALONE)

    /* Create the main device simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_device_simulation, "tx demo device simulation", tx_demo_thread_device_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
#endif
}

#if defined(UX_DEVICE_STANDALONE)
static void  tx_demo_thread_device_simulation_entry(ULONG arg)
{
    while(1)
    {
        ux_system_tasks_run();
        tx_thread_relinquish();
    }
}
#endif

static UINT    demo_thread_hid_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{

UINT status;


    if (set_report_test_started == 1)
    {

        status = ux_utility_memory_compare(event -> ux_device_class_hid_event_buffer, dummy_report, sizeof(dummy_report));
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }
    }

    return(UX_SUCCESS);
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                            status;
UX_ENDPOINT                    *control_endpoint;
UX_TRANSFER                    *transfer_request;
UX_HOST_CLASS_HID_KEYBOARD     *keyboard;
UX_SLAVE_CLASS                 *slave_class;
ULONG                           idle_time;
ULONG                           report_id;
UX_SLAVE_CLASS_HID_EVENT        hid_event;
UX_SLAVE_CLASS_HID             *slave_hid;
UINT                            max_get_report_attempts = 5;


    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Get the HID client */
    hid_client = hid -> ux_host_class_hid_client;

    /* Check if the instance of the keyboard is live */
    while (hid_client -> ux_host_class_hid_client_local_instance == UX_NULL)
        tx_thread_sleep(10);

    /* Get the keyboard instance */
    keyboard =  (UX_HOST_CLASS_HID_KEYBOARD *)hid_client -> ux_host_class_hid_client_local_instance;

    /* Set idle time and report id. */
    idle_time = 0;
    report_id = 0;

    /* Get the slave hid class and hid instance. */
    slave_class = _ux_system_slave -> ux_system_slave_device.ux_slave_device_first_interface -> ux_slave_interface_class;
    slave_hid = _ux_system_slave -> ux_system_slave_device.ux_slave_device_first_interface -> ux_slave_interface_class_instance;

    /* Get the endpoint and transfer request.  */
    control_endpoint =  &hid -> ux_host_class_hid_device -> ux_device_control_endpoint;
    transfer_request = &control_endpoint -> ux_endpoint_transfer_request;

#if !defined(UX_DEVICE_STANDALONE)

    /* Suspend the device interrupt thread so it doesn't get take reports,
       thus preventing us from using a control transfer to get them. */
    status = ux_utility_thread_suspend(&slave_class -> ux_slave_class_thread);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
#else

    /* Set HID state to EXIT, no IDLE/Event processing.  */
    slave_hid -> ux_device_class_hid_event_state = UX_STATE_EXIT;
    tx_thread_sleep(1);
#endif

    /* Set event buffer. */
    event_buffer[0] = 0x01;
    event_buffer[1] = 0x02;
    event_buffer[2] = 0x03;
    event_buffer[3] = 0x04;
    event_buffer[4] = 0x05;
    event_buffer[5] = 0x06;
    event_buffer[6] = 0x07;
    event_buffer[7] = 0x08;

    /**************************************************/
    /** Test case: 'case UX_DEVICE_CLASS_HID_COMMAND_GET_REPORT:' **/
    /**************************************************/

    /* Set the hid event. */
    hid_event.ux_device_class_hid_event_length = sizeof(event_buffer);
    ux_utility_memory_copy(hid_event.ux_device_class_hid_event_buffer, event_buffer, hid_event.ux_device_class_hid_event_length);

    /* Add the event. */
    status = ux_device_class_hid_event_set(slave_hid, &hid_event);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create a transfer request for the GET_REPORT request.  */
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  hid_event.ux_device_class_hid_event_length;
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_GET_REPORT;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             (UINT)((UX_HOST_CLASS_HID_REPORT_TYPE_INPUT << 8) | report_id);
    transfer_request -> ux_transfer_request_index =             hid -> ux_host_class_hid_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Send request to HCD layer.  */
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    status = ux_utility_memory_compare(buffer, event_buffer, hid_event.ux_device_class_hid_event_length);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: 'case UX_DEVICE_CLASS_HID_COMMAND_SET_REPORT:' **/
    /**************************************************/

    /* Create a transfer request for the SET_REPORT request.  */
    /* NOTE: Right now, the size of the dummy report must not be 8 bytes. This is due to a bug in sim_transaction_schedule(). Refer to bug file.  */
    transfer_request -> ux_transfer_request_data_pointer =      dummy_report;
    transfer_request -> ux_transfer_request_requested_length =  sizeof(dummy_report);
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_SET_REPORT;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             (UINT)((UX_HOST_CLASS_HID_REPORT_TYPE_OUTPUT << 8) | report_id);
    transfer_request -> ux_transfer_request_index =             hid -> ux_host_class_hid_interface -> ux_interface_descriptor.bInterfaceNumber;

    set_report_test_started = 1;

    /* Send request to HCD layer.  */
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: 'case UX_DEVICE_CLASS_HID_COMMAND_GET_IDLE:' **/
    /**************************************************/

    /* Create a transfer request for the GET_IDLE request.  */
    transfer_request -> ux_transfer_request_data_pointer =      UX_NULL;
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_GET_IDLE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             (UINT)((idle_time << 8) | report_id);
    transfer_request -> ux_transfer_request_index =             hid -> ux_host_class_hid_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Send request to HCD layer.  */
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: 'case UX_DEVICE_CLASS_HID_COMMAND_SET_IDLE:' **/
    /**************************************************/

    /* Create a transfer request for the SET_IDLE request.  */
    transfer_request -> ux_transfer_request_data_pointer =      UX_NULL;
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_SET_IDLE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             (UINT)((idle_time << 8) | report_id);
    transfer_request -> ux_transfer_request_index =             hid -> ux_host_class_hid_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Send request to HCD layer.  */
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: 'case UX_DEVICE_CLASS_HID_COMMAND_GET_PROTOCOL:' **/
    /**************************************************/

    /* Default protocol should be report protocol.  */
    if (ux_device_class_hid_protocol_get(hid_device) != 1)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create a transfer request for the GET_PROTOCOL request.  */
    transfer_request -> ux_transfer_request_data_pointer =      dummy_report;
    transfer_request -> ux_transfer_request_requested_length =  1;
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_GET_PROTOCOL;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             hid -> ux_host_class_hid_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Send request to HCD layer.  */
    dummy_report[0] = 0xFF;
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Before protocol change it must be report protocol (1).  */
    if (status == UX_SUCCESS && dummy_report[0] != 1)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: 'case UX_DEVICE_CLASS_HID_COMMAND_SET_PROTOCOL:' **/
    /**************************************************/

    /* Create a transfer request for the SET_PROTOCOL request.  */
    transfer_request -> ux_transfer_request_data_pointer =      UX_NULL;
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_SET_PROTOCOL;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0; /* boot report  */
    transfer_request -> ux_transfer_request_index =             hid -> ux_host_class_hid_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Send request to HCD layer.  */
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Protocol should be boot protocol.  */
    if (status == UX_SUCCESS &&
        ux_device_class_hid_protocol_get(hid_device) != 0)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create a transfer request for the GET_PROTOCOL request.  */
    transfer_request -> ux_transfer_request_data_pointer =      dummy_report;
    transfer_request -> ux_transfer_request_requested_length =  1;
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_GET_PROTOCOL;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             hid -> ux_host_class_hid_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* Send request to HCD layer.  */
    dummy_report[0] = 0xFF;
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* After protocol change it must be boot device (0).  */
    if (status == UX_SUCCESS && dummy_report[0] != 0)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: 'default:' (unknown request) **/
    /**************************************************/

    /* Create a transfer request for the an unknown request.  */
    transfer_request -> ux_transfer_request_data_pointer =      UX_NULL;
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          0xdeadbeef;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0;
    transfer_request -> ux_transfer_request_index =             hid -> ux_host_class_hid_interface -> ux_interface_descriptor.bInterfaceNumber;

    /* We expect error here */
    error_is_expected = UX_TRUE;

    /* Send request to HCD layer.  */
    status =  ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("Error on line %d, report error expected here\n", __LINE__);
        test_control_return(1);
    }
    error_is_expected = UX_FALSE;

    /* Now disconnect the device.  */
    _ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}
