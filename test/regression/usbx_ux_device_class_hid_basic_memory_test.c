/* This file tests the ux_device_class_hid API. */

#include "usbx_test_common_hid.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

#include "ux_host_class_hid_mouse.h"
#include "ux_host_class_hid_keyboard.h"
#include "ux_host_class_hid_remote_control.h"


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
    0xc0,                          // END_COLLECTION

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
    0xc0,                          // END_COLLECTION
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
    0xc0,                          // END_COLLECTION
};
#define HID_MOUSE_REPORT_LENGTH (sizeof(hid_mouse_report)/sizeof(hid_mouse_report[0]))

static UCHAR hid_remote_control_report[] = {

    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x02,                    //   USAGE (Numeric Key Pad)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x0a,                    //     USAGE_MAXIMUM (Button 10)
    0x15, 0x01,                    //     LOGICAL_MINIMUM (1)
    0x25, 0x0a,                    //     LOGICAL_MAXIMUM (10)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x00,                    //     INPUT (Data,Ary,Abs)
    0xc0,                          //   END_COLLECTION
    0x05, 0x0c,                    //   USAGE_PAGE (Consumer Devices)
    0x09, 0x86,                    //   USAGE (Channel)
    0x09, 0xe0,                    //   USAGE (Volume)
    0x15, 0xff,                    //   LOGICAL_MINIMUM (-1)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x02,                    //   REPORT_SIZE (2)
    0x95, 0x02,                    //   REPORT_COUNT (2)
    0x81, 0x46,                    //   INPUT (Data,Var,Rel,Null)
    0xc0                           // END_COLLECTION
};
#define HID_REMOTE_CONTROL_REPORT_LENGTH (sizeof(hid_remote_control_report)/sizeof(hid_remote_control_report[0]))

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

    CFG_DESC(CFG_DESC_LEN+3*HID_IFC_DESC_ALL_LEN, 2, 1)
    /* Keyboard */
    HID_IFC_DESC_ALL(0, HID_KEYBOARD_REPORT_LENGTH, 0x82)
    /* Mouse */
    HID_IFC_DESC_ALL(1, HID_MOUSE_REPORT_LENGTH, 0x81)
    /* Remote control */
    HID_IFC_DESC_ALL(2, HID_REMOTE_CONTROL_REPORT_LENGTH, 0x83)
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

    CFG_DESC(CFG_DESC_LEN+3*HID_IFC_DESC_ALL_LEN, 2, 1)
    /* Keyboard */
    HID_IFC_DESC_ALL(0, HID_KEYBOARD_REPORT_LENGTH, 0x82)
    /* Mouse */
    HID_IFC_DESC_ALL(1, HID_MOUSE_REPORT_LENGTH, 0x81)
    /* Remote control */
    HID_IFC_DESC_ALL(2, HID_REMOTE_CONTROL_REPORT_LENGTH, 0x83)
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

static UX_HOST_CLASS_HID                   *hid = UX_NULL;
static UX_HOST_CLASS_HID_MOUSE             *hid_mouse = UX_NULL;
static UX_HOST_CLASS_HID_KEYBOARD          *hid_keyboard = UX_NULL;
static UX_HOST_CLASS_HID_REMOTE_CONTROL    *hid_remote_control = UX_NULL;

static UX_SLAVE_CLASS_HID                  *hid_mouse_slave = UX_NULL;
static UX_SLAVE_CLASS_HID                  *hid_keyboard_slave = UX_NULL;
static UX_SLAVE_CLASS_HID                  *hid_remote_control_slave = UX_NULL;

static UX_SLAVE_CLASS_HID_PARAMETER         hid_mouse_parameter;
static UX_SLAVE_CLASS_HID_PARAMETER         hid_keyboard_parameter;
static UX_SLAVE_CLASS_HID_PARAMETER         hid_remote_control_parameter;

static ULONG                               rsc_mem_alloc_cnt_on_set_cfg;
static ULONG                               rsc_enum_mem_alloc_count;
static ULONG                               rsc_hid_mem_alloc_count;

static ULONG                               error_callback_counter;
static UCHAR                               error_callback_ignore;

