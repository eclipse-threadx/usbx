#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"

#define TEST_HID_EVENTS_SIZE (UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE + 1)

#define DUMMY_USBX_MEMORY_SIZE (64*1024)
static UCHAR dummy_usbx_memory[DUMMY_USBX_MEMORY_SIZE];

static UCHAR hid_report_descriptor[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x00,                    //   REPORT_ID (0)
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
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_class_hid_event_get_AND_set_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Inform user.  */
    printf("Running ux_device_class_hid_event_get_AND_set test.................. ");

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
    hid_parameter.ux_device_class_hid_parameter_report_id      = UX_TRUE;

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
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT status;
UX_HOST_CLASS_HID_KEYBOARD     *keyboard;
ULONG                           tmp;
UX_SLAVE_DEVICE                *device;
UX_SLAVE_CLASS_HID             *device_hid;
UX_SLAVE_CLASS_HID_EVENT        hid_events[TEST_HID_EVENTS_SIZE];
UX_SLAVE_CLASS_HID_EVENT        hid_event;
UX_SLAVE_INTERFACE             *interface;
UX_SLAVE_CLASS                 *slave_class;
UX_SLAVE_CLASS_HID              slave_class_hid;
UINT                            report_id;

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

    /* Derive device hid class. */
    device =  &_ux_system_slave -> ux_system_slave_device;
    interface = device -> ux_slave_device_first_interface;
    slave_class = interface -> ux_slave_interface_class;
    device_hid = interface -> ux_slave_interface_class_instance;

    for (report_id = 0; report_id < 2; report_id ++)
    {

        /* Modify report ID setting! */
        device_hid -> ux_device_class_hid_report_id = report_id;

        /* Initialize events. */
        for (int i = 0; i < TEST_HID_EVENTS_SIZE; i++)
        {

            if (report_id == 0)
                hid_events[i].ux_device_class_hid_event_length = UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH;
            else
                hid_events[i].ux_device_class_hid_event_length = UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH - 1;

            for (int j = 0; j < UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH; j++)
                hid_events[i].ux_device_class_hid_event_buffer[j] = i;
        }

        /* Suspend the device interrupt thread so it doesn't get take reports,
        thus preventing us from using a control transfer to get them. */
        status = ux_utility_thread_suspend(&slave_class -> ux_slave_class_thread);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
            test_control_return(1);
        }

        /**************************************************/
        /** Test case: if (current_hid_event == UX_NULL). **/
        /**************************************************/

        /* Nullify head of round robin buffer. */
        slave_class_hid.ux_device_class_hid_event_array_head = NULL;

        status = _ux_device_class_hid_event_set(&slave_class_hid, 0);
        if (status != UX_ERROR)
        {

            printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
            test_control_return(1);
        }

        /**************************************************/
        /** Test case: get an event when the buffer is empty. **/
        /**************************************************/

        status = _ux_device_class_hid_event_get(device_hid, &hid_event);
        if (status == UX_SUCCESS)
        {

            printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
            test_control_return(1);
        }

        /**************************************************/
        /** Test case: add one. **/
        /**************************************************/

        status = _ux_device_class_hid_event_set(device_hid, &hid_events[0]);
        if (report_id & 1)
        {
            /* Branch case: if current_hid_event length too big.  */
            device_hid->ux_device_class_hid_event_array_tail->ux_device_class_hid_event_length = UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH + 1;
        }
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
            test_control_return(1);
        }

        status = _ux_device_class_hid_event_get(device_hid, &hid_event);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
            test_control_return(1);
        }

        status = ux_utility_memory_compare(&hid_event.ux_device_class_hid_event_buffer[report_id],
                                        &hid_events[0].ux_device_class_hid_event_buffer[report_id],
                                        UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH - report_id);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
            test_control_return(1);
        }

        /**************************************************/
        /** Test case: fill halfway. **/
        /**************************************************/

        for (int i = 0; i < UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE / 2; i++)
        {

            status = _ux_device_class_hid_event_set(device_hid, &hid_events[i]);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
                test_control_return(1);
            }
        }

        for (int i = 0; i < UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE / 2; i++)
        {

            status = _ux_device_class_hid_event_get(device_hid, &hid_event);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
                test_control_return(1);
            }

            status = ux_utility_memory_compare(&hid_event.ux_device_class_hid_event_buffer[report_id],
                                            &hid_events[i].ux_device_class_hid_event_buffer[report_id],
                                            UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH - report_id);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
                test_control_return(1);
            }
        }

        /**************************************************/
        /** Test case: fill all the way. **/
        /**************************************************/

        /* Note that with the way the buffer is set up, the maximum number of
        events able to be added is one less. */
        for (int i = 0; i < (UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE - 1); i++)
        {

            status = _ux_device_class_hid_event_set(device_hid, &hid_events[i]);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
                test_control_return(1);
            }
        }

        for (int i = 0; i < (UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE - 1); i++)
        {

            status = _ux_device_class_hid_event_get(device_hid, &hid_event);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
                test_control_return(1);
            }

            status = ux_utility_memory_compare(&hid_event.ux_device_class_hid_event_buffer[report_id],
                                            &hid_events[i].ux_device_class_hid_event_buffer[report_id],
                                            UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH - report_id);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
                test_control_return(1);
            }
        }

        /**************************************************/
        /** Test case: fill past max. **/
        /**************************************************/

        /* Note that with the way the buffer is set up, the maximum number of
        events able to be added is one less. */
        for (int i = 0; i < (UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE - 1); i++)
        {

            status = _ux_device_class_hid_event_set(device_hid, &hid_events[i]);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
                test_control_return(1);
            }
        }

        status = _ux_device_class_hid_event_set(device_hid, &hid_events[UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE - 1]);
        if (status == UX_SUCCESS)
        {

            printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
            test_control_return(1);
        }

        for (int i = 0; i < (UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE - 1); i++)
        {

            status = _ux_device_class_hid_event_get(device_hid, &hid_event);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
                test_control_return(1);
            }

            status = ux_utility_memory_compare(&hid_event.ux_device_class_hid_event_buffer[report_id],
                                            &hid_events[i].ux_device_class_hid_event_buffer[report_id],
                                            UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH - report_id);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
                test_control_return(1);
            }
        }

        /**************************************************/
        /** Test case: if (device -> ux_slave_device_state !=  UX_DEVICE_CONFIGURED). **/
        /** Why direct: The host indirectly calls this via a report get request, however, in order to make this request to a device, **/
        /** the device must have been configured already. **/
        /**************************************************/

        /* Put device in unconfigured state. */
        tmp = _ux_system_slave -> ux_system_slave_device.ux_slave_device_state;
        _ux_system_slave -> ux_system_slave_device.ux_slave_device_state = ~UX_DEVICE_CONFIGURED;

        status = _ux_device_class_hid_event_get(device_hid, &hid_event);
        if (status != UX_DEVICE_HANDLE_UNKNOWN)
        {

            printf("Error on line %d, test %d, error code: 0x%x\n", __LINE__, report_id, status);
            test_control_return(1);
        }

        /* Restore state for next test. */
        _ux_system_slave -> ux_system_slave_device.ux_slave_device_state = tmp;
    }

    /**************************************************/
    /** Test case: report ID is 1 and event length is max. **/
    /** Specific condition: if (hid_event -> ux_device_class_hid_event_length + 1 > UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH). **/
    /**************************************************/

    hid_event.ux_device_class_hid_event_length = UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH;
    hid_event.ux_device_class_hid_event_report_id = UX_TRUE;
    status = _ux_device_class_hid_event_set(device_hid, &hid_event);
    if (status == UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: device not connected..             */
    /**************************************************/

    /* Now disconnect the device.  */
    _ux_device_stack_disconnect();

    status = ux_device_class_hid_event_get(device_hid, &hid_event);
    if (status != UX_DEVICE_HANDLE_UNKNOWN)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

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
