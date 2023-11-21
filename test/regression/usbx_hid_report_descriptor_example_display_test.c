/* This test ensures that the parser handles the example display report descriptor
   from the USB HID report descriptor tool. */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"


static UCHAR hid_report_descriptor[] = {

    0x05, 0x14,                    // USAGE_PAGE (Alphnumeric Display)
    0x09, 0x01,                    // USAGE (Alphanumeric Display)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0xa1, 0x02,                    // COLLECTION (Logical)
    0x09, 0x20,                    //   USAGE (Display Attributes Report)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x09, 0x35,                    //     USAGE (Rows)
    0x09, 0x36,                    //     USAGE (Columns)
    0x09, 0x3d,                    //     USAGE (Character Width)
    0x09, 0x3e,                    //     USAGE (Character Height)
    0x85, 0x01,                    //     REPORT_ID (1)
    0x25, 0x1f,                    //     LOGICAL_MAXIMUM (31)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x09, 0x21,                    //     USAGE (ASCII Character Set)
    0x09, 0x22,                    //     USAGE (Data Read Back)
    0x09, 0x29,                    //     USAGE (Vertical Scroll)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x25, 0x02,                    //   LOGICAL_MAXIMUM (2)
    0x09, 0x2d,                    //   USAGE (Display Status)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x09, 0x2e,                    //     USAGE (Stat Not Ready)
    0x09, 0x2f,                    //     USAGE (Stat Ready)
    0x09, 0x30,                    //     USAGE (Err Not a loadable character)
    0x81, 0x40,                    //     INPUT (Data,Ary,Abs,Null)
    0xc0,                          //   END_COLLECTION
    0x09, 0x32,                    //   USAGE (Cursor Position Report)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x85, 0x02,                    //     REPORT_ID (2)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x25, 0x0f,                    //     LOGICAL_MAXIMUM (15)
    0x09, 0x34,                    //     USAGE (Column)
    0xb1, 0x22,                    //     FEATURE (Data,Var,Abs,NPrf)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x09, 0x33,                    //     USAGE (Row)
    0xb1, 0x22,                    //     FEATURE (Data,Var,Abs,NPrf)
    0xc0,                          //   END_COLLECTION
    0x09, 0x2b,                    //   USAGE (Character Report)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x85, 0x03,                    //     REPORT_ID (3)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x25, 0x7e,                    //     LOGICAL_MAXIMUM (126)
    0x09, 0x2c,                    //     USAGE (Display Data)
    0xb2, 0x02, 0x01,              //     FEATURE (Data,Var,Abs,Buf)
    0xc0,                          //   END_COLLECTION
    0x85, 0x04,                    //   REPORT_ID (4)
    0x09, 0x3b,                    //   USAGE (Font Report)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x7e,                    //     LOGICAL_MAXIMUM (126)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x09, 0x2c,                    //     USAGE (Display Data)
    0x91, 0x02,                    //     OUTPUT (Data,Var,Abs)
    0x95, 0x05,                    //     REPORT_COUNT (5)
    0x09, 0x3c,                    //     USAGE (Font Data)
    0x92, 0x02, 0x01,              //     OUTPUT (Data,Var,Abs,Buf)
    0xc0,                          //   END_COLLECTION
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


static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    /* USBX currently doesn't support this device. However, USBX parses the report descriptor before searching for clients, so this isn't a waste of time. */
    if (error_code != UX_HOST_CLASS_HID_UNKNOWN)
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
void    usbx_hid_report_descriptor_example_display_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Inform user.  */
    printf("Running HID Report Descriptor Example display Test.................. ");

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


    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the input report descriptor. */
    report_id.ux_host_class_hid_report_get_report = UX_NULL;
    report_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Do minimal error-checking. */
    if (report_id.ux_host_class_hid_report_get_report == UX_NULL)
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
