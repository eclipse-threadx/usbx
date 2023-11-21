/* This file tests basic HID functionalities.  */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"

#define DUMMY_USBX_MEMORY_SIZE (64*1024)
static UCHAR dummy_usbx_memory[DUMMY_USBX_MEMORY_SIZE];

static UCHAR hid_keyboard_report[] = {

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
#define HID_KEYBOARD_REPORT_LENGTH sizeof(hid_keyboard_report)/sizeof(hid_keyboard_report[0])

static UCHAR hid_mouse_report[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0x09, 0x38,                    //     USAGE (Mouse Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};
#define HID_MOUSE_REPORT_LENGTH (sizeof(hid_mouse_report)/sizeof(hid_mouse_report[0]))

#define hid_report_descriptor hid_mouse_report
#define HID_REPORT_LENGTH HID_MOUSE_REPORT_LENGTH

/* Configuration descriptor 9 bytes */
#define CFG_DESC(wTotalLength, bNumInterfaces, bConfigurationValue)\
    /* Configuration 1 descriptor 9 bytes */\
    0x09, 0x02, LSB(wTotalLength), MSB(wTotalLength),\
    (bNumInterfaces), (bConfigurationValue), 0x00,\
    0x40, 0x00,
#define CFG_DESC_LEN (9)


/* HID Mouse/Keyboard interface descriptors 9+9+7=25 bytes */
#define HID_IFC_DESC_ALL(ifc, report_len, interrupt_epa) \
    /* Interface descriptor */\
    0x09, 0x04, (ifc), 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,\
    /* HID descriptor */\
    0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(report_len),\
        MSB(report_len),\
    /* Endpoint descriptor (Interrupt) */\
    0x07, 0x05, (interrupt_epa), 0x03, 0x08, 0x00, 0x08,
#define HID_IFC_DESC_ALL_LEN (9+9+7)

static UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
    0x81, 0x0A, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01,

    CFG_DESC(CFG_DESC_LEN+1*HID_IFC_DESC_ALL_LEN, 1, 1)
    /* Keyboard */
    HID_IFC_DESC_ALL(0, HID_REPORT_LENGTH, 0x81)
};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)

static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
    0x03, 0x01,

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x01, 0x00,

    CFG_DESC(CFG_DESC_LEN+1*HID_IFC_DESC_ALL_LEN, 1, 1)
    /* Keyboard */
    HID_IFC_DESC_ALL(0, HID_REPORT_LENGTH, 0x81)
};
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)


/* String Device Framework :
    Byte 0 and 1 : Word containing the language ID : 0x0904 for US
    Byte 2       : Byte containing the index of the descriptor
    Byte 3       : Byte containing the length of the descriptor string
*/
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
#define STRING_FRAMEWORK_LENGTH sizeof(string_framework)


/* Multiple languages are supported on the device, to add
    a language besides english, the unicode language code must
    be appended to the language_id_framework array and the length
    adjusted accordingly. */
static UCHAR language_id_framework[] = {

    /* English. */
    0x09, 0x04
};
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(language_id_framework)


UINT  _ux_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter);


