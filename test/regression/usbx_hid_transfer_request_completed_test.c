/* This test concentrates on failed hid transfer requests and ensuring they're resent and received correctly.
 * Note: USBX HID has a thread for polling the device. Upon completion, ux_host_class_hid_transfer_request_completed is called,
 * and if the transfer request status is set to unsuccessful, ux_host_class_hid_transfer_request_completed retries the transfer request. */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_mouse.h"
#include "ux_test.h"


static UX_HOST_CLASS_HID_MOUSE  *mouse;
static TX_SEMAPHORE             test_semaphore_host_wakes_slave;


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


#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED 52
static UCHAR device_framework_full_speed[] = {

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
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_MOUSE_REPORT_LENGTH),
        MSB(HID_MOUSE_REPORT_LENGTH),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };


#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED 62
static UCHAR device_framework_high_speed[] = {

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
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_MOUSE_REPORT_LENGTH),
        MSB(HID_MOUSE_REPORT_LENGTH),

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
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_hid_transfer_request_completed_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;


    /* Inform user.  */
    printf("Running HID Transfer Request Completed Test......................... ");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_mouse_name, ux_host_class_hid_mouse_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
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

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the hid class parameters for a mouse.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_mouse_report;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_MOUSE_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;

    /* Initilize the device hid class. The class is connected with interface 2 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,2, (VOID *)&hid_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }


    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    UX_TEST_CHECK_SUCCESS(ux_utility_semaphore_create(&test_semaphore_host_wakes_slave, "test_semaphore_host_wakes_slave", 0));

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main demo thread.  */
    status =  tx_thread_create(&tx_demo_thread_slave_simulation, "tx demo slave simulation", tx_demo_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
}


UINT num_transfer_requests;

static UINT ux_hcd_sim_host_entry_filter(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT status;
    UX_TRANSFER *transfer_request;

    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = (UX_TRANSFER *)parameter;
        /* Fail every three times. */
        if(transfer_request -> ux_transfer_request_completion_function == _ux_host_class_hid_transfer_request_completed &&
            (num_transfer_requests++ % 3) == 0)
        {

            transfer_request -> ux_transfer_request_completion_code = UX_TRANSFER_ERROR;
            transfer_request -> ux_transfer_request_completion_function(transfer_request);

            status = transfer_request -> ux_transfer_request_completion_code;
        }
        else
        {
            status = _ux_hcd_sim_host_entry(hcd, function, parameter);
        }
    }
    else
    {
        status = _ux_hcd_sim_host_entry(hcd, function, parameter);
    }

    return status;
}


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT    status;
UINT    max_num_loops;
UINT    num_loops;
SLONG   cur_mouse_wheel_movement;
SLONG   next_mouse_wheel_movement;
SLONG   cur_mouse_x_position;
SLONG   cur_mouse_y_position;
SLONG   next_mouse_x_position;
SLONG   next_mouse_y_position;
ULONG   cur_mouse_buttons;
UCHAR   next_mouse_buttons;
UX_HCD  *hcd;


    /* Initilize max loop value.  */
    max_num_loops = 16;

    /* Find the HID class */
    status =  demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the HID client */
    hid_client = hid -> ux_host_class_hid_client;

    /* Check if the instance of the keyboard is live */
    while (hid_client -> ux_host_class_hid_client_local_instance == UX_NULL)
        tx_thread_sleep(10);

    /* Get the mouse instance */
    mouse =  (UX_HOST_CLASS_HID_MOUSE *)hid_client -> ux_host_class_hid_client_local_instance;

    /* Change the hcd entry function to our filter function. */
    hcd = UX_DEVICE_HCD_GET(hid -> ux_host_class_hid_device);
    hcd -> ux_hcd_entry_function = ux_hcd_sim_host_entry_filter;

    /* Set number of successful loops to execute. */
    num_loops = 0;
    max_num_loops = 16;

    /* Set initial mouse button values. */
    next_mouse_buttons = 0x01;

    /* Set initial mouse position values. */
    next_mouse_x_position = -8;
    next_mouse_y_position = -8;

    /* Set initial mouse wheel value. */
    next_mouse_wheel_movement = -8;

    while (num_loops++ != max_num_loops)
    {

        /* Wait for device to send. */
        tx_thread_sleep(50);

        UX_TEST_CHECK_SUCCESS(ux_host_class_hid_mouse_buttons_get(mouse, &cur_mouse_buttons));
        UX_TEST_CHECK_SUCCESS(ux_host_class_hid_mouse_position_get(mouse, &cur_mouse_x_position, &cur_mouse_y_position));
        UX_TEST_CHECK_SUCCESS(ux_host_class_hid_mouse_wheel_get(mouse, &cur_mouse_wheel_movement));

        /* Ensure these are the expected values. */
        UX_TEST_ASSERT(cur_mouse_buttons == next_mouse_buttons &&
                       cur_mouse_x_position == next_mouse_x_position &&
                       cur_mouse_y_position == next_mouse_y_position &&
                       cur_mouse_wheel_movement == next_mouse_wheel_movement);

        /* Increment values. */
        next_mouse_buttons = 0x07 & (next_mouse_buttons + 1);
        next_mouse_x_position++;
        next_mouse_y_position++;
        next_mouse_wheel_movement++;

        /* Tell the device to send the next. */
        UX_TEST_CHECK_SUCCESS(ux_utility_semaphore_put(&test_semaphore_host_wakes_slave));
    }

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

