/* This file tests the ux_device_class_hid API. */

#include "usbx_test_common_hid.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

#include "ux_host_class_hid_mouse.h"
#include "ux_host_class_hid_keyboard.h"


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
    0x85, 0x01,                    //   REPORT_ID (1)
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

    CFG_DESC(CFG_DESC_LEN+2*HID_IFC_DESC_ALL_LEN, 2, 1)
    /* Mouse */
    HID_IFC_DESC_ALL(0, HID_MOUSE_REPORT_LENGTH, 0x81)
    /* Keyboard */
    HID_IFC_DESC_ALL(1, HID_KEYBOARD_REPORT_LENGTH, 0x82)
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

    CFG_DESC(CFG_DESC_LEN+2*HID_IFC_DESC_ALL_LEN, 2, 1)
    /* Mouse */
    HID_IFC_DESC_ALL(0, HID_MOUSE_REPORT_LENGTH, 0x81)
    /* Keyboard */
    HID_IFC_DESC_ALL(1, HID_KEYBOARD_REPORT_LENGTH, 0x82)
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

static UCHAR                               buffer[4*UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH];

static UX_HOST_CLASS_HID                   *hid = UX_NULL;
static UX_HOST_CLASS_HID_MOUSE             *hid_mouse = UX_NULL;
static UX_HOST_CLASS_HID_KEYBOARD          *hid_keyboard = UX_NULL;

static UX_SLAVE_CLASS_HID                  *hid_mouse_slave = UX_NULL;
static UX_SLAVE_CLASS_HID                  *hid_keyboard_slave = UX_NULL;

static UX_SLAVE_CLASS_HID_PARAMETER         hid_mouse_parameter;
static UX_SLAVE_CLASS_HID_PARAMETER         hid_keyboard_parameter;

static ULONG                               rsc_mem_alloc_cnt_on_set_cfg;
static ULONG                               rsc_enum_mem_alloc_count;
static ULONG                               rsc_hid_mem_alloc_count;

static ULONG                               error_callback_counter;
static UCHAR                               error_callback_ignore;

static ULONG                               error_counter = 0;

static UCHAR                               event_callback_length_error = 0;

static UINT ux_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_HID_CLIENT *client = (UX_HOST_CLASS_HID_CLIENT *)inst;
UINT                      is_mouse = (UX_SUCCESS == _ux_utility_memory_compare(client->ux_host_class_hid_client_name,
                                                                _ux_system_host_class_hid_client_mouse_name,
                                                                _ux_utility_string_length_get(_ux_system_host_class_hid_client_mouse_name)));

    // printf("hChg: %lx, %p, %p\n", event, cls, inst);
    switch(event)
    {

        case UX_HID_CLIENT_INSERTION:
            if (is_mouse)
                hid_mouse = (UX_HOST_CLASS_HID_MOUSE *)client->ux_host_class_hid_client_local_instance;
            else
                hid_keyboard = (UX_HOST_CLASS_HID_KEYBOARD *)client->ux_host_class_hid_client_local_instance;
        break;

        case UX_HID_CLIENT_REMOVAL:
            if (is_mouse)
                hid_mouse = UX_NULL;
            else
                hid_keyboard = UX_NULL;
        break;

        default:
        break;
    }
    return 0;
}

static VOID mouse_instance_activate_callback(VOID *parameter)
{
    // printf("dMouse: %p\n", parameter);
    hid_mouse_slave = (UX_SLAVE_CLASS_HID *)parameter;
}

static VOID keyboard_instance_activate_callback(VOID *parameter)
{
    // printf("dKeyboard: %p\n", parameter);
    hid_keyboard_slave = (UX_SLAVE_CLASS_HID *)parameter;
}

