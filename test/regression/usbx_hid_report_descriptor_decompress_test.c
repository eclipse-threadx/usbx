/* This test concentrates on the report decompress api. */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"


static UCHAR                               report_buffer_compressed_original[1024];
static UCHAR                               report_buffer_compressed[1024];
static SLONG                               report_buffer_decompressed[1024];

static UCHAR hid_report_descriptor[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)

                                   /*   First field */
    0x09, 0x01,                    //   USAGE (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

                                   /*   Second field */
    0x19, 0x02,                    //   USAGE_MINIMUM (2)
    0x29, 0x04,                    //   USAGE_MAXIMUM (4)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

                                   /*   Third field */
    0x0a, 0xcd, 0xab,              //   USAGE (0xabcd)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

                                   /*   Fourth field */
    0x19, 0x06,                    //   USAGE_MINIMUM (6)
    0x29, 0x08,                    //   USAGE_MAXIMUM (8)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

                                   /*   Fifth field */
    0x09, 0x01,                    //   USAGE (1)
    0x75, 0x09,                    //   REPORT_SIZE (9)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

                                   /*   Sixth field */
    0x19, 0x0a,                    //   USAGE_MINIMUM (10)
    0x29, 0x0c,                    //   USAGE_MAXIMUM (12)
    0x75, 0x09,                    //   REPORT_SIZE (9)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

                                   /*   Seventh field */
    0x09, 0x0d,                    //   USAGE (13)
    0x75, 0x1f,                    //   REPORT_SIZE (31)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

                                   /*   Eigth field */
    0x09, 0x0e,                    //   USAGE (14)
    0x75, 0x20,                    //   REPORT_SIZE (32)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

                                   /*   bits: 1 + 3 + 8 + 24 + 9 + 27 + 31 + 32 = 135; 17 bytes overall */

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
void    usbx_hid_report_descriptor_decompress_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
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
UX_HOST_CLASS_HID_REPORT_GET_ID     report_get_id;
UX_HOST_CLASS_HID_CLIENT_REPORT     client_report;
UX_HOST_CLASS_HID_REPORT            *hid_report;

    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the first input report descriptor. */
    report_get_id.ux_host_class_hid_report_get_report = UX_NULL;
    report_get_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    status = ux_host_class_hid_report_id_get(hid, &report_get_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    hid_report = report_get_id.ux_host_class_hid_report_get_report;
    if (hid_report -> ux_host_class_hid_report_id != 0)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Fill out the request for the decompress api. */
    client_report.ux_host_class_hid_client_report = hid_report;
    client_report.ux_host_class_hid_client_report_buffer = report_buffer_decompressed;
    client_report.ux_host_class_hid_client_report_length = hid_report -> ux_host_class_hid_report_byte_length;
    client_report.ux_host_class_hid_client_report_actual_length = 0;

    /* Fill out report. */
    report_buffer_compressed_original[0]  = 0x0b; // [0] bits 1-8:      0000 1011
    report_buffer_compressed_original[1]  = 0x23; // [1] bits 9-16:     0010 0011
    report_buffer_compressed_original[2]  = 0x45; // [2] bits 17-24:    0100 0101
    report_buffer_compressed_original[3]  = 0x67; // [3] bits 25-32:    0110 0111
    report_buffer_compressed_original[4]  = 0x89; // [4] bits 33-40:    1000 1001
    report_buffer_compressed_original[5]  = 0xab; // [5] bits 41-48:    1010 1011
    report_buffer_compressed_original[6]  = 0xcd; // [6] bits 49-56:    1100 1101
    report_buffer_compressed_original[7]  = 0xef; // [7] bits 57-64:    1110 1111
    report_buffer_compressed_original[8]  = 0x01; // [8] bits 65-72:    0000 0001
    report_buffer_compressed_original[9]  = 0x23; // [9] bits 73-80:    0010 0011
    report_buffer_compressed_original[10] = 0x45; // [10] bits 81-88:   0100 0101
    report_buffer_compressed_original[11] = 0x67; // [11] bits 89-96:   0110 0111
    report_buffer_compressed_original[12] = 0x89; // [12] bits 97-104:  1000 1001
    report_buffer_compressed_original[13] = 0xab; // [13] bits 105-112: 1010 1011
    report_buffer_compressed_original[14] = 0xcd; // [14] bits 113-120: 1100 1101
    report_buffer_compressed_original[15] = 0xef; // [15] bits 121-128: 1110 1111
    report_buffer_compressed_original[16] = 0x01; // [16] bits 129-136: 0000 0001

    /* Request decompression. */
    status = _ux_host_class_hid_report_decompress(hid, &client_report, report_buffer_compressed_original, 17);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Check output of decompression of report. */
    if (
        /* First field */

        /* bit 1 */
        report_buffer_decompressed[0] != 0x00010001 ||
        report_buffer_decompressed[1] != 0x01 ||

        /* Second field */

        /* bit 2 */
        report_buffer_decompressed[2] != 0x00010002 ||
        report_buffer_decompressed[3] != 0x01 ||

        /* bit 3 */
        report_buffer_decompressed[4] != 0x00010003 ||
        report_buffer_decompressed[5] != 0x00 ||

        /* bit 4 */
        report_buffer_decompressed[6] != 0x00010004 ||
        report_buffer_decompressed[7] != 0x01 ||

        /* Third field */

        /* bits 5-12 */
        report_buffer_decompressed[8] != 0x0001abcd ||
        report_buffer_decompressed[9] != 0x30 ||

        /* Fourth field */

        /* bits 13-20 */
        report_buffer_decompressed[10] != 0x00010006 ||
        report_buffer_decompressed[11] != 0x52 ||

        /* bits 21-28 */
        report_buffer_decompressed[12] != 0x00010007 ||
        report_buffer_decompressed[13] != 0x74 ||

        /* bits 29-36 */
        report_buffer_decompressed[14] != 0x00010008 ||
        report_buffer_decompressed[15] != 0x96 ||

        /* Fifth field */

        /* bits 37-45 */
        report_buffer_decompressed[16] != 0x00010001 ||
        report_buffer_decompressed[17] != 0x0b8 ||

        /* Sixth field */

        /* bits 46-54 - 0 0110 1101 */
        report_buffer_decompressed[18] != 0x0001000a ||
        report_buffer_decompressed[19] != 0x06d ||

        /* bits 55-63 - 1 1011 1111 */
        report_buffer_decompressed[20] != 0x0001000b ||
        report_buffer_decompressed[21] != 0x1bf ||

        /* bits 64-72 - 0 0000 0011 */
        report_buffer_decompressed[22] != 0x0001000c ||
        report_buffer_decompressed[23] != 0x003 ||

        /* Seventh field */

        /* bits 73-103 - 000 1001 0110 0111 0100 0101 0010 0011 */
        report_buffer_decompressed[24] != 0x0001000d ||
        report_buffer_decompressed[25] != 0x09674523 ||

        /* Eigth field */

        /* bits 104-135 - 0000 0011 1101 1111 1001 1011 0101 0111 */
        report_buffer_decompressed[26] != 0x0001000e ||
        report_buffer_decompressed[27] != 0x03df9b57
        )
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

#if 0
    /* Now perform the inverse: compression. */
    status = _ux_host_class_hid_report_compress(hid, &client_report, report_buffer_compressed, 28);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* These should be the same (god help us). */
    if(ux_utility_memory_compare(report_buffer_compressed, report_buffer_compressed_original, 17) != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
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
