/* This test concentrates on the ux_host_class_hid_report_set API. */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"

#include "ux_test_hcd_sim_host.h"

UCHAR           buffer[64];
UINT            test_number;
UCHAR           buffer_device[64];
TX_SEMAPHORE    test_semaphore;

#define DUMMY_USBX_MEMORY_SIZE (64*1024)
static UCHAR dummy_usbx_memory[DUMMY_USBX_MEMORY_SIZE];

#define NUM_TESTS 5
UCHAR test_buffers[NUM_TESTS][5] = {
    {0xab, 0xcd, 0xef, 0x01, 0x23},
    {0x23, 0xab, 0xcd, 0xef, 0x01},
    {0x01, 0x23, 0xab, 0xcd, 0xef},
    {0xef, 0x01, 0x23, 0xab, 0xcd},
    {0xcd, 0xef, 0x01, 0x23, 0xab},
};

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

    0x85, 0x01,                    //   REPORT_ID (1)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)

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

static UX_TEST_HCD_SIM_ACTION zlp_on_transfer_0[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER, 0, UX_NULL, 0, UX_SUCCESS,
        UX_SUCCESS},
{   0   }
};

UINT  _ux_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter);

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_class_hid_report_set_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
UINT                            descriptor_size = HID_REPORT_LENGTH;


    /* Inform user.  */
    printf("Running ux_host_class_hid_report_set Test........................... ");

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
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name, ux_host_class_hid_keyboard_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
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
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

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
}