static UINT    demo_thread_hid_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{
    return(UX_SUCCESS);
}

static void  tx_demo_thread_slave_simulation_entry(ULONG arg)
{

UX_SLAVE_DEVICE                 *device;
UX_SLAVE_INTERFACE              *interface;
UX_SLAVE_CLASS_HID              *hid;
UX_SLAVE_CLASS_HID_EVENT        hid_event;

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* reset the HID event structure.  */
    ux_utility_memory_set(&hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));

    /* Set length of event.  */
    hid_event.ux_device_class_hid_event_length = 4;

    /* Set initial value for buttons. The first three bits are for buttons, the rest are padding. */
    hid_event.ux_device_class_hid_event_buffer[0] = 1;

    /* Set initial value for x and y positions. */
    hid_event.ux_device_class_hid_event_buffer[1] = -8;
    hid_event.ux_device_class_hid_event_buffer[2] = -8;

    /* Set initial value for mouse wheel. */
    hid_event.ux_device_class_hid_event_buffer[3] = -8;

    while (1)
    {

        /* Is the device configured ? */
        while (device -> ux_slave_device_state != UX_DEVICE_CONFIGURED)

            /* Then wait.  */
            tx_thread_sleep(10);

        /* Until the device stays configured.  */
        while (device -> ux_slave_device_state == UX_DEVICE_CONFIGURED)
        {

            /* Get the interface.  We use the first interface, this is a simple device.  */
            interface =  device -> ux_slave_device_first_interface;

            /* From that interface, derive the HID owner.  */
            hid = interface -> ux_slave_interface_class_instance;

            /* Set the mouse event.  */
            ux_device_class_hid_event_set(hid, &hid_event);

            /* Change the buttons.  */
            hid_event.ux_device_class_hid_event_buffer[0] = (0x7 & hid_event.ux_device_class_hid_event_buffer[0] + 1);

            /* Change the x, y, and wheel positions. These are relative values.  */
            hid_event.ux_device_class_hid_event_buffer[1] = 1;
            hid_event.ux_device_class_hid_event_buffer[2] = 1;
            hid_event.ux_device_class_hid_event_buffer[3] = 1;

            /* Wait for host to receive. */
            UX_TEST_CHECK_SUCCESS(ux_utility_semaphore_get(&test_semaphore_host_wakes_slave, UX_WAIT_FOREVER));
        }
    }
}