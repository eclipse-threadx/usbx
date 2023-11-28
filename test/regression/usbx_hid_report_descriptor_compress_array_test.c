/* This test concentrates on the report compress api. */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"
#include "ux_test.h"


static UCHAR                               input_report_buffer_compressed[1024];
static SLONG                               input_report_buffer_decompressed_original[1024];

static UCHAR hid_report_descriptor[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
                                        /* Keys. */
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x01,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x01,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
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


static UCHAR ignore_errors;
static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    if (!ignore_errors)
    {

        /* Failed test.  */
        printf("Error on line %d, system_level: %d, system_context: %d, error code: %d\n", __LINE__, system_level, system_context, error_code);
        test_control_return(1);
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_hid_report_descriptor_compress_array_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
UINT                            descriptor_size = HID_REPORT_LENGTH;

    /* Inform user.  */
    printf("Running HID Report Descriptor Decompress Test....................... ");

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
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

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


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                status;
UX_HOST_CLASS_HID_REPORT_GET_ID     report_id;
UX_HOST_CLASS_HID_CLIENT_REPORT     client_report;
UX_HOST_CLASS_HID_REPORT            *input_report_descriptor;
SLONG                               report_buffer_decompressed[1024];
UCHAR                               report_buffer_compressed[1024] = {0};


    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /** Do basic test.  **/

    /* Get the first input report descriptor. */
    report_id.ux_host_class_hid_report_get_report = UX_NULL;
    report_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    UX_TEST_CHECK_SUCCESS(ux_host_class_hid_report_id_get(hid, &report_id));
    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;

    /* 1 */
    report_buffer_decompressed[0] = 0x00070001;
    report_buffer_decompressed[1] = 0x1;

    /* 2 */
    report_buffer_decompressed[2] = 0x00070002;
    report_buffer_decompressed[3] = 0x2;

    /* 3 */
    report_buffer_decompressed[4] = 0x00070003;
    report_buffer_decompressed[5] = 0x3;

    /* 4 */
    report_buffer_decompressed[6] = 0x00070004;
    report_buffer_decompressed[7] = 0x4;

    /* 5 */
    report_buffer_decompressed[8] = 0x00070005;
    report_buffer_decompressed[9] = 0x5;

    /* 6 */
    report_buffer_decompressed[10] = 0x00070006;
    report_buffer_decompressed[11] = 0x6;

    /* Fill out the request for the compress api. */
    client_report.ux_host_class_hid_client_report = input_report_descriptor;
    client_report.ux_host_class_hid_client_report_buffer = report_buffer_decompressed;
    client_report.ux_host_class_hid_client_report_actual_length = 0;

    /* Request compression. */
    UX_TEST_CHECK_SUCCESS(_ux_host_class_hid_report_compress(hid, &client_report, report_buffer_compressed, sizeof(report_buffer_compressed)));

    /* Check it. */
    UX_TEST_ASSERT(report_buffer_compressed[0] == 0x1);
    UX_TEST_ASSERT(report_buffer_compressed[1] == 0x2);
    UX_TEST_ASSERT(report_buffer_compressed[2] == 0x3);
    UX_TEST_ASSERT(report_buffer_compressed[3] == 0x4);
    UX_TEST_ASSERT(report_buffer_compressed[4] == 0x5);
    UX_TEST_ASSERT(report_buffer_compressed[5] == 0x6);

    /** Test invalid usage page. **/

    /* Get the first input report descriptor. */
    report_id.ux_host_class_hid_report_get_report = UX_NULL;
    report_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    UX_TEST_CHECK_SUCCESS(ux_host_class_hid_report_id_get(hid, &report_id));
    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;

    /* Add invalid usage page. */
    report_buffer_decompressed[0] = 0x00000000;
    report_buffer_decompressed[1] = 0x00;

    /* Fill out the request for the compress api. */
    client_report.ux_host_class_hid_client_report = input_report_descriptor;
    client_report.ux_host_class_hid_client_report_buffer = report_buffer_decompressed;
    client_report.ux_host_class_hid_client_report_actual_length = 0;

    ignore_errors = 1;

    /* Request compression. This should fail. */
    UX_TEST_CHECK_NOT_SUCCESS(_ux_host_class_hid_report_compress(hid, &client_report, report_buffer_compressed, sizeof(report_buffer_compressed)));

    ignore_errors = 0;

    /** Test usage too large. **/

    /* Get the first input report descriptor. */
    report_id.ux_host_class_hid_report_get_report = UX_NULL;
    report_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    UX_TEST_CHECK_SUCCESS(ux_host_class_hid_report_id_get(hid, &report_id));
    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;

    /* Add unknown usage. */
    report_buffer_decompressed[0] = 0x00070066;
    report_buffer_decompressed[1] = 0x66;

    /* Fill out the request for the compress api. */
    client_report.ux_host_class_hid_client_report = input_report_descriptor;
    client_report.ux_host_class_hid_client_report_buffer = report_buffer_decompressed;
    client_report.ux_host_class_hid_client_report_actual_length = 0;

    ignore_errors = 1;

    /* Request compression. This should fail. */
    UX_TEST_CHECK_NOT_SUCCESS(_ux_host_class_hid_report_compress(hid, &client_report, report_buffer_compressed, sizeof(report_buffer_compressed)));

    ignore_errors = 0;

    /** Test usage too small. **/

    /* Get the first input report descriptor. */
    report_id.ux_host_class_hid_report_get_report = UX_NULL;
    report_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    UX_TEST_CHECK_SUCCESS(ux_host_class_hid_report_id_get(hid, &report_id));
    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;

    /* Add unknown usage. */
    report_buffer_decompressed[0] = 0x00070000;
    report_buffer_decompressed[1] = 0x0;

    /* Fill out the request for the compress api. */
    client_report.ux_host_class_hid_client_report = input_report_descriptor;
    client_report.ux_host_class_hid_client_report_buffer = report_buffer_decompressed;
    client_report.ux_host_class_hid_client_report_actual_length = 0;

    ignore_errors = 1;

    /* Request compression. This should fail. */
    UX_TEST_CHECK_NOT_SUCCESS(_ux_host_class_hid_report_compress(hid, &client_report, report_buffer_compressed, sizeof(report_buffer_compressed)));

    ignore_errors = 0;

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