static UINT ux_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{
    switch(event)
    {
    case UX_HID_CLIENT_INSERTION:
        break;
    case UX_HID_CLIENT_REMOVAL:
        break;
#if defined(UX_HOST_STANDALONE)
    case UX_STANDALONE_WAIT_BACKGROUND_TASK:
        /* Let other threads to run.  */
        tx_thread_relinquish();
        break;
#endif
    default:
        break;
    }
    return 0;
}

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_class_hid_mouse_basic_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Inform user.  */
    printf("Running HID Class Mouse Basic Test............................... ");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(ux_system_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_mouse_name, ux_host_class_hid_mouse_entry);
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

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the hid class parameters.  */
    ux_utility_memory_set(&hid_parameter, 0, sizeof(hid_parameter));
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_report_descriptor;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_set_callback;
    hid_parameter.ux_device_class_hid_parameter_get_callback   = demo_thread_hid_get_callback;

    hid_parameter.ux_slave_class_hid_instance_activate = demo_device_hid_instance_activate;
    hid_parameter.ux_slave_class_hid_instance_deactivate = demo_device_hid_instance_deactivate;

    /* Initialize the device hid class. The class is connected with interface 2 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                             1, 0, (VOID *)&hid_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main device simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_device_simulation, "tx demo device simulation", tx_demo_thread_device_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }
    stack_pointer += UX_DEMO_STACK_SIZE;

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }
}

static void  tx_demo_thread_device_simulation_entry(ULONG arg)
{
    while(1)
    {
#if defined(UX_DEVICE_STANDALONE)
        ux_system_tasks_run();
#else
        tx_thread_suspend(&tx_demo_thread_device_simulation);
#endif
    }
}

static UINT demo_class_hid_wait(ULONG tick)
{
    return(ux_test_sleep_break_on_success(tick, demo_class_hid_mouse_get));
}

static UINT _wait_mouse_change(UX_HOST_CLASS_HID_MOUSE *mouse,
    ULONG *buttons, SLONG *x, SLONG *y, SLONG *w)
{
UINT        status;
UINT        i;
ULONG       buttons_bak = *buttons;
SLONG       x_bak = *x, y_bak = *y;
SLONG       w_bak = *w;

    for (i = 0; i < 200; i ++)
    {
#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
#endif
        ux_host_class_hid_mouse_buttons_get(hid_mouse, buttons);
        ux_host_class_hid_mouse_position_get(hid_mouse, x, y);
        ux_host_class_hid_mouse_wheel_get(hid_mouse, w);
        if (buttons_bak != *buttons ||
            x_bak != *x ||
            y_bak != *y ||
            w_bak != *w)
        {
            return(UX_SUCCESS);
        }
        tx_thread_sleep(1);
    }
    return(UX_ERROR);
}

static void test_hid_mouse_events(VOID)
{
UINT                            status;
ULONG                           buttons = 0;
SLONG                           x = 0xFF, y = 0xFF;
SLONG                           wheel = 0;

    /* Initialize mouse event.  */
    device_hid_event.ux_device_class_hid_event_length = 4;
    device_hid_event.ux_device_class_hid_event_buffer[0] = 0; /* ...M|R|L  */
    device_hid_event.ux_device_class_hid_event_buffer[1] = 0; /* X         */
    device_hid_event.ux_device_class_hid_event_buffer[2] = 0; /* Y         */
    device_hid_event.ux_device_class_hid_event_buffer[3] = 0; /* Wheel     */

    status  = ux_host_class_hid_mouse_buttons_get(hid_mouse, &buttons);
    status |= ux_host_class_hid_mouse_position_get(hid_mouse, &x, &y);
    status |= ux_host_class_hid_mouse_wheel_get(hid_mouse, &wheel);
    UX_ASSERT(status == UX_SUCCESS);
    UX_ASSERT(buttons == 0);
    UX_ASSERT(x == 0);
    UX_ASSERT(y == 0);
    UX_ASSERT(wheel == 0);

    /* Set L click.  */
    device_hid_event.ux_device_class_hid_event_buffer[0] = 1; /* ...M|R|L  */
    _ux_device_class_hid_event_set(device_hid, &device_hid_event);
    status = _wait_mouse_change(hid_mouse, &buttons, &x, &y, &wheel);
    UX_ASSERT(status == UX_SUCCESS);
    UX_ASSERT(buttons == 1);
    UX_ASSERT(x == 0);
    UX_ASSERT(y == 0);
    UX_ASSERT(wheel == 0);

    /* Set R click.  */
    device_hid_event.ux_device_class_hid_event_buffer[0] = 3; /* ...M|R|L  */
    _ux_device_class_hid_event_set(device_hid, &device_hid_event);
    status = _wait_mouse_change(hid_mouse, &buttons, &x, &y, &wheel);
    UX_ASSERT(status == UX_SUCCESS);
    UX_ASSERT(buttons == 3);
    UX_ASSERT(x == 0);
    UX_ASSERT(y == 0);
    UX_ASSERT(wheel == 0);

    /* Move X.  */
    device_hid_event.ux_device_class_hid_event_buffer[1] = -1; /* X         */
    _ux_device_class_hid_event_set(device_hid, &device_hid_event);
    status = _wait_mouse_change(hid_mouse, &buttons, &x, &y, &wheel);
    UX_ASSERT(status == UX_SUCCESS);
    UX_ASSERT(buttons == 3);
    UX_ASSERT(x == -1);
    UX_ASSERT(y == 0);
    UX_ASSERT(wheel == 0);

    /* Move X.  */
    device_hid_event.ux_device_class_hid_event_buffer[1] = +8; /* X         */
    _ux_device_class_hid_event_set(device_hid, &device_hid_event);
    status = _wait_mouse_change(hid_mouse, &buttons, &x, &y, &wheel);
    UX_ASSERT(status == UX_SUCCESS);
    UX_ASSERT(buttons == 3);
    UX_ASSERT(x == +7);
    UX_ASSERT(y == 0);
    UX_ASSERT(wheel == 0);

    /* Move Y.  */
    device_hid_event.ux_device_class_hid_event_buffer[2] = +8; /* Y         */
    _ux_device_class_hid_event_set(device_hid, &device_hid_event);
    status = _wait_mouse_change(hid_mouse, &buttons, &x, &y, &wheel);
    UX_ASSERT(status == UX_SUCCESS);
    UX_ASSERT(buttons == 3);
    UX_ASSERT(x == 15);
    UX_ASSERT(y == 8);
    UX_ASSERT(wheel == 0);

    /* Move Wheel.  */
    device_hid_event.ux_device_class_hid_event_buffer[1] = -2;  /* X         */
    device_hid_event.ux_device_class_hid_event_buffer[2] = -5;  /* Y         */
    device_hid_event.ux_device_class_hid_event_buffer[3] = +10; /* Wheel     */
    _ux_device_class_hid_event_set(device_hid, &device_hid_event);
    status = _wait_mouse_change(hid_mouse, &buttons, &x, &y, &wheel);
    UX_ASSERT(status == UX_SUCCESS);
    UX_ASSERT(buttons == 3);
    UX_ASSERT(x == 13);
    UX_ASSERT(y == 3);
    UX_ASSERT(wheel == 10);
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT status;

    /* Find the HID class */
    status = demo_class_hid_wait(100);
    UX_TEST_ASSERT(status == UX_SUCCESS);

    test_hid_mouse_events();

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

static UINT    demo_thread_hid_set_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{
    _ux_utility_memory_copy(&device_hid_event, event, sizeof(UX_SLAVE_CLASS_HID_EVENT));
    return(UX_SUCCESS);
}
static UINT    demo_thread_hid_get_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{
    _ux_utility_memory_copy(event, &device_hid_event, sizeof(UX_SLAVE_CLASS_HID_EVENT));
    return(UX_SUCCESS);
}

static void                         demo_device_hid_instance_activate(VOID *inst)
{
    if (device_hid == UX_NULL)
        device_hid = (UX_SLAVE_CLASS_HID *)inst;
}
static void                         demo_device_hid_instance_deactivate(VOID *inst)
{
    if (inst == (VOID *)device_hid)
        device_hid = UX_NULL;
}