static ULONG                               error_counter = 0;

static UINT ux_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_HID_CLIENT *client = (UX_HOST_CLASS_HID_CLIENT *)inst;
UINT                      is_mouse = (UX_SUCCESS == _ux_utility_memory_compare(client->ux_host_class_hid_client_name,
                                                                _ux_system_host_class_hid_client_mouse_name,
                                                                _ux_utility_string_length_get(_ux_system_host_class_hid_client_mouse_name)));
UINT                      is_keyboard = (UX_SUCCESS == _ux_utility_memory_compare(client->ux_host_class_hid_client_name,
                                                                _ux_system_host_class_hid_client_keyboard_name,
                                                                _ux_utility_string_length_get(_ux_system_host_class_hid_client_keyboard_name)));

    // if(event >= UX_HID_CLIENT_INSERTION) printf("hChg: ev %lx, cls %p, inst %p, %s\n", event, cls, inst, is_mouse ? "Mouse" : "Keyboard");
    switch(event)
    {

        case UX_HID_CLIENT_INSERTION:
            if (is_mouse)
                hid_mouse = (UX_HOST_CLASS_HID_MOUSE *)client->ux_host_class_hid_client_local_instance;
            else
            {
                if (is_keyboard)
                    hid_keyboard = (UX_HOST_CLASS_HID_KEYBOARD *)client->ux_host_class_hid_client_local_instance;
                else
                    hid_remote_control = (UX_HOST_CLASS_HID_REMOTE_CONTROL *)client->ux_host_class_hid_client_local_instance;
            }
        break;

        case UX_HID_CLIENT_REMOVAL:
            if (is_mouse)
                hid_mouse = UX_NULL;
            else
            {
                if (is_keyboard)
                    hid_keyboard = UX_NULL;
                else
                    hid_remote_control = UX_NULL;
            }
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

static VOID remote_control_instance_activate_callback(VOID *parameter)
{
    // printf("dKeyboard: %p\n", parameter);
    hid_remote_control_slave = (UX_SLAVE_CLASS_HID *)parameter;
}

static VOID instance_deactivate_callback(VOID *parameter)
{
    // printf("dRm: %p\n", parameter);
    if ((VOID *)hid_mouse_slave == parameter)
        hid_mouse_slave = UX_NULL;

    if ((VOID *)hid_keyboard_slave == parameter)
        hid_keyboard_slave = UX_NULL;

    if ((VOID *)hid_remote_control_slave == parameter)
        hid_remote_control_slave = UX_NULL;
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
    if (hid_remote_control_slave == UX_NULL)
        return 0;
    if (hid_mouse == UX_NULL)
        return 0;
    if (hid_keyboard == UX_NULL)
        return 0;
    if (hid_remote_control == UX_NULL)
        return 0;

    return 1;
}


static UINT break_on_all_removed(VOID)
{
    if (hid_mouse_slave || hid_keyboard_slave || hid_remote_control_slave)
        return 0;
    if (hid_mouse || hid_keyboard || hid_remote_control)
        return 0;

    return 1;
}


static UINT  sleep_break_on_error(VOID)
{

    if (error_callback_counter >= 3)
        return error_callback_counter;

    return UX_SUCCESS;
}


static VOID ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *_params)
{
    rsc_mem_alloc_cnt_on_set_cfg = ux_test_utility_sim_mem_alloc_count();
}

static UX_TEST_SETUP _SetConfigure = UX_TEST_SETUP_SetConfigure;

