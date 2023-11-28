/* This test ensures that the parser correctly handles pushing and popping. */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"


static UCHAR hid_report_descriptor[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
                                   /*   Set the global state to A */
    0x85,    1,                    //   REPORT_ID (1)
    0x05,    2,                    //   USAGE_PAGE (2)
    0x15,    3,                    //   LOGICAL_MINIMUM (3)
    0x25,    4,                    //   LOGICAL_MAXIMUM (4)
    0x35,    5,                    //   PHYSICAL_MINIMUM (5)
    0x45,    6,                    //   PHYSICAL_MAXIMUM (6)
    0x65,    7,                    //   UNIT (7)
    0x55,    8,                    //   UNIT_EXPONENT (8)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x02,                    //   REPORT_SIZE (2)
    0xa4,                          //   PUSH
                                   /*   Set the global state to B */
    0x85,   11,                    //   REPORT_ID (11)
    0x05,   12,                    //   USAGE_PAGE (12)
    0x15,   15,                    //   LOGICAL_MINIMUM (15)
    0x25,   16,                    //   LOGICAL_MAXIMUM (16)
    0x35,   17,                    //   PHYSICAL_MINIMUM (17)
    0x45,   18,                    //   PHYSICAL_MAXIMUM (18)
    0x65,   19,                    //   UNIT (19)
    0x55,   20,                    //   UNIT_EXPONENT (20)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0x75, 0x04,                    //   REPORT_SIZE (4)
    0xa4,                          //   PUSH
                                   /*   Set the global state to C */
    0x85,   23,                    //   REPORT_ID (23)
    0x05,   24,                    //   USAGE_PAGE (24)
    0x15,   27,                    //   LOGICAL_MINIMUM (27)
    0x25,   28,                    //   LOGICAL_MAXIMUM (28)
    0x35,   29,                    //   PHYSICAL_MINIMUM (29)
    0x45,   30,                    //   PHYSICAL_MAXIMUM (30)
    0x65,   31,                    //   UNIT (31)
    0x55,   32,                    //   UNIT_EXPONENT (32)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x06,                    //   REPORT_SIZE (6)
    0xa4,                          //   PUSH
                                   /*   Set the global state to D */
    0x85,   35,                    //   REPORT_ID (35)
    0x05,   36,                    //   USAGE_PAGE (36)
    0x15,   39,                    //   LOGICAL_MINIMUM (39)
    0x25,   40,                    //   LOGICAL_MAXIMUM (40)
    0x35,   41,                    //   PHYSICAL_MINIMUM (41)
    0x45,   42,                    //   PHYSICAL_MAXIMUM (42)
    0x65,   43,                    //   UNIT (43)
    0x55,   44,                    //   UNIT_EXPONENT (44)
    0x95, 0x07,                    //   REPORT_COUNT (7)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0xa4,                          //   PUSH
                                   /*   Set the global state to E */
    0x85,   47,                    //   REPORT_ID (47)
    0x05,   48,                    //   USAGE_PAGE (48)
    0x15,   51,                    //   LOGICAL_MINIMUM (51)
    0x25,   52,                    //   LOGICAL_MAXIMUM (52)
    0x35,   53,                    //   PHYSICAL_MINIMUM (53)
    0x45,   54,                    //   PHYSICAL_MAXIMUM (54)
    0x65,   55,                    //   UNIT (55)
    0x55,   56,                    //   UNIT_EXPONENT (56)
    0x95, 0x09,                    //   REPORT_COUNT (9)
    0x75, 0x0a,                    //   REPORT_SIZE (10)
                                   /*   Pop state D */
    0xb4,                          //   POP
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
                                   /*   Pop state C */
    0xb4,                          //   POP
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
                                   /*   Pop state B */
    0xb4,                          //   POP
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
                                   /*   Pop state A */
    0xb4,                          //   POP
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)

    /* USBX expects keyboards to have at least one output report, otherwise it's an error. */
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)

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

    /* Failed test.  */
    printf("Error on line %d, system_level: %d, system_context: %d, error code: %d\n", __LINE__, system_level, system_context, error_code);
    test_control_return(1);
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_hid_report_descriptor_push_pop_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
UINT                            descriptor_size = HID_REPORT_LENGTH;


    /* Inform user.  */
    printf("Running HID Report Descriptor Push Pop Test......................... ");

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
UX_HOST_CLASS_HID_REPORT            *input_report_descriptor;
UX_HOST_CLASS_HID_FIELD             *report_field;


    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the first input report descriptor (state D). */
    report_id.ux_host_class_hid_report_get_report = UX_NULL;
    report_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;

    /* Check the field in the report. */
    report_field = report_id.ux_host_class_hid_report_get_report -> ux_host_class_hid_report_field;
    if(
       report_field -> ux_host_class_hid_field_logical_min != 39 ||
       report_field -> ux_host_class_hid_field_logical_max != 40 ||
       report_field -> ux_host_class_hid_field_physical_min != 41 ||
       report_field -> ux_host_class_hid_field_physical_max != 42 ||
       report_field -> ux_host_class_hid_field_unit != 43 ||
       report_field -> ux_host_class_hid_field_unit_expo != 44 ||
       report_field -> ux_host_class_hid_field_report_id != 35 ||
       report_field -> ux_host_class_hid_field_report_count != 7 ||
       report_field -> ux_host_class_hid_field_report_size != 8
       )
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the second input report descriptor (state C). */
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;

    /* Check the field in the report. */
    report_field = report_id.ux_host_class_hid_report_get_report -> ux_host_class_hid_report_field;
    if(
       report_field -> ux_host_class_hid_field_logical_min != 27 ||
       report_field -> ux_host_class_hid_field_logical_max != 28 ||
       report_field -> ux_host_class_hid_field_physical_min != 29 ||
       report_field -> ux_host_class_hid_field_physical_max != 30 ||
       report_field -> ux_host_class_hid_field_unit != 31 ||
       report_field -> ux_host_class_hid_field_unit_expo != 32 ||
       report_field -> ux_host_class_hid_field_report_id != 23 ||
       report_field -> ux_host_class_hid_field_report_count != 5 ||
       report_field -> ux_host_class_hid_field_report_size != 6
       )
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the third input report descriptor (state B). */
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;

    /* Check the field in the report. */
    report_field = report_id.ux_host_class_hid_report_get_report -> ux_host_class_hid_report_field;
    if(
       report_field -> ux_host_class_hid_field_logical_min != 15 ||
       report_field -> ux_host_class_hid_field_logical_max != 16 ||
       report_field -> ux_host_class_hid_field_physical_min != 17 ||
       report_field -> ux_host_class_hid_field_physical_max != 18 ||
       report_field -> ux_host_class_hid_field_unit != 19 ||
       report_field -> ux_host_class_hid_field_unit_expo != 20 ||
       report_field -> ux_host_class_hid_field_report_id != 11 ||
       report_field -> ux_host_class_hid_field_report_count != 3 ||
       report_field -> ux_host_class_hid_field_report_size != 4
       )
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the fourth input report descriptor (state A). */
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;

    /* Check the field in the report. */
    report_field = report_id.ux_host_class_hid_report_get_report -> ux_host_class_hid_report_field;
    if(
       report_field -> ux_host_class_hid_field_logical_min != 3 ||
       report_field -> ux_host_class_hid_field_logical_max != 4 ||
       report_field -> ux_host_class_hid_field_physical_min != 5 ||
       report_field -> ux_host_class_hid_field_physical_max != 6 ||
       report_field -> ux_host_class_hid_field_unit != 7 ||
       report_field -> ux_host_class_hid_field_unit_expo != 8 ||
       report_field -> ux_host_class_hid_field_report_id != 1 ||
       report_field -> ux_host_class_hid_field_report_count != 1 ||
       report_field -> ux_host_class_hid_field_report_size != 2
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
