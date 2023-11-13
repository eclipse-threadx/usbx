/* TODO: some common stuff from storage we might want to pull out:
    -memory check - pretty good
    -connect and disconnect
    -getting class/instance
*/

#include "ux_api.h"
#include "ux_utility.h"
#include "ux_host_class_hid.h"
#include "ux_device_class_hid.h"
#include "ux_host_class_hid_remote_control.h"

#include "ux_test.h"
#include "ux_test_actions.h"
#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"


#define     LSB(x)              (x & 0xff)
#define     MSB(x)              ((x & 0xff00) >> 8)

/* Define constants.  */
#define                             UX_DEMO_STACK_SIZE  	1024
#define                             UX_DEMO_MEMORY_SIZE     (64*1024)

/* Define local/extern function prototypes.  */
static void                         test_main_thread_entry(ULONG);

/* Define global data structures.  */
static UCHAR                                usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static TX_THREAD                            test_main_thread;
static TX_THREAD                            test_slave_thread;
static UCHAR                                test_slave_thread_stack[4096];
static UX_HOST_CLASS                        *global_host_hid_class;
static UX_HOST_CLASS_HID                    *global_host_hid;
static UX_SLAVE_CLASS_HID                   *global_slave_hid;
static UX_SLAVE_CLASS_HID                   *global_slave_hid_persistent;
static UX_HOST_CLASS_HID_CLIENT             *global_host_hid_client;
static UX_HOST_CLASS_HID_REMOTE_CONTROL     *global_host_remote_control;
static UX_SLAVE_CLASS_HID_PARAMETER         global_slave_hid_parameter;
static UX_HCD                               *global_hcd;


static UCHAR hid_report_descriptor[] = {

    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x02,                    //   USAGE (Numeric Key Pad)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x0a,                    //     USAGE_MAXIMUM (Button 10)
    0x15, 0x01,                    //     LOGICAL_MINIMUM (1)
    0x25, 0x0a,                    //     LOGICAL_MAXIMUM (10)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x00,                    //     INPUT (Data,Ary,Abs)
    0xc0,                          //   END_COLLECTION
    0x05, 0x0c,                    //   USAGE_PAGE (Consumer Devices)
    0x09, 0x86,                    //   USAGE (Channel)
    0x09, 0xe0,                    //   USAGE (Volume)
    0x15, 0xff,                    //   LOGICAL_MINIMUM (-1)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x02,                    //   REPORT_SIZE (2)
    0x95, 0x02,                    //   REPORT_COUNT (2)
    0x81, 0x46,                    //   INPUT (Data,Var,Rel,Null)
    0xc0                           // END_COLLECTION
};

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
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(sizeof(hid_report_descriptor)),
        MSB(sizeof(hid_report_descriptor)),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };

#define FULL_SPEED_REPORT_DESCRIPTOR_LENGTH_LSB_POS  (0x12 + 0x09 + 0x09 + 0x7)
#define FULL_SPEED_REPORT_DESCRIPTOR_LENGTH_MSB_POS  (FULL_SPEED_REPORT_DESCRIPTOR_LENGTH_LSB_POS + 1)


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
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(sizeof(hid_report_descriptor)),
        MSB(sizeof(hid_report_descriptor)),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };

#define HIGH_SPEED_REPORT_DESCRIPTOR_LENGTH_LSB_POS  (0x12 + 0x0a + 0x09 + 0x09 + 0x7)
#define HIGH_SPEED_REPORT_DESCRIPTOR_LENGTH_MSB_POS  (HIGH_SPEED_REPORT_DESCRIPTOR_LENGTH_LSB_POS + 1)


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

/* Functions from storage basic test. */