static UX_TEST_HCD_SIM_ACTION log_on_SetCfg[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_SetConfigure,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_SUCCESS, ux_test_hcd_entry_set_cfg,
        UX_TRUE}, /* Invoke callback & continue */
{   0   }
};

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_class_hid_basic_memory_test_application_define(void *first_unused_memory)
#endif
{

UINT                                                status;
CHAR                                               *stack_pointer;
CHAR                                               *memory_pointer;
ULONG                                               mem_free;
ULONG                                               alloc_count;
ULONG                                               test_n;

    /* Inform user.  */
    printf("Running ux_device_class_hid Basic Memory test ...................... ");
    stepinfo("\n");

    /* Initialize memory logger. */
    ux_test_utility_sim_mem_alloc_log_enable(UX_TRUE);
    ux_test_utility_sim_mem_alloc_count_reset();

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
    status |= ux_host_class_hid_client_register(_ux_system_host_class_hid_client_mouse_name, ux_host_class_hid_mouse_entry);
    status |= ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name, ux_host_class_hid_keyboard_entry);
    status |= ux_host_class_hid_client_register(_ux_system_host_class_hid_client_remote_control_name, ux_host_class_hid_remote_control_entry);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
#endif

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
    hid_mouse_parameter.ux_slave_class_hid_instance_activate            = mouse_instance_activate_callback;
    hid_mouse_parameter.ux_slave_class_hid_instance_deactivate          = instance_deactivate_callback;

    hid_keyboard_parameter.ux_device_class_hid_parameter_report_address = hid_keyboard_report;
    hid_keyboard_parameter.ux_device_class_hid_parameter_report_length  = HID_KEYBOARD_REPORT_LENGTH;
    hid_keyboard_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;
    hid_keyboard_parameter.ux_slave_class_hid_instance_activate         = keyboard_instance_activate_callback;
    hid_keyboard_parameter.ux_slave_class_hid_instance_deactivate       = instance_deactivate_callback;

    hid_remote_control_parameter.ux_device_class_hid_parameter_report_address = hid_remote_control_report;
    hid_remote_control_parameter.ux_device_class_hid_parameter_report_length  = HID_REMOTE_CONTROL_REPORT_LENGTH;
    hid_remote_control_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;
    hid_remote_control_parameter.ux_slave_class_hid_instance_activate =         remote_control_instance_activate_callback;
    hid_remote_control_parameter.ux_slave_class_hid_instance_deactivate =       instance_deactivate_callback;

    stepinfo(">>>>>>>>>> Test HID Class Initialize/deinitialize memory\n");

    stepinfo(">>>>>>>>>> - Reset counts\n");
    ux_test_utility_sim_mem_alloc_count_reset();
    mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    stepinfo(">>>>>>>>>> - _class_register\n");

    /* Initilize the device hid class. */
    status  = ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                             1,1, (VOID *)&hid_mouse_parameter);
    status |= ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                             1,0, (VOID *)&hid_keyboard_parameter);
    status |= ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                             1,2, (VOID *)&hid_remote_control_parameter);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
#endif
#if 1
    /* Log create counts when instances active for further tests. */
    alloc_count = ux_test_utility_sim_mem_alloc_count();

    /* Lock log base for tests. */
    ux_test_utility_sim_mem_alloc_log_lock();

    stepinfo("init & uninit alloc : %ld\n", alloc_count);
    stepinfo("mem free            : %ld, %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available);

    if (alloc_count) stepinfo(">>>>>>>>>> - Init/deinit memory errors test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < alloc_count; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, alloc_count - 1);

        /* Unregister. */
        ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);
        ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);
        ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #%d.%ld: Memory level different after re-enumerations %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            error_counter ++;
            test_control_return(1);
        }

        /* Set memory error generation */
        ux_test_utility_sim_mem_alloc_error_generation_start(test_n);

        /* Register. */
        status  = ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,1, (VOID *)&hid_mouse_parameter);
        status |= ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,0, (VOID *)&hid_keyboard_parameter);
        status |= ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,2, (VOID *)&hid_remote_control_parameter);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
        /* Check error */
        if (status == UX_SUCCESS)
        {

            printf("ERROR #%d.%ld: registered when there is memory error\n", __LINE__, test_n);
            error_counter ++;
        }
#endif

        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_mem_alloc_error_generation_stop();
    if (alloc_count) stepinfo("\n");

    /* Unregister. */
    ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);
    ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);
    ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);

    /* Register. */
    status  = ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                            1,1, (VOID *)&hid_mouse_parameter);
    status |= ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                            1,0, (VOID *)&hid_keyboard_parameter);
    status |= ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                            1,2, (VOID *)&hid_remote_control_parameter);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
