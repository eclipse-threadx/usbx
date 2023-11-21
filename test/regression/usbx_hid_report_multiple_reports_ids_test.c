/* This test ensures that the parser correctly handles multiple report IDs for input reports. */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"

#define TEST_LOG_N                  8

static TX_SEMAPHORE                 test_semaphore_host_wakes_device;

static ULONG                        test_log_count = 0;
static ULONG                        test_log_index = 0;
static INT                          test_log_report_id[TEST_LOG_N] = {-1};
static UCHAR                        test_log_report_data[TEST_LOG_N][32];
static ULONG                        test_log_report_data_len[TEST_LOG_N];

static UCHAR                        test_device_report_id = 0;
static UCHAR                        test_device_report_count = 1;
static UCHAR                        test_device_report_fill = 0x5A;
static UCHAR                        test_device_report_len = 1; /* Without ID */

static UCHAR hid_report_descriptor[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)

    /* 0: 4 x 1-bit values  */
    0x19, 0x01,                    //   USAGE_MINIMUM (1)
    0x29, 0x04,                    //   USAGE_MAXIMUM (4)
    0x75, 0x04,                    //   REPORT_SIZE (4)
    0x95, 0x04,                    //   REPORT_COUNT (4)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

    /* 2: 3 x bytes  */
    0x85, 0x02,                    //   REPORT_ID (2)
    0x19, 0x05,                    //   USAGE_MINIMUM (5)
    0x29, 0x07,                    //   USAGE_MAXIMUM (7)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

    /* 4: 1 x 16-bit value  */
    0x85, 0x04,                    //   REPORT_ID (4)
    0x09, 0x08,                    //   USAGE (8)
    0x75, 0x10,                    //   REPORT_SIZE (16)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

    /* USBX expects keyboards to have at least one output report, otherwise it's an error. */
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)

    0xc0,                          // END_COLLECTION
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



static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    /* Fine, HID client not registered.  */
    if (error_code == UX_HOST_CLASS_HID_UNKNOWN)
        return;

    /* Failed test.  */
    printf("Error on line %d, system_level: %x, system_context: %x, error code: %x\n", __LINE__, system_level, system_context, error_code);
    test_control_return(1);
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_hid_report_multiple_reports_ids_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
UINT                            descriptor_size = HID_REPORT_LENGTH;


    /* Inform user.  */
    printf("Running HID Report Multiple Reports IDs Test........................ ");

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

    /* Initialize the hid class parameters.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_report_descriptor;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;
    /* Report ID not inserted from event field, we manage buffer by ourselvs.  */
    hid_parameter.ux_device_class_hid_parameter_report_id      = UX_FALSE;

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

    UX_TEST_CHECK_SUCCESS(ux_utility_semaphore_create(&test_semaphore_host_wakes_device, "test_semaphore_host_wakes_device", 0));

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

    /* Create the main device thread.  */
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


static inline void test_log_reset(UCHAR data_fill)
{
ULONG i;
    test_log_count = 0;
    test_log_index = 0;
    for (i = 0; i < TEST_LOG_N; i ++)
        test_log_report_id[i] = -1;
    ux_utility_memory_set(test_log_report_data_len, 0, sizeof(test_log_report_data_len));
    ux_utility_memory_set(test_log_report_data, 0, sizeof(test_log_report_data));
}
static inline void test_log_add(UCHAR report_id, UCHAR *data_buf, ULONG data_size)
{
    test_log_count ++;
    if (test_log_index < TEST_LOG_N)
    {
        test_log_report_id[test_log_index] = report_id;
        test_log_report_data_len[test_log_index] = data_size;
        if (data_size)
            ux_utility_memory_copy(test_log_report_data[test_log_index], data_buf, data_size);
        test_log_index ++;
    }
}