static VOID instance_deactivate_callback(VOID *parameter)
{
    // printf("dRm: %p\n", parameter);
    if ((VOID *)hid_mouse_slave == parameter)
        hid_mouse_slave = UX_NULL;

    if ((VOID *)hid_keyboard_slave == parameter)
        hid_keyboard_slave = UX_NULL;
}

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    if (error_code == UX_MEMORY_INSUFFICIENT)
        error_callback_counter ++;
    // printf("ERROR #%d: 0x%x, 0x%x, 0x%x\n", __LINE__, system_level, system_context, error_code);
}

static UINT break_on_all_activated(VOID)
{

    if (hid_mouse_slave == UX_NULL)
        return 0;
    if (hid_keyboard_slave == UX_NULL)
        return 0;
    if (hid_mouse == UX_NULL)
        return 0;
    if (hid_keyboard == UX_NULL)
        return 0;

    return 1;
}


static UINT break_on_all_removed(VOID)
{
    if (hid_mouse_slave || hid_keyboard_slave)
        return 0;
    if (hid_mouse || hid_keyboard)
        return 0;

    return 1;
}


static UINT  sleep_break_on_error(VOID)
{

    if (error_callback_counter >= 3)
        return error_callback_counter;

    return UX_SUCCESS;
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_class_hid_report_test_application_define(void *first_unused_memory)
#endif
{

UINT                                                status;
CHAR                                               *stack_pointer;
CHAR                                               *memory_pointer;


    /* Inform user.  */
    printf("Running ux_device_class_hid_report_... test ......,,................ ");
    stepinfo("\n");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(ux_system_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status  = ux_host_class_hid_client_register(_ux_system_host_class_hid_client_mouse_name, ux_host_class_hid_mouse_entry);
    status |= ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name, ux_host_class_hid_keyboard_entry);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
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

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the hid class parameters for mouse and keyboard.  */
    hid_mouse_parameter.ux_device_class_hid_parameter_report_address    = hid_mouse_report;
    hid_mouse_parameter.ux_device_class_hid_parameter_report_length     = HID_MOUSE_REPORT_LENGTH;
    hid_mouse_parameter.ux_device_class_hid_parameter_callback          = demo_thread_hid_callback;
    hid_mouse_parameter.ux_device_class_hid_parameter_get_callback      = demo_thread_hid_get_callback;
    hid_mouse_parameter.ux_slave_class_hid_instance_activate            = mouse_instance_activate_callback;
    hid_mouse_parameter.ux_slave_class_hid_instance_deactivate          = instance_deactivate_callback;
    hid_mouse_parameter.ux_device_class_hid_parameter_report_id         = UX_TRUE;

    hid_keyboard_parameter.ux_device_class_hid_parameter_report_address = hid_keyboard_report;
    hid_keyboard_parameter.ux_device_class_hid_parameter_report_length  = HID_KEYBOARD_REPORT_LENGTH;
    hid_keyboard_parameter.ux_device_class_hid_parameter_callback       = UX_NULL;
    hid_keyboard_parameter.ux_device_class_hid_parameter_get_callback   = UX_NULL;
    hid_keyboard_parameter.ux_slave_class_hid_instance_activate         = keyboard_instance_activate_callback;

    /* Initilize the device hid class. */
    status  = ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                             1,0, (VOID *)&hid_mouse_parameter);
    status |= ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                             1,1, (VOID *)&hid_keyboard_parameter);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