#endif
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

#if defined(UX_DEVICE_STANDALONE)

    /* Create the main device simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_device_simulation, "tx demo device simulation", tx_demo_thread_device_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
#endif
}

#if defined(UX_DEVICE_STANDALONE)
static void  tx_demo_thread_device_simulation_entry(ULONG arg)
{
    while(1)
    {
        ux_system_tasks_run();
        tx_thread_relinquish();
    }
}
#endif

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
UX_DEVICE                                           *device;
ULONG                                               mem_free;
ULONG                                               alloc_count;
ULONG                                               test_n;
UX_HCD                                              *hcd;


    stepinfo(">>>>>>>>>> Thread start\n");

    hcd = &_ux_system_host->ux_system_host_hcd_array[0];

    ux_test_breakable_sleep(500, break_on_all_activated);

    /* Get device instance. */
    status = ux_host_stack_device_get(0, &device);

    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: get_device fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>> Disconnect\n");

    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();

    if (hid_keyboard || hid_mouse)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>> Reset counts\n");

    ux_test_utility_sim_mem_alloc_count_reset();
    mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    rsc_mem_alloc_cnt_on_set_cfg = 0;
    ux_test_hcd_sim_host_set_actions(log_on_SetCfg);

    stepinfo(">>>>>>>>>> Connect\n");

    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    ux_test_breakable_sleep(500, break_on_all_activated);

    /* Log create counts for further tests. */
    rsc_enum_mem_alloc_count = rsc_mem_alloc_cnt_on_set_cfg;

    /* Log create counts when instances active for further tests. */
    alloc_count = ux_test_utility_sim_mem_alloc_count();

    /* Lock log base for tests. */
    ux_test_utility_sim_mem_alloc_log_lock();

    rsc_hid_mem_alloc_count = alloc_count - rsc_enum_mem_alloc_count;

    stepinfo("enum mem: %ld\n", rsc_enum_mem_alloc_count);
    stepinfo("hid mem : %ld\n", rsc_hid_mem_alloc_count);
    stepinfo("mem free: %ld, %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available);

    if (hid_mouse == UX_NULL
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
        || hid_keyboard == UX_NULL
#endif
        )
    {
        printf("ERROR #%d: %p %p\n", __LINE__, hid_keyboard, hid_mouse);
        test_control_return(1);
    }

    /* Simulate detach and attach for FS enumeration,
       and test possible memory allocation error handlings.
     */
    if (rsc_hid_mem_alloc_count) stepinfo(">>>>>>>>>> Memory errors enumeration test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < rsc_hid_mem_alloc_count; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_hid_mem_alloc_count - 1);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Check number of devices.  */
        if (hcd->ux_hcd_nb_devices != 0)
        {
            printf("ERROR #%d.%ld: number of devices (%d) must be 0\n", __LINE__, test_n, hcd->ux_hcd_nb_devices);
            error_counter ++;
        }

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #%d.%ld: Memory level different after re-enumerations %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            error_counter ++;
            test_control_return(1);
        }

        /* Set memory error generation */
        ux_test_utility_sim_mem_alloc_error_generation_start(test_n + rsc_enum_mem_alloc_count);

        /* Connect. */
        error_callback_counter = 0;
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
#if 1 /* @nick */
        ux_test_hcd_sim_host_connect_no_wait(UX_FULL_SPEED_DEVICE);

        /* Wait for enum thread to complete. */
        ux_test_wait_for_enum_thread_completion();
#else
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        /* Wait and break on errors. */
        ux_test_breakable_sleep(400, sleep_break_on_error);
#endif

        /* Check error */
        if (hid_mouse && hid_keyboard && hid_mouse_slave && hid_keyboard_slave && hid_remote_control && hid_remote_control_slave)
        {

            printf("ERROR #%d.%ld: device detected when there is memory error\n", __LINE__, test_n);
            error_counter ++;
        }
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
        ux_test_utility_sim_mem_alloc_error_generation_stop();
    }
    if (alloc_count) stepinfo("\n");

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

static UINT    demo_thread_hid_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{
    return(UX_SUCCESS);
}