static VOID get_global_hid_values()
{

    UX_TEST_CHECK_SUCCESS(ux_host_stack_class_get(_ux_system_host_class_hid_name, &global_host_hid_class));
    UX_TEST_CHECK_SUCCESS(ux_host_stack_class_instance_get(global_host_hid_class, 0, (void **) &global_host_hid));
    UX_TEST_ASSERT(global_host_hid -> ux_host_class_hid_state == UX_HOST_CLASS_INSTANCE_LIVE);
    global_host_hid_client = global_host_hid -> ux_host_class_hid_client;
    UX_TEST_ASSERT(global_host_hid_client -> ux_host_class_hid_client_local_instance != UX_NULL);
    global_host_remote_control = (UX_HOST_CLASS_HID_REMOTE_CONTROL *)global_host_hid_client -> ux_host_class_hid_client_local_instance;
}

static VOID wait_for_enum_completion_and_get_global_hid_values()
{

    ux_test_wait_for_enum_thread_completion();
    get_global_hid_values();
}

/* Returns whether or not the enumeration succeeded. */
static VOID connect_host_and_slave()
{

    ux_test_connect_slave_and_host_wait_for_enum_completion();
    get_global_hid_values();
}

/* General HID utilities. */

void set_report_descriptor(UCHAR *report_descriptor, ULONG report_descriptor_length)
{

    /* Should only be called if the host and slave is disconnected. */
    UX_TEST_ASSERT(global_hcd->ux_hcd_nb_devices == 0);
    UX_TEST_ASSERT(_ux_system_slave->ux_system_slave_device.ux_slave_device_state == UX_DEVICE_RESET);

    global_slave_hid_persistent->ux_device_class_hid_report_address = report_descriptor;
    global_slave_hid_persistent->ux_device_class_hid_report_length = report_descriptor_length;

    device_framework_full_speed[FULL_SPEED_REPORT_DESCRIPTOR_LENGTH_LSB_POS] = LSB(report_descriptor_length);
    device_framework_full_speed[FULL_SPEED_REPORT_DESCRIPTOR_LENGTH_MSB_POS] = MSB(report_descriptor_length);

    device_framework_high_speed[HIGH_SPEED_REPORT_DESCRIPTOR_LENGTH_LSB_POS] = LSB(report_descriptor_length);
    device_framework_high_speed[HIGH_SPEED_REPORT_DESCRIPTOR_LENGTH_MSB_POS] = MSB(report_descriptor_length);
}

UINT slave_hid_callback(UX_SLAVE_CLASS_HID *hid, UX_SLAVE_CLASS_HID_EVENT *event)
{
    return 0;
}

VOID slave_class_hid_instance_activate(VOID *instance)
{

    if (global_slave_hid_persistent)
        UX_TEST_ASSERT(global_slave_hid_persistent == instance);
    global_slave_hid_persistent = instance;

    global_slave_hid = instance;
}

