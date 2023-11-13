/**************************************************/
/** Test case: The default case of switch (item -> ux_host_class_hid_item_report_tag). **/
/**************************************************/

#include "usbx_test_common_hid.h"


#define DUMMY_USBX_MEMORY_SIZE (64*1024)
static UCHAR dummy_usbx_memory[DUMMY_USBX_MEMORY_SIZE];

static UCHAR hid_report_descriptor[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xA1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xA1, 0x00,                    //   COLLECTION (Physical)

    0x95, 0x10,                    //     REPORT_COUNT (16)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x10,                    //     USAGE_MAXIMUM (Button 16)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)

    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x16, 0x01, 0x80,              //     LOGICAL_MINIMUM (-32767)
    0x26, 0xff, 0x7f,              //     LOGICAL_MAXIMUM (32767)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)

    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x09, 0x38,                    //     USAGE (Wheel)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)

    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x05, 0x0C,                    //     USAGE_PAGE (Consumer Devices)
    0x0a, 0x38, 0x02,              //     USAGE (AC Pan)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
#if 1 /* Additional keyboard for test */
    0x95, 0x06,                    //     REPORT_COUNT (6)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //     LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //     USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //     USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //     USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //     INPUT (Data,Ary,Abs)
#endif
    0xC0,                          //   END_COLLECTION

    0x06, 0x00, 0xFF,              //   USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0xF1,                    //   USAGE (Vendor Usage 1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)

    0xc0                           // END_COLLECTION
};
#define HID_REPORT_LENGTH sizeof(hid_report_descriptor)/sizeof(hid_report_descriptor[0])
typedef struct DEMO_HID_REPORT_STRUCT {
    USHORT          input_buttons;
    SHORT           input_x;
    SHORT           input_y;
    CHAR            input_wheel;
    UCHAR           input_ac_pan;
    UCHAR           input_vendor[5];
} DEMO_HID_REPORT;


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

    if (!(error_code == UX_DESCRIPTOR_CORRUPTED ||
          error_code == UX_DEVICE_ENUMERATION_FAILURE))
    {

        /* Error callback should have been invoked.  */
        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_class_hid_logitech_pro_x_superlight_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Inform user.  */
    printf("Running ux_host_class_hid_logitech_pro_x_superlight Test............ ");

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

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_mouse_name, ux_host_class_hid_mouse_entry);
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

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the hid class parameters.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_report_descriptor;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;

    hid_parameter.ux_slave_class_hid_instance_activate = demo_device_hid_instance_activate;
    hid_parameter.ux_slave_class_hid_instance_deactivate = demo_device_hid_instance_deactivate;

    /* Initilize the device hid class. The class is connected with interface 2 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,2, (VOID *)&hid_parameter);
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

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                            status;

    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Dump input reports.  */
    UX_HOST_CLASS_HID_REPORT_GET_ID get_id;
    UX_HOST_CLASS_HID_REPORT *hid_report;
    UX_HOST_CLASS_HID_FIELD *hid_field;
    get_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    status = ux_host_class_hid_report_id_get(hid, &get_id);
    hid_report = get_id.ux_host_class_hid_report_get_report;
    while(hid_report != UX_NULL)
    {
ULONG   calc_n_bytes = 0;
ULONG   calc_n_bits = 0;
ULONG   calc_n_item = 0;
        printf("Input report %p, ID: %ld\n", (void*)hid_report, hid_report -> ux_host_class_hid_report_id);
        printf(" report n_item %ld, n_bytes %ld, n_bits %ld\n", hid_report -> ux_host_class_hid_report_number_item, hid_report -> ux_host_class_hid_report_byte_length, hid_report -> ux_host_class_hid_report_bit_length);
        hid_field = hid_report -> ux_host_class_hid_report_field;
        while(hid_field)
        {
            calc_n_item += hid_field -> ux_host_class_hid_field_report_count;
            calc_n_bits += hid_field -> ux_host_class_hid_field_report_count * hid_field -> ux_host_class_hid_field_report_size;
            printf("  Field %p, report count %ld size %ld\n",
                (void*)hid_field, hid_field ->ux_host_class_hid_field_report_count, hid_field->ux_host_class_hid_field_report_size);
            printf("    Offset %ld, value %ld\n", hid_field -> ux_host_class_hid_field_report_offset, hid_field -> ux_host_class_hid_field_value);
            printf("    Usage page %ld (%lx), min %ld, max %ld\n",
                hid_field -> ux_host_class_hid_field_usage_page, hid_field -> ux_host_class_hid_field_usage_page,
                hid_field -> ux_host_class_hid_field_usage_min, hid_field -> ux_host_class_hid_field_usage_max);
            printf("    Usage logical min %ld, max %ld\n",
                hid_field -> ux_host_class_hid_field_logical_min, hid_field -> ux_host_class_hid_field_logical_max);
            printf("    Usage n %ld(%ld) @ %p\n",
                hid_field -> ux_host_class_hid_field_number_usage,
                hid_field -> ux_host_class_hid_field_number_usage,
                hid_field -> ux_host_class_hid_field_usages);
            if (hid_field -> ux_host_class_hid_field_usages)
            {
                printf("    Usage [%lx (%ld<<16)]+",
                    *hid_field -> ux_host_class_hid_field_usages & 0xFFFF0000,
                    *hid_field -> ux_host_class_hid_field_usages >> 16);
                for (int i = 0; i < hid_field -> ux_host_class_hid_field_number_usage; i++)
                {
                    printf(" %ld", hid_field -> ux_host_class_hid_field_usages[i] & 0xFFFF);
                }
                printf("\n");
            }
            hid_field = hid_field -> ux_host_class_hid_field_next_field;
        }
        hid_report = hid_report -> ux_host_class_hid_report_next_report;

        calc_n_bytes = (calc_n_bits + 7) / 8;
        printf(" Calc n_item %ld, n_bytes %ld, n_bits %ld\n", calc_n_item, calc_n_bytes, calc_n_bits);
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

static void  tx_demo_thread_device_simulation_entry(ULONG arg)
{
UX_SLAVE_CLASS_HID_EVENT        hid_event;
DEMO_HID_REPORT                 *device_report = (DEMO_HID_REPORT*)hid_event.ux_device_class_hid_event_buffer;

    while(1)
    {
#if defined(UX_DEVICE_STANDALONE)
        ux_system_tasks_run();
#endif

        if (device_hid)
        {
            /* Wait for 2s. */
            tx_thread_sleep(2 * TX_TIMER_TICKS_PER_SECOND);
    
            /* Report ID set to 0.  */
            hid_event.ux_device_class_hid_event_report_id = 0;
    
            /* Report type set to OUTPUT.  */
            hid_event.ux_device_class_hid_event_report_type = UX_DEVICE_CLASS_HID_REPORT_TYPE_INPUT;
            
            /* Length is fixed to 1.  */
            hid_event.ux_device_class_hid_event_length = sizeof(DEMO_HID_REPORT);

            /* Set the event.  */
            ux_device_class_hid_event_set(device_hid, &hid_event);
        }
    }
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
