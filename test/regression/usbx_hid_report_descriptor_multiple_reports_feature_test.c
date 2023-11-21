/* This test ensures that the parser correctly handles multiple report IDs for feature reports. */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"


static UCHAR                               feature_report_buffer_raw[1024];
static SLONG                               feature_report_buffer_decompressed[1024];

static UCHAR hid_report_descriptor[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x19, 0x01,                    //   USAGE_MINIMUM (1)
    0x29, 0x04,                    //   USAGE_MAXIMUM (4)
    0x75, 0x04,                    //   REPORT_SIZE (4)
    0x95, 0x04,                    //   REPORT_COUNT (4)
    0xb1, 0x02,                    //   FEATURE (Data,Var,Abs)
    0x85, 0x02,                    //   REPORT_ID (2)
    0x19, 0x05,                    //   USAGE_MINIMUM (5)
    0x29, 0x07,                    //   USAGE_MAXIMUM (7)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0xb1, 0x02,                    //   FEATURE (Data,Var,Abs)
    0x85, 0x04,                    //   REPORT_ID (4)
    0x09, 0x08,                    //   USAGE (8)
    0x75, 0x10,                    //   REPORT_SIZE (16)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0xb1, 0x02,                    //   FEATURE (Data,Var,Abs)

    /* USBX requires at least one input report. */
    0x09, 0x01,                    //   USAGE (1)
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

    /* Failed test.  */
    printf("Error on line %d, system_level: %d, system_context: %d, error code: %d\n", __LINE__, system_level, system_context, error_code);
    test_control_return(1);
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_hid_report_descriptor_multiple_reports_feature_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
UINT                            descriptor_size = HID_REPORT_LENGTH;


    /* Inform user.  */
    printf("Running HID Report Descriptor Multiple Reports Feature Test......... ");

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

UINT status;
UX_HOST_CLASS_HID_REPORT_GET_ID     report_id;
UX_HOST_CLASS_HID_CLIENT_REPORT     feature_report_request;
UX_HOST_CLASS_HID_REPORT            *feature_report_descriptor;


    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the first feature report descriptor. */
    report_id.ux_host_class_hid_report_get_report = UX_NULL;
    report_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_FEATURE;
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    feature_report_descriptor = report_id.ux_host_class_hid_report_get_report;
    if (feature_report_descriptor -> ux_host_class_hid_report_id != 0)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Fill out the request for the decompress api. */
    feature_report_request.ux_host_class_hid_client_report = feature_report_descriptor;
    feature_report_request.ux_host_class_hid_client_report_buffer = feature_report_buffer_decompressed;
    feature_report_request.ux_host_class_hid_client_report_length = feature_report_descriptor -> ux_host_class_hid_report_byte_length;
    feature_report_request.ux_host_class_hid_client_report_actual_length = 0;
    feature_report_request.ux_host_class_hid_client_report_flags = UX_HOST_CLASS_HID_REPORT_DECOMPRESSED;

    /* Fill out first report. */
    feature_report_buffer_raw[0] = 0x01;
    feature_report_buffer_raw[1] = 0x23;

    /* Request decompression. */
    status = _ux_host_class_hid_report_decompress(hid, &feature_report_request, feature_report_buffer_raw, 2);

    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Check output of decompression of first report. */
    if (
        feature_report_buffer_decompressed[0] != 0x00010001 ||
        feature_report_buffer_decompressed[1] != 0x01 ||
        feature_report_buffer_decompressed[2] != 0x00010002 ||
        feature_report_buffer_decompressed[3] != 0x00 ||
        feature_report_buffer_decompressed[4] != 0x00010003 ||
        feature_report_buffer_decompressed[5] != 0x03 ||
        feature_report_buffer_decompressed[6] != 0x00010004 ||
        feature_report_buffer_decompressed[7] != 0x02
        )
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the second feature report descriptor. */
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    feature_report_descriptor = report_id.ux_host_class_hid_report_get_report;
    if (feature_report_descriptor->ux_host_class_hid_report_id != 2)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Fill out the request for the decompress api. */
    feature_report_request.ux_host_class_hid_client_report = feature_report_descriptor;
    feature_report_request.ux_host_class_hid_client_report_buffer = feature_report_buffer_decompressed;
    feature_report_request.ux_host_class_hid_client_report_length = feature_report_descriptor -> ux_host_class_hid_report_byte_length;
    feature_report_request.ux_host_class_hid_client_report_actual_length = 0;
    feature_report_request.ux_host_class_hid_client_report_flags = UX_HOST_CLASS_HID_REPORT_DECOMPRESSED;

    /* Fill out second report. */
    feature_report_buffer_raw[0] = 0x02; // First byte is the Report ID.
    feature_report_buffer_raw[1] = 0x45;
    feature_report_buffer_raw[2] = 0x67;
    feature_report_buffer_raw[3] = 0x89;

    /* Request decompression. */
    status = _ux_host_class_hid_report_decompress(hid, &feature_report_request, feature_report_buffer_raw, 3);

    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Check output of decompression of second report. */
    if (
        feature_report_buffer_decompressed[0] != 0x00010005 ||
        feature_report_buffer_decompressed[1] != 0x45 ||
        feature_report_buffer_decompressed[2] != 0x00010006 ||
        feature_report_buffer_decompressed[3] != 0x67 ||
        feature_report_buffer_decompressed[4] != 0x00010007 ||
        feature_report_buffer_decompressed[5] != 0x89
        )
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the third feature report descriptor. */
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    feature_report_descriptor = report_id.ux_host_class_hid_report_get_report;
    if (feature_report_descriptor->ux_host_class_hid_report_id != 4)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Fill out the request for the decompress api. */
    feature_report_request.ux_host_class_hid_client_report = feature_report_descriptor;
    feature_report_request.ux_host_class_hid_client_report_buffer = feature_report_buffer_decompressed;
    feature_report_request.ux_host_class_hid_client_report_length = feature_report_descriptor -> ux_host_class_hid_report_byte_length;
    feature_report_request.ux_host_class_hid_client_report_actual_length = 0;
    feature_report_request.ux_host_class_hid_client_report_flags = UX_HOST_CLASS_HID_REPORT_DECOMPRESSED;

    /* Fill out third report. */
    feature_report_buffer_raw[0] = 0x04; // First byte is the Report ID.
    feature_report_buffer_raw[1] = 0xab;
    feature_report_buffer_raw[2] = 0xcd;

    /* Request decompression. */
    status = _ux_host_class_hid_report_decompress(hid, &feature_report_request, feature_report_buffer_raw, 2);

    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Check output of decompression of third report. */
    if (
        feature_report_buffer_decompressed[0] != 0x00010008 ||
        feature_report_buffer_decompressed[1] != 0xcdab
        )
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
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