VOID slave_class_hid_instance_deactivate(VOID *instance)
{

    global_slave_hid = UX_NULL;
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_hid_remote_control_tests_application_define(void *first_unused_memory)
#endif
{

UINT    status;
CHAR    *stack_pointer;
CHAR    *memory_pointer;


    /* Inform user.  */
    printf("Running HID Remote Control Tests.................................... ");

    stepinfo("\n");

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
    _ux_utility_error_callback_register(ux_test_error_callback);

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
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_remote_control_name, ux_host_class_hid_remote_control_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, sizeof(device_framework_high_speed),
                                         device_framework_full_speed, sizeof(device_framework_full_speed),
                                         string_framework, STRING_FRAMEWORK_LENGTH,
                                         language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH, UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the hid class parameters for a mouse.  */
    global_slave_hid_parameter.ux_slave_class_hid_instance_activate =         slave_class_hid_instance_activate;
    global_slave_hid_parameter.ux_slave_class_hid_instance_deactivate =       slave_class_hid_instance_deactivate;
    global_slave_hid_parameter.ux_device_class_hid_parameter_report_address = hid_report_descriptor;
    global_slave_hid_parameter.ux_device_class_hid_parameter_report_length  = sizeof(hid_report_descriptor);
    global_slave_hid_parameter.ux_device_class_hid_parameter_callback       = slave_hid_callback;

    /* Initilize the device hid class. The class is connected with interface 2. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                             1, 2, (VOID *)&global_slave_hid_parameter);
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
    
    global_hcd = &_ux_system_host->ux_system_host_hcd_array[0];

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&test_main_thread, "test_main_thread", test_main_thread_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
}

/* basic_test resources */

static UINT basic_test_get_next_channel_volume_value(ULONG value)
{

    if (value == 0x03)
        return 0x00;
    else if (value == 0x00)
        return 0x01;
    else if (value == 0x01)
        return 0x03;

    return 0xff;
}

static void basic_test_slave_thread_entry(ULONG arg)
{

UX_SLAVE_CLASS_HID_EVENT    hid_event;
ULONG                       value;
UINT                        max_num_loops;


    /* reset the HID event structure.  */
    ux_utility_memory_set(&hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));

    /* Set length of event.  */
    hid_event.ux_device_class_hid_event_length = 1;

    /* Set initial keypad value. */
    hid_event.ux_device_class_hid_event_buffer[0] = 0x01;

    /* Set initial channel value. */
    hid_event.ux_device_class_hid_event_buffer[0] |= (0x03 << 4);

    /* Set initial volume value. */
    hid_event.ux_device_class_hid_event_buffer[0] |= (0x01 << 6);

    max_num_loops = 2*UX_HOST_CLASS_HID_REMOTE_CONTROL_USAGE_ARRAY_LENGTH;
    while (max_num_loops--)
    {

        stepinfo("    slave - max_num_loops: %d\n", max_num_loops);

        /* Wait for host to receive. */
        ux_utility_thread_sleep(2);

        /* Set the mouse event.  */
        UX_TEST_CHECK_SUCCESS(ux_device_class_hid_event_set(global_slave_hid, &hid_event));

        /* Change keypad value. */
        value = hid_event.ux_device_class_hid_event_buffer[0] & 0x0f;
        if (value >= 0x0a)
            value = 0x01;
        else
            value++;

        hid_event.ux_device_class_hid_event_buffer[0] &= 0xf0;
        hid_event.ux_device_class_hid_event_buffer[0] |= value;

        /* Change channel value. */
        value = ((hid_event.ux_device_class_hid_event_buffer[0] & 0x30) >> 4);
        hid_event.ux_device_class_hid_event_buffer[0] &= ~0x30;
        hid_event.ux_device_class_hid_event_buffer[0] |= (basic_test_get_next_channel_volume_value(value) << 4);

        /* Change volume value. */
        value = ((hid_event.ux_device_class_hid_event_buffer[0] & 0xc0) >> 6);
        hid_event.ux_device_class_hid_event_buffer[0] &= ~0xc0;
        hid_event.ux_device_class_hid_event_buffer[0] |= (basic_test_get_next_channel_volume_value(value) << 6);
    }
}

static void basic_test()
{

UINT    max_num_loops;
ULONG   usage;
ULONG   value;
ULONG   expected_keypad_value;
ULONG   expected_channel_value;
ULONG   expected_volume_value;


    stepinfo("basic_test\n");

    UX_TEST_CHECK_SUCCESS(tx_thread_create(&test_slave_thread, "test_slave_thread", basic_test_slave_thread_entry, 0,
                                           test_slave_thread_stack, UX_DEMO_STACK_SIZE,
                                           20, 20, 1, TX_AUTO_START));

    /* Initialize expected values. */
    expected_keypad_value = 0x01;
    expected_channel_value = 0x03;
    expected_volume_value = 0x01;

    /* Set number of successful loops to execute. */
    max_num_loops = 2*UX_HOST_CLASS_HID_REMOTE_CONTROL_USAGE_ARRAY_LENGTH;
    while (max_num_loops--)
    {

        stepinfo("    host - max_num_loops:  %d\n", max_num_loops);

        /* Wait for an event from the device. Each event should have 3 usages. The first is the keypad. */
        while (ux_host_class_hid_remote_control_usage_get(global_host_remote_control, &usage, &value) != UX_SUCCESS)
            tx_thread_sleep(1);

        if (usage != (0x00090000 | expected_keypad_value) || value != expected_keypad_value)
        {

            printf("Error on line %d. usage: 0x%lx, expected usage: 0x%lx, value: 0x%lx, expected_keypad_value: 0x%lx\n", 
                   __LINE__, usage, 0x00090000 | expected_keypad_value, value, expected_keypad_value);
            test_control_return(1);
        }

        if (++expected_keypad_value > 0x0a)
            expected_keypad_value = 1;

        /* Get the channel value. */
        ux_host_class_hid_remote_control_usage_get(global_host_remote_control, &usage, &value);
        if (usage != 0x000c0086 || value != expected_channel_value)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        expected_channel_value = basic_test_get_next_channel_volume_value(value);

        /* Get the volume value. */
        ux_host_class_hid_remote_control_usage_get(global_host_remote_control, &usage, &value);
        if (usage != 0x000c00e0 || value != expected_volume_value)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        expected_volume_value = basic_test_get_next_channel_volume_value(value);
    }

    UX_TEST_CHECK_SUCCESS(tx_thread_terminate(&test_slave_thread));
    UX_TEST_CHECK_SUCCESS(tx_thread_delete(&test_slave_thread));
}

/* event_overflow_test resources */

#define EBT_MAX_EVENTS ((UX_HOST_CLASS_HID_REMOTE_CONTROL_USAGE_ARRAY_LENGTH/2) - 1)
#define EBT_NUM_OVERFLOW_EVENTS 100

static TX_SEMAPHORE ebt_slave_wakes_host_semaphore;
static TX_SEMAPHORE ebt_host_wakes_slave_semaphore;

static UCHAR host_event_buffer_test_hid_report_descriptor[] = {

    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x0c,                    //   USAGE_PAGE (Consumer Devices)
    0x09, 0xe0,                    //   USAGE (Volume)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0xff,                    //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x46,                    //   INPUT (Data,Var,Rel,Null)
    0xc0                           // END_COLLECTION
};

static void event_buffer_test_slave_thread_entry(ULONG arg)
{

UX_SLAVE_CLASS_HID_EVENT    hid_event = { 0 };
UINT                        i;


    /* Add the exact amount. Remember, the host's usage array consists of pairs,
       hence the divide by two. */
    for (i = 0; i < EBT_MAX_EVENTS; i++)
    {

        /* Setup and send event. */
        hid_event.ux_device_class_hid_event_length = 1;
        hid_event.ux_device_class_hid_event_buffer[0] = i;
        UX_TEST_CHECK_SUCCESS(ux_device_class_hid_event_set(global_slave_hid, &hid_event));

        /* Wait for host to receive it. */
        tx_thread_sleep(2);
    }

    /* Wake up host test thread. */
    tx_semaphore_put(&ebt_slave_wakes_host_semaphore);

    /* Wait for second part of test. */
    tx_semaphore_get(&ebt_host_wakes_slave_semaphore, TX_WAIT_FOREVER);

    /* We expect to receive some errors. */
    ux_test_add_action_to_main_list_multiple(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_BUFFER_OVERFLOW), EBT_NUM_OVERFLOW_EVENTS);

    for (i = 0; i < EBT_MAX_EVENTS + EBT_NUM_OVERFLOW_EVENTS; i++)
    {

        /* Setup and send event. */
        hid_event.ux_device_class_hid_event_length = 1;
        hid_event.ux_device_class_hid_event_buffer[0] = i;
        UX_TEST_CHECK_SUCCESS(ux_device_class_hid_event_set(global_slave_hid, &hid_event));

        /* Wait for host to receive it. */
        tx_thread_sleep(2);
    }

    /* Ensure all of our actions are gone. */
    UX_TEST_ASSERT_MESSAGE(ux_test_check_actions_empty(), "Number of actions remaining: %d\n", ux_test_get_num_actions_left());

    /* Wake up host test thread. */
    tx_semaphore_put(&ebt_slave_wakes_host_semaphore);
}