#endif

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
UX_DEVICE                                           *device;
UX_ENDPOINT                                         *endpoint;
UX_TRANSFER                                         *transfer_request;

    stepinfo(">>>>>>>>>> Thread start\n");

    ux_test_breakable_sleep(200, break_on_all_activated);

    /* Get device instance. */
    status = ux_host_stack_device_get(0, &device);

    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: get_device fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Get control endpoint and control transfer request. */
    endpoint = &device->ux_device_control_endpoint;
    transfer_request = &endpoint->ux_endpoint_transfer_request;

    /* Create a transfer request for the SET_REPORT request.  */
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_SET_REPORT;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;

    if (hid_keyboard)
    {

        stepinfo(">>>>>>>>>> SetReport(noID, %d)\n", UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH + 1);

        transfer_request -> ux_transfer_request_requested_length =  UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH + 1;
        transfer_request -> ux_transfer_request_value =             (UINT)((USHORT) 0 | (USHORT) UX_HOST_CLASS_HID_REPORT_TYPE_OUTPUT << 8);
        transfer_request -> ux_transfer_request_index =             hid_keyboard->ux_host_class_hid_keyboard_hid->ux_host_class_hid_interface->ux_interface_descriptor.bInterfaceNumber;

        status = ux_host_stack_transfer_request(transfer_request);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d: code 0x%x\n", __LINE__, status);
            error_counter ++;
        }
    }

    stepinfo(">>>>>>>>>> SetReport(1, %d)\n", UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH + 1);

    /* Create a transfer request for the SET_REPORT request.  */
    transfer_request -> ux_transfer_request_requested_length =  UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH;
    transfer_request -> ux_transfer_request_value =             (UINT)((USHORT) 1 | (USHORT) UX_HOST_CLASS_HID_REPORT_TYPE_OUTPUT << 8);
    transfer_request -> ux_transfer_request_index =             hid_mouse->ux_host_class_hid_mouse_hid->ux_host_class_hid_interface->ux_interface_descriptor.bInterfaceNumber;

    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        error_counter ++;
    }

    if (hid_keyboard)
    {

        /* Create a transfer request for the GET_REPORT request.  */
        transfer_request -> ux_transfer_request_data_pointer =      buffer;
        transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_GET_REPORT;
        transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;

        stepinfo(">>>>>>>>>> GetReport(noID, %d)\n", UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH + 1);

        transfer_request -> ux_transfer_request_requested_length =  UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH + 1;
        transfer_request -> ux_transfer_request_value =             (UINT)((USHORT) 0 | (USHORT) UX_HOST_CLASS_HID_REPORT_TYPE_INPUT << 8);
        transfer_request -> ux_transfer_request_index =             hid_keyboard->ux_host_class_hid_keyboard_hid->ux_host_class_hid_interface->ux_interface_descriptor.bInterfaceNumber;

        status = ux_host_stack_transfer_request(transfer_request);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d: code 0x%x\n", __LINE__, status);
            error_counter ++;
        }
    }

    /* Create a transfer request for the GET_REPORT request.  */
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_GET_REPORT;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;

    if (hid_keyboard)
    {

        /* Request a size that is smaller than the event buffer.  */
        stepinfo(">>>>>>>>>> GetReport(noID, %d)\n", UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH - 1);

        /* Suspend the background keyboard interrupt thread so that it doesn't get our report.  */
        tx_thread_suspend(&hid_keyboard_slave->ux_slave_class_hid_interface->ux_slave_interface_class->ux_slave_class_thread);

        /* Add an event so the device will try to send one back to the host.  */
        UX_SLAVE_CLASS_HID_EVENT hid_event = { 0 };
        hid_event.ux_device_class_hid_event_length = UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH;
        hid_event.ux_device_class_hid_event_report_id = 0;
        hid_event.ux_device_class_hid_event_report_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
        status = _ux_device_class_hid_event_set(hid_keyboard_slave, &hid_event);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d: code 0x%x\n", __LINE__, status);
            error_counter ++;
        }

        transfer_request -> ux_transfer_request_data_pointer =      buffer;
        transfer_request -> ux_transfer_request_requested_length =  UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH - 1;
        transfer_request -> ux_transfer_request_value =             (UINT)((USHORT) 0 | (USHORT) UX_HOST_CLASS_HID_REPORT_TYPE_INPUT << 8);
        transfer_request -> ux_transfer_request_index =             hid_keyboard->ux_host_class_hid_keyboard_hid->ux_host_class_hid_interface->ux_interface_descriptor.bInterfaceNumber;

        status = ux_host_stack_transfer_request(transfer_request);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d: code 0x%x\n", __LINE__, status);
            error_counter ++;
        }

        /* Resume the keyboard interrupt thread.  */
        tx_thread_suspend(&hid_keyboard_slave->ux_slave_class_hid_interface->ux_slave_interface_class->ux_slave_class_thread);

        /* Request a size that is larger than the max control transfer, and when there are no events.  */
        stepinfo(">>>>>>>>>> SetReport(noID, %d)\n", UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH + 1);

        transfer_request -> ux_transfer_request_data_pointer =      buffer;
        transfer_request -> ux_transfer_request_requested_length =  UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH + 1;
        transfer_request -> ux_transfer_request_value =             (UINT)((USHORT) 0 | (USHORT) UX_HOST_CLASS_HID_REPORT_TYPE_INPUT << 8);
        transfer_request -> ux_transfer_request_index =             hid_keyboard->ux_host_class_hid_keyboard_hid->ux_host_class_hid_interface->ux_interface_descriptor.bInterfaceNumber;

        status = ux_host_stack_transfer_request(transfer_request);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d: code 0x%x\n", __LINE__, status);
            error_counter ++;
        }
    }

    stepinfo(">>>>>>>>>> SetReport(1, %d)\n", UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH + 1);

    /* Create a transfer request for the SET_REPORT request.  */
    transfer_request -> ux_transfer_request_requested_length =  UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH + 1;
    transfer_request -> ux_transfer_request_value =             (UINT)((USHORT) 1 | (USHORT) UX_HOST_CLASS_HID_REPORT_TYPE_INPUT << 8);
    transfer_request -> ux_transfer_request_index =             hid_mouse->ux_host_class_hid_mouse_hid->ux_host_class_hid_interface->ux_interface_descriptor.bInterfaceNumber;

    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        error_counter ++;
    }

    stepinfo(">>>>>>>>>> SetReport(FEATURE, 1, %d)\n", 2);

    /* Create a transfer request for the SET_REPORT request.  */
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_SET_REPORT;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_requested_length =  2;
    transfer_request -> ux_transfer_request_value =             (UINT)((USHORT) 1 | (USHORT) UX_HOST_CLASS_HID_REPORT_TYPE_FEATURE << 8);
    transfer_request -> ux_transfer_request_index =             hid_mouse->ux_host_class_hid_mouse_hid->ux_host_class_hid_interface->ux_interface_descriptor.bInterfaceNumber;
    buffer[0] = 1;
    buffer[1] = 0x5A;

    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        error_counter ++;
    }

    stepinfo(">>>>>>>>>> GetReport(FEATURE, 1, %d)\n", 2);

    /* Create a transfer request for the GET_REPORT request.  */
    transfer_request -> ux_transfer_request_function =          UX_HOST_CLASS_HID_GET_REPORT;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_requested_length =  2;
    transfer_request -> ux_transfer_request_value =             (UINT)((USHORT) 1 | (USHORT) UX_HOST_CLASS_HID_REPORT_TYPE_FEATURE << 8);
    transfer_request -> ux_transfer_request_index =             hid_mouse->ux_host_class_hid_mouse_hid->ux_host_class_hid_interface->ux_interface_descriptor.bInterfaceNumber;
    buffer[0] = 0;
    buffer[1] = 0;

    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        error_counter ++;
    }
    if (buffer[0] != 1)
    {
        printf("ERROR #%d: buffer[0] %x\n", __LINE__, buffer[0]);
        error_counter ++;
    }
    if (buffer[1] != 0x5A)
    {
        printf("ERROR #%d: buffer[1] %x\n", __LINE__, buffer[1]);
        error_counter ++;
    }

    stepinfo(">>>>>>>>>> GetReport(FEATURE, 1, %d)\n", UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH + 2);

    /* Create a transfer request for the SET_REPORT request.  */
    transfer_request -> ux_transfer_request_requested_length =  UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH + 2;
    transfer_request -> ux_transfer_request_value =             (UINT)((USHORT) 1 | (USHORT) UX_HOST_CLASS_HID_REPORT_TYPE_FEATURE << 8);
    transfer_request -> ux_transfer_request_index =             hid_mouse->ux_host_class_hid_mouse_hid->ux_host_class_hid_interface->ux_interface_descriptor.bInterfaceNumber;
    event_callback_length_error = UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH + 3;

    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        error_counter ++;
    }

    if (hid_keyboard)
    {

        stepinfo(">>>>>>>>>> GetReport(keyboard, FEATURE, 1, %d)\n", 1);

        /* Create a transfer request for the SET_REPORT request.  */
        transfer_request -> ux_transfer_request_requested_length =  1;
        transfer_request -> ux_transfer_request_value =             (UINT)((USHORT) 1 | (USHORT) UX_HOST_CLASS_HID_REPORT_TYPE_FEATURE << 8);
        transfer_request -> ux_transfer_request_index =             hid_keyboard->ux_host_class_hid_keyboard_hid->ux_host_class_hid_interface->ux_interface_descriptor.bInterfaceNumber;

        status = ux_host_stack_transfer_request(transfer_request);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d: code 0x%x\n", __LINE__, status);
            error_counter ++;
        }
    }

    stepinfo(">>>>>>>>>> Test done\n");

    /* Now disconnect the device.  */
    _ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    if (error_counter)
    {
        printf("FAIL %ld errors!\n", error_counter);
        test_control_return(1);
    }

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}