static UINT demo_thread_hid_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{

    if (event -> ux_device_class_hid_event_report_type == UX_DEVICE_CLASS_HID_REPORT_TYPE_OUTPUT)
    {

        if (test_number > 0)
        {

            /* Ensure report id is correct. */
            if (event -> ux_device_class_hid_event_buffer[0] != 0x01)
            {

                printf("Error on line %d: test number %d, report id %d\n", __LINE__, test_number, event -> ux_device_class_hid_event_buffer[0]);
                test_control_return(1);
            }

            if (ux_utility_memory_compare(test_buffers[test_number - 1], event->ux_device_class_hid_event_buffer + 1, 5) != UX_SUCCESS)
            {

                printf("Error on line %d: test number %d\n -", __LINE__, test_number);
                for (int i = 0; i < 5; i ++)
                    printf(" %2x", test_buffers[test_number - 1][i]);
                printf(" <>\n -");
                for (int i = 0; i < 5; i ++)
                    printf(" %2x", event->ux_device_class_hid_event_buffer[i + 1]);
                printf("\n");
                test_control_return(1);
            }

            ux_utility_semaphore_put(&test_semaphore);
        }
    }

    return(UX_SUCCESS);
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                            status;
UX_HOST_CLASS_HID_CLIENT_REPORT client_report;
UX_HOST_CLASS_HID_REPORT        hid_report;
UX_HOST_CLASS_HID_REPORT       *hid_report_ptr;
ULONG                           tmp;
UX_MEMORY_BLOCK                *dummy_memory_block_first = (UX_MEMORY_BLOCK *)dummy_usbx_memory;
UX_MEMORY_BLOCK                *original_regular_memory_block = (UX_MEMORY_BLOCK *)_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start;
UX_MEMORY_BLOCK                *original_cache_safe_memory_block = (UX_MEMORY_BLOCK *)_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_start;
UCHAR                           buffer[256];


    status =  ux_utility_semaphore_create(&test_semaphore, "test_semaphore", 0);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: status =  _ux_utility_semaphore_get(&hid -> ux_host_class_hid_device -> ux_device_protection_semaphore, UX_WAIT_FOREVER); fails **/
    /**************************************************/

    client_report.ux_host_class_hid_client_report = hid -> ux_host_class_hid_parser.ux_host_class_hid_parser_output_report -> ux_host_class_hid_report_next_report;
    client_report.ux_host_class_hid_client_report_flags = UX_HOST_CLASS_HID_REPORT_RAW;
    client_report.ux_host_class_hid_client_report_length = 5;
    client_report.ux_host_class_hid_client_report_buffer = (ULONG *)buffer;

    /* Make sure it fails. */
    hid -> ux_host_class_hid_device -> ux_device_protection_semaphore.tx_semaphore_id++;

    status = _ux_host_class_hid_report_set(hid, &client_report);
    if(status != TX_SEMAPHORE_ERROR)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state. */
    hid -> ux_host_class_hid_device -> ux_device_protection_semaphore.tx_semaphore_id--;

    /* The function did a get() on the hid semaphore and never put() it since we error'd out, so we must do it! */
    status = ux_utility_semaphore_put(&hid -> ux_host_class_hid_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: status =  _ux_utility_semaphore_get(&hid -> ux_host_class_hid_semaphore, UX_WAIT_FOREVER); fails **/
    /**************************************************/

    client_report.ux_host_class_hid_client_report = hid -> ux_host_class_hid_parser.ux_host_class_hid_parser_output_report -> ux_host_class_hid_report_next_report;
    client_report.ux_host_class_hid_client_report_flags = UX_HOST_CLASS_HID_REPORT_RAW;
    client_report.ux_host_class_hid_client_report_length = 5;

    /* Make sure it fails. */
    hid -> ux_host_class_hid_semaphore.tx_semaphore_id++;

    status = _ux_host_class_hid_report_set(hid, &client_report);
    if(status != TX_SEMAPHORE_ERROR)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state. */
    hid -> ux_host_class_hid_semaphore.tx_semaphore_id--;

    /**************************************************/
    /** Test case: Template for making ux_utility_memory_allocate() fail **/
    /**************************************************/

    hid_report.ux_host_class_hid_report_type = UX_HOST_CLASS_HID_REPORT_TYPE_OUTPUT;
    client_report.ux_host_class_hid_client_report = &hid_report;

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_byte_pool = UX_NULL;
    // dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    // dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    // dummy_memory_block_first -> ux_memory_block_size = 0;
    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start = (UCHAR*)dummy_memory_block_first;
    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_start = (UCHAR*)dummy_memory_block_first;

    status = _ux_host_class_hid_report_set(hid, &client_report);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start = (UCHAR*)original_regular_memory_block;
    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_start = (UCHAR*)original_cache_safe_memory_block;

    /**************************************************/
    /** Test case: _ux_host_stack_class_instance_verify() fails. **/
    /**************************************************/

    /* Make sure verify() doesn't find any classes. */
    // tmp = _ux_system_host -> ux_system_host_max_class;
    // _ux_system_host -> ux_system_host_max_class = 0;
    tmp = _ux_system_host->ux_system_host_class_array->ux_host_class_status;
    _ux_system_host->ux_system_host_class_array->ux_host_class_status = UX_UNUSED;

    if (_ux_host_class_hid_report_set(hid, UX_NULL) != UX_HOST_CLASS_INSTANCE_UNKNOWN)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Restore state for next test. */
    // _ux_system_host -> ux_system_host_max_class = tmp;
    _ux_system_host->ux_system_host_class_array->ux_host_class_status = (UINT)tmp;

    /**************************************************/
    /** Test case: if (hid_report -> ux_host_class_hid_report_type == UX_HOST_CLASS_HID_REPORT_TYPE_INPUT) **/
    /**************************************************/

    /* Set to an input report. */
    hid_report.ux_host_class_hid_report_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;

    client_report.ux_host_class_hid_client_report = &hid_report;

    if (_ux_host_class_hid_report_set(hid, &client_report) != UX_HOST_CLASS_HID_REPORT_ERROR)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: if (hid_report -> ux_host_class_hid_report_byte_length < client_report -> ux_host_class_hid_client_report_length) **/
    /**************************************************/

    /* Set client report length to less than hid report length. */
    hid_report.ux_host_class_hid_report_type = UX_HOST_CLASS_HID_REPORT_TYPE_FEATURE;
    hid_report.ux_host_class_hid_report_byte_length = 1;
    client_report.ux_host_class_hid_client_report_length = hid_report.ux_host_class_hid_report_byte_length + 1;

    client_report.ux_host_class_hid_client_report = &hid_report;
    client_report.ux_host_class_hid_client_report_flags = UX_HOST_CLASS_HID_REPORT_RAW;

    if (_ux_host_class_hid_report_set(hid, &client_report) != UX_HOST_CLASS_HID_REPORT_OVERFLOW)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: call _ux_host_class_hid_report_compress(). **/
    /**************************************************/

    hid_report_ptr = hid -> ux_host_class_hid_parser.ux_host_class_hid_parser_output_report;

    client_report.ux_host_class_hid_client_report = hid_report_ptr;
    client_report.ux_host_class_hid_client_report_length = hid_report_ptr -> ux_host_class_hid_report_byte_length;
    client_report.ux_host_class_hid_client_report_buffer = (ULONG *)buffer;
    client_report.ux_host_class_hid_client_report_flags = UX_HOST_CLASS_HID_REPORT_DECOMPRESSED;

    if (_ux_host_class_hid_report_set(hid, &client_report) != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /**************************************************/
    /** Test case: ux_host_stack_transfer_request() fails. **/
    /**************************************************/

    ux_test_hcd_sim_host_set_actions(error_on_transfer_0);

    if (_ux_host_class_hid_report_set(hid, &client_report) != UX_HOST_CLASS_HID_REPORT_ERROR)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /** Restore state for next test. **/

    /* ux_host_stack_transfer_request() doesn't release the device protection semaphore when it fails, so do it manually. */
    ux_utility_semaphore_put(&hid -> ux_host_class_hid_device -> ux_device_protection_semaphore);

    /**************************************************/
    /** Test case: ux_host_stack_transfer_request() length error. **/
    /**************************************************/

    ux_test_hcd_sim_host_set_actions(zlp_on_transfer_0);

    if (_ux_host_class_hid_report_set(hid, &client_report) != UX_HOST_CLASS_HID_REPORT_ERROR)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /** Restore state for next test. **/

    /* ux_host_stack_transfer_request() doesn't release the device protection semaphore when it fails, so do it manually. */
    ux_utility_semaphore_put(&hid -> ux_host_class_hid_device -> ux_device_protection_semaphore);

#if 0 /* TODO: USBX-246, USBX-247 */
    /**************************************************/
    /** Test case: functionality test. **/
    /**************************************************/

    client_report.ux_host_class_hid_client_report = hid -> ux_host_class_hid_parser.ux_host_class_hid_parser_output_report -> ux_host_class_hid_report_next_report;
    client_report.ux_host_class_hid_client_report_flags = UX_HOST_CLASS_HID_REPORT_RAW;
    client_report.ux_host_class_hid_client_report_length = 5;

    /* Perform tests. */
    for (test_number = 1; test_number <= NUM_TESTS; test_number++)
    {
        client_report.ux_host_class_hid_client_report_buffer = (ULONG *)test_buffers[test_number - 1];

        if (_ux_host_class_hid_report_set(hid, &client_report) != UX_SUCCESS)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        /* Wait for device to receive our request. */
        if (ux_utility_semaphore_get(&test_semaphore, 10) != UX_SUCCESS)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }
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