static VOID demo_host_class_hid_report0_callback(UX_HOST_CLASS_HID_REPORT_CALLBACK *callback)
{
    test_log_add(0, callback->ux_host_class_hid_report_callback_buffer,callback->ux_host_class_hid_report_callback_actual_length);
}
static VOID demo_host_class_hid_report2_callback(UX_HOST_CLASS_HID_REPORT_CALLBACK *callback)
{
    test_log_add(2, callback->ux_host_class_hid_report_callback_buffer,callback->ux_host_class_hid_report_callback_actual_length);
}
static VOID demo_host_class_hid_report4_callback(UX_HOST_CLASS_HID_REPORT_CALLBACK *callback)
{
    test_log_add(4, callback->ux_host_class_hid_report_callback_buffer,callback->ux_host_class_hid_report_callback_actual_length);
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                status;
UX_HOST_CLASS_HID_REPORT_GET_ID     report_id;
UX_HOST_CLASS_HID_CLIENT_REPORT     input_report_request;
UX_HOST_CLASS_HID_REPORT            *input_report_descriptor;
UX_HOST_CLASS_HID_REPORT_CALLBACK   call_back;
ULONG                               c, i;


    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Start periodic report transfer.  */
    _ux_host_class_hid_periodic_report_start(hid);

    /* Reset logs.  */
    test_log_reset(0);

    /* Trigger device events.  */
    test_device_report_id = 1;
    test_device_report_count = 2;
    test_device_report_fill = 0xef; /* 0xef, 0xf0  */
    test_device_report_len = 3; /* ...  */
    UX_TEST_CHECK_SUCCESS(ux_utility_semaphore_put(&test_semaphore_host_wakes_device));

    /* Check received callbacks.  */
    tx_thread_sleep(50);
    UX_TEST_ASSERT(test_log_count == 0);

    /* Get the first input report descriptor. */
    report_id.ux_host_class_hid_report_get_report = UX_NULL;
    report_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;
    if (input_report_descriptor -> ux_host_class_hid_report_id != 0)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Register callback for the report descriptor.  */
    call_back.ux_host_class_hid_report_callback_id =         input_report_descriptor -> ux_host_class_hid_report_id;
    call_back.ux_host_class_hid_report_callback_function =   demo_host_class_hid_report0_callback;
    call_back.ux_host_class_hid_report_callback_flags =      UX_HOST_CLASS_HID_REPORT_RAW;
    call_back.ux_host_class_hid_report_callback_buffer =     UX_NULL;
    call_back.ux_host_class_hid_report_callback_length =     0;
    status =  _ux_host_class_hid_report_callback_register(hid, &call_back);

    /* Reset logs.  */
    test_log_reset(0);

    /* Trigger device events.  */
    test_device_report_id = 0;
    test_device_report_count = 3;
    test_device_report_fill = 0x30; /* 0x30, 0x31, 0x32  */
    test_device_report_len = 1; /* 4 x 1-bit value.  */
    UX_TEST_CHECK_SUCCESS(ux_utility_semaphore_put(&test_semaphore_host_wakes_device));

    /* Check received callbacks.  */
    tx_thread_sleep(50);
    UX_TEST_ASSERT(test_log_count == 3);
    for (c = 0; c < 3; c ++)
    {
        UX_TEST_ASSERT(test_log_report_id[c] == 0);
        UX_TEST_ASSERT(test_log_report_data_len[c] == 2);
        UX_TEST_ASSERT(test_log_report_data[c][0] == 0);
        UX_TEST_ASSERT(test_log_report_data[c][1] == (0x30 + c));
    }

    /* Get the second input report descriptor. */
    report_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;
    if (input_report_descriptor->ux_host_class_hid_report_id != 2)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Register callback for the report descriptor.  */
    call_back.ux_host_class_hid_report_callback_id =         input_report_descriptor -> ux_host_class_hid_report_id;
    call_back.ux_host_class_hid_report_callback_function =   demo_host_class_hid_report2_callback;
    call_back.ux_host_class_hid_report_callback_flags =      UX_HOST_CLASS_HID_REPORT_RAW;
    call_back.ux_host_class_hid_report_callback_buffer =     UX_NULL;
    call_back.ux_host_class_hid_report_callback_length =     0;
    status =  _ux_host_class_hid_report_callback_register(hid, &call_back);

    /* Reset logs.  */
    test_log_reset(0);

    /* Trigger device events.  */
    test_device_report_id = 2;
    test_device_report_count = 5;
    test_device_report_fill = 0x52; /* 0x52 .. 0x56  */
    test_device_report_len = 3; /* 3 x bytes.  */
    UX_TEST_CHECK_SUCCESS(ux_utility_semaphore_put(&test_semaphore_host_wakes_device));

    /* Check received callbacks.  */
    tx_thread_sleep(50);
    UX_TEST_ASSERT(test_log_count == test_device_report_count);
    for (c = 0; c < test_device_report_count; c ++)
    {
        UX_TEST_ASSERT(test_log_report_id[c] == test_device_report_id);
        UX_TEST_ASSERT(test_log_report_data_len[c] == test_device_report_len+1);
        UX_TEST_ASSERT(test_log_report_data[c][0] == test_device_report_id);
        for (i = 0; i < test_device_report_len; i ++)
            UX_TEST_ASSERT(test_log_report_data[c][1 + i] ==
                            (test_device_report_fill + c));
    }


    /* Get the third input report descriptor. */
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;
    if (input_report_descriptor->ux_host_class_hid_report_id != 4)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Register callback for the report descriptor.  */
    call_back.ux_host_class_hid_report_callback_id =         input_report_descriptor -> ux_host_class_hid_report_id;
    call_back.ux_host_class_hid_report_callback_function =   demo_host_class_hid_report4_callback;
    call_back.ux_host_class_hid_report_callback_flags =      UX_HOST_CLASS_HID_REPORT_RAW;
    call_back.ux_host_class_hid_report_callback_buffer =     UX_NULL;
    call_back.ux_host_class_hid_report_callback_length =     0;
    status =  _ux_host_class_hid_report_callback_register(hid, &call_back);

    /* Reset logs.  */
    test_log_reset(0);

    /* Trigger device events.  */
    test_device_report_id = 4;
    test_device_report_count = 7;
    test_device_report_fill = 0x77; /* 0x77 .. 0x7D  */
    test_device_report_len = 2; /* 1 x 16-bit value.  */
    UX_TEST_CHECK_SUCCESS(ux_utility_semaphore_put(&test_semaphore_host_wakes_device));

    /* Check received callbacks.  */
    tx_thread_sleep(50);
    UX_TEST_ASSERT(test_log_count == test_device_report_count);
    for (c = 0; c < test_device_report_count; c ++)
    {
        UX_TEST_ASSERT(test_log_report_id[c] == test_device_report_id);
        UX_TEST_ASSERT(test_log_report_data_len[c] == test_device_report_len+1);
        UX_TEST_ASSERT(test_log_report_data[c][0] == test_device_report_id);
        for (i = 0; i < test_device_report_len; i ++)
            UX_TEST_ASSERT(test_log_report_data[c][1 + i] ==
                            (test_device_report_fill + c));
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
ULONG                           i;

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* reset the HID event structure.  */
    ux_utility_memory_set(&hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));

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

             /* Wait for host to trigger. */
            UX_TEST_CHECK_SUCCESS(ux_utility_semaphore_get(&test_semaphore_host_wakes_device, UX_WAIT_FOREVER));

            for (i = 0; i < test_device_report_count; i ++)
            {

                /* Build the event to test.  */

                /* Set length of event.  */
                hid_event.ux_device_class_hid_event_length = test_device_report_len + 1;

                /* Set ID.  */
                hid_event.ux_device_class_hid_event_buffer[0] = test_device_report_id;

                /* Set fills.  */
                ux_utility_memory_set(&hid_event.ux_device_class_hid_event_buffer[1],
                                    test_device_report_fill + i,
                                    test_device_report_len);

                /* Set the mouse event.  */
                ux_device_class_hid_event_set(hid, &hid_event);
            }
            }
    }
}