static UX_SLAVE_CLASS_HID_EVENT hid_mouse_slave_event;
static UX_SLAVE_CLASS_HID_EVENT hid_keyboard_slave_event;
static UINT    demo_thread_hid_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{
    /* Event buffer contains no report ID.  */
    if (class == hid_mouse_slave)
        _ux_utility_memory_copy(&hid_mouse_slave_event, event, sizeof(UX_SLAVE_CLASS_HID_EVENT));
    if (class == hid_keyboard_slave)
        _ux_utility_memory_copy(&hid_keyboard_slave_event, event, sizeof(UX_SLAVE_CLASS_HID_EVENT));
    return(UX_SUCCESS);
}

static UINT    demo_thread_hid_get_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{
UX_SLAVE_CLASS_HID_EVENT *class_event;
    if (class == hid_mouse_slave)
        class_event = &hid_mouse_slave_event;
    else if (class == hid_keyboard_slave)
        class_event = &hid_keyboard_slave_event;
    else
        return(UX_ERROR);
    if (class -> ux_device_class_hid_report_id)
    {
        /* First byte in buffer should be report ID, if report ID is required.
         * See HID spec. for more details.
         */
        event->ux_device_class_hid_event_report_id = class_event->ux_device_class_hid_event_report_id;
        event->ux_device_class_hid_event_report_type = class_event->ux_device_class_hid_event_report_type;
        if (class_event->ux_device_class_hid_event_length < UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH - 1)
            class_event->ux_device_class_hid_event_length += 1;
        event->ux_device_class_hid_event_length = class_event->ux_device_class_hid_event_length;
        *(event->ux_device_class_hid_event_buffer) = (UCHAR)event->ux_device_class_hid_event_report_id;
        _ux_utility_memory_copy(event->ux_device_class_hid_event_buffer + 1,
                                class_event->ux_device_class_hid_event_buffer,
                                event->ux_device_class_hid_event_length - 1);
    }
    else
        _ux_utility_memory_copy(event, class_event, sizeof(UX_SLAVE_CLASS_HID_EVENT));
    if (event_callback_length_error)
    {
        event->ux_device_class_hid_event_length = event_callback_length_error;
        event_callback_length_error = 0;
    }
    return(UX_SUCCESS);
}