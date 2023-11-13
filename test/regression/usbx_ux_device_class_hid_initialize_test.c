#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"


#define DUMMY_USBX_MEMORY_SIZE (64*1024)
static UCHAR dummy_usbx_memory[DUMMY_USBX_MEMORY_SIZE];

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
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_class_hid_initialize_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Inform user.  */
    printf("Running ux_device_class_hid_initialize Test......................... ");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(ux_system_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name, ux_host_class_hid_keyboard_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
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

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the hid class parameters.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_report_descriptor;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;

    /* Initilize the device hid class. The class is connected with interface 2 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,2, (VOID *)&hid_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                            status;
UX_SLAVE_CLASS_COMMAND          command;
UX_SLAVE_CLASS                  class;
UX_SLAVE_CLASS_HID_PARAMETER    hid_parameter;
// UX_MEMORY_BLOCK                *dummy_memory_block_first = (UX_MEMORY_BLOCK *)dummy_usbx_memory;
// UX_MEMORY_BLOCK                *original_regular_memory_block = (UX_MEMORY_BLOCK *)_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start;
// UX_MEMORY_BLOCK                *original_cache_safe_memory_block = (UX_MEMORY_BLOCK *)_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_start;
UCHAR                           class_thread_stack[UX_THREAD_STACK_SIZE];
UX_SLAVE_CLASS_HID             *hid_instance;
TX_EVENT_FLAGS_GROUP            hid_instance_event_flags_copy;


    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Get the HID client */
    hid_client = hid -> ux_host_class_hid_client;

    /* Check if the instance of the keyboard is live */
    while (hid_client -> ux_host_class_hid_client_local_instance == UX_NULL)
        tx_thread_sleep(10);

    command.ux_slave_class_command_parameter = &hid_parameter;
    command.ux_slave_class_command_class_ptr = &class;

    /* Allocate a hid instance like hid_initialize(). */
    hid_instance =  _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_SLAVE_CLASS_HID));
    if (hid_instance == UX_NULL)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: status =  _ux_utility_event_flags_create(&hid -> ux_device_class_hid_event_flags_group, "ux_device_class_hid_event_flag"); fails **/
    /**************************************************/

    /* Create the event_flags like hid_initialize(). */
    status =  ux_utility_event_flags_create(&hid_instance -> ux_device_class_hid_event_flags_group, "ux_host_class_hid_keyboard_event_flags");
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Make a copy of the event_flags. */
    hid_instance_event_flags_copy = hid_instance -> ux_device_class_hid_event_flags_group;

    /* Free the memory so hid_initialize() uses the same memory as us, which will cause threadx to detect a event_flags duplicate. */
    ux_utility_memory_free(hid_instance);

    status = _ux_device_class_hid_initialize(&command);
    if (status != UX_EVENT_ERROR)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* ux_utility_memory_allocate() zero'd out our hid instance event_flags! Good thing we made a copy! */
    hid_instance -> ux_device_class_hid_event_flags_group = hid_instance_event_flags_copy;

    /** Restore state for next test. **/

    status = ux_utility_event_flags_delete(&hid_instance -> ux_device_class_hid_event_flags_group);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: status =  _ux_utility_thread_create(&class -> ux_slave_class_thread, "ux_slave_class_thread",
                _ux_device_class_hid_interrupt_thread,
                (ULONG) class, (VOID *) class -> ux_slave_class_thread_stack,
                UX_THREAD_STACK_SIZE, UX_THREAD_PRIORITY_CLASS,
                UX_THREAD_PRIORITY_CLASS, UX_NO_TIME_SLICE, TX_DONT_START); fails **/
    /**************************************************/

    /* Create the thread like hid_initialize(). */
    status =  _ux_utility_thread_create(&class.ux_slave_class_thread, "ux_slave_class_thread",
                _ux_device_class_hid_interrupt_thread,
                (ULONG) (ALIGN_TYPE) &class, class_thread_stack,
                UX_THREAD_STACK_SIZE, UX_THREAD_PRIORITY_CLASS,
                UX_THREAD_PRIORITY_CLASS, UX_NO_TIME_SLICE, TX_DONT_START);
    UX_THREAD_EXTENSION_PTR_SET(&class.ux_slave_class_thread, &class)
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    status = _ux_device_class_hid_initialize(&command);
    if (status != UX_THREAD_ERROR)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    status = ux_utility_thread_delete(&class.ux_slave_class_thread);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
#if 0 /* Tested by basic memory tests */
    /**************************************************/
    /** Test case: hid =  _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_SLAVE_CLASS_HID)); fails **/
    /**************************************************/

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = 0;
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_device_class_hid_initialize(&command);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: class -> ux_slave_class_thread_stack =
            _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, UX_THREAD_STACK_SIZE); fails **/
    /**************************************************/

    command.ux_slave_class_command_class_ptr = &class;

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = calculate_final_memory_request_size(1, sizeof(UX_SLAVE_CLASS_HID));
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_device_class_hid_initialize(&command);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
#endif
#if 0 /* @BUG_FIX_PENDING: returns UX_EVENT_ERROR when it should return UX_MEMORY_INSUFFICIENT */
    /**************************************************/
    /** Test case: class -> ux_slave_class_thread_stack =
            _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, UX_THREAD_STACK_SIZE); fails **/
    /**************************************************/

    command.ux_slave_class_command_parameter = &hid_parameter;
    command.ux_slave_class_command_class_ptr = &class;

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = calculate_final_memory_request_size(2, sizeof(UX_SLAVE_CLASS_HID), UX_THREAD_STACK_SIZE);
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_device_class_hid_initialize(&command);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;

    status = ux_utility_thread_delete(&class.ux_slave_class_thread);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
#endif

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