static void host_event_buffer_test()
{

ULONG   usage;
ULONG   value;
UINT    i;


    stepinfo("event_buffer_overflow_test\n");

    ux_test_disconnect_slave_and_host_wait_for_enum_completion(global_hcd);
    set_report_descriptor(host_event_buffer_test_hid_report_descriptor, sizeof(host_event_buffer_test_hid_report_descriptor));
    connect_host_and_slave();

    UX_TEST_CHECK_SUCCESS(tx_semaphore_create(&ebt_slave_wakes_host_semaphore, "ebt_slave_wakes_host_semaphore", 0));
    UX_TEST_CHECK_SUCCESS(tx_semaphore_create(&ebt_host_wakes_slave_semaphore, "ebt_host_wakes_slave_semaphore", 0));

    UX_TEST_CHECK_SUCCESS(tx_thread_create(&test_slave_thread, "test_slave_thread", event_buffer_test_slave_thread_entry, 0,
                                           test_slave_thread_stack, UX_DEMO_STACK_SIZE,
                                           20, 20, 1, TX_AUTO_START));

    stepinfo("    exact amount\n");

    /* Wait for slave to send exact amount. */
    tx_semaphore_get(&ebt_slave_wakes_host_semaphore, TX_WAIT_FOREVER);

    /* Ensure exact amount was sent. */
    for (i = 0; i < EBT_MAX_EVENTS; i++)
    {

        UX_TEST_CHECK_SUCCESS(ux_host_class_hid_remote_control_usage_get(global_host_remote_control, &usage, &value));
        UX_TEST_ASSERT(usage == 0x000c00e0);
        UX_TEST_ASSERT(value == i);
    }

    /* Should be no more. */
    UX_TEST_CHECK_NOT_SUCCESS(ux_host_class_hid_remote_control_usage_get(global_host_remote_control, &usage, &value));

    stepinfo("    overflow\n");

    /* Wake up slave. */
    tx_semaphore_put(&ebt_host_wakes_slave_semaphore);

    /* Wait for slave to overflow. */
    tx_semaphore_get(&ebt_slave_wakes_host_semaphore, TX_WAIT_FOREVER);

    /* Ensure exact amount was sent. */
    for (i = 0; i < EBT_MAX_EVENTS; i++)
    {

        UX_TEST_CHECK_SUCCESS(ux_host_class_hid_remote_control_usage_get(global_host_remote_control, &usage, &value));
        UX_TEST_ASSERT(usage == 0x000c00e0);
        UX_TEST_ASSERT(value == i);
    }

    /* Should be no more. */
    UX_TEST_CHECK_NOT_SUCCESS(ux_host_class_hid_remote_control_usage_get(global_host_remote_control, &usage, &value));

    UX_TEST_CHECK_SUCCESS(tx_thread_terminate(&test_slave_thread));
    UX_TEST_CHECK_SUCCESS(tx_thread_delete(&test_slave_thread));
}

static void test_main_thread_entry(ULONG arg)
{

UINT    status;
UINT    i;
void    (*tests[])() = 
{
    basic_test,
    host_event_buffer_test,
};


    ux_test_wait_for_enum_thread_completion();
    get_global_hid_values();
    ux_test_memory_test_initialize();
    get_global_hid_values();
    /* Run tests. */
    for (i = 0; i < ARRAY_COUNT(tests); i++)
    {
        tests[i]();

        ux_test_disconnect_slave_and_host_wait_for_enum_completion(global_hcd);
        set_report_descriptor(hid_report_descriptor, sizeof(hid_report_descriptor));
        connect_host_and_slave();

        UX_TEST_ASSERT(ux_test_check_actions_empty());
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

static UINT demo_thread_hid_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{
    return(UX_SUCCESS);
}