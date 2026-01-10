/*
 This test verifies the optional HID set protocol callback behavior:
 - Callback is invoked with correct protocol (boot=0, report=1)
 - Callback receives the correct HID instance pointer
 - When callback is NULL, SET_PROTOCOL still functions correctly
*/

#include "usbx_test_common_hid.h"

#include "ux_host_class_hid_keyboard.h"
#include "ux_device_class_hid.h"

#define DUMMY_USBX_MEMORY_SIZE (64*1024)
static UCHAR dummy_usbx_memory[DUMMY_USBX_MEMORY_SIZE];

static UX_SLAVE_CLASS_HID * g_hid_device = UX_NULL;

static volatile UINT g_set_protocol_callback_calls = 0;
static volatile ULONG g_set_protocol_last_value = 0xFFFFFFFFu;
static volatile UX_SLAVE_CLASS_HID * g_set_protocol_last_hid = UX_NULL;

static UCHAR    dummy_report[8];
static UCHAR    buffer[64];

static UCHAR hid_report_descriptor[] = {
    0x05, 0x01,                    /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x06,                    /* USAGE (Keyboard) */
    0xa1, 0x01,                    /* COLLECTION (Application) */
    0x05, 0x07,                    /*   USAGE_PAGE (Keyboard) */
    0x19, 0xe0,                    /*   USAGE_MINIMUM (Keyboard LeftControl) */
    0x29, 0xe7,                    /*   USAGE_MAXIMUM (Keyboard Right GUI) */
    0x15, 0x00,                    /*   LOGICAL_MINIMUM (0) */
    0x25, 0x01,                    /*   LOGICAL_MAXIMUM (1) */
    0x75, 0x01,                    /*   REPORT_SIZE (1) */
    0x95, 0x08,                    /*   REPORT_COUNT (8) */
    0x81, 0x02,                    /*   INPUT (Data,Var,Abs) */
    0x95, 0x01,                    /*   REPORT_COUNT (1) */
    0x75, 0x08,                    /*   REPORT_SIZE (8) */
    0x81, 0x03,                    /*   INPUT (Cnst,Var,Abs) */
    0x95, 0x05,                    /*   REPORT_COUNT (5) */
    0x75, 0x01,                    /*   REPORT_SIZE (1) */
    0x05, 0x08,                    /*   USAGE_PAGE (LEDs) */
    0x19, 0x01,                    /*   USAGE_MINIMUM (Num Lock) */
    0x29, 0x05,                    /*   USAGE_MAXIMUM (Kana) */
    0x91, 0x02,                    /*   OUTPUT (Data,Var,Abs) */
    0x95, 0x01,                    /*   REPORT_COUNT (1) */
    0x75, 0x03,                    /*   REPORT_SIZE (3) */
    0x91, 0x03,                    /*   OUTPUT (Cnst,Var,Abs) */
    0x95, 0x06,                    /*   REPORT_COUNT (6) */
    0x75, 0x08,                    /*   REPORT_SIZE (8) */
    0x15, 0x00,                    /*   LOGICAL_MINIMUM (0) */
    0x25, 0x65,                    /*   LOGICAL_MAXIMUM (101) */
    0x05, 0x07,                    /*   USAGE_PAGE (Keyboard) */
    0x19, 0x00,                    /*   USAGE_MINIMUM (Reserved) */
    0x29, 0x65,                    /*   USAGE_MAXIMUM */
    0x81, 0x00,                    /*   INPUT (Data,Ary,Abs) */
    0xc0                           /* END_COLLECTION */
};
#define HID_REPORT_LENGTH (sizeof(hid_report_descriptor)/sizeof(hid_report_descriptor[0]))

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

#define LANGUAGE_ID_FRAMEWORK_LENGTH 2
static UCHAR language_id_framework[] = { 0x09, 0x04 };

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    /* Not expecting errors in this test; print and fail. */
    printf("Error on line %d, system_level: %u, system_context: %u, error: 0x%x\n", __LINE__, system_level, system_context, error_code);
    test_control_return(1);
}

static VOID test_hid_instance_activate(VOID *inst)
{
    g_hid_device = (UX_SLAVE_CLASS_HID *)inst;
}
static VOID test_hid_instance_deactivate(VOID *inst)
{
    if ((VOID *)g_hid_device == inst)
        g_hid_device = UX_NULL;
}

static VOID test_set_protocol_callback(UX_SLAVE_CLASS_HID *hid, ULONG protocol)
{
    g_set_protocol_callback_calls++;
    g_set_protocol_last_value = protocol;
    g_set_protocol_last_hid = hid;
}

UINT  _ux_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter);

static UINT ux_system_host_change_function(ULONG a, UX_HOST_CLASS *b, VOID *c)
{
    UX_PARAMETER_NOT_USED(a);
    UX_PARAMETER_NOT_USED(b);
    UX_PARAMETER_NOT_USED(c);
    return 0;
}

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_class_hid_set_protocol_callback_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;

    printf("Running HID Set Protocol Callback Test......................... ");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);
    if (status != UX_SUCCESS)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    ux_utility_error_callback_register(error_callback);

    /* Host stack init. */
    status =  ux_host_stack_initialize(ux_system_host_change_function);
    if (status != UX_SUCCESS)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the HID keyboard client (ensures enumeration). */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name, ux_host_class_hid_keyboard_entry);
    if (status != UX_SUCCESS)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Device stack init. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                         device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                         string_framework, STRING_FRAMEWORK_LENGTH,
                                         language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize HID class parameters, including set_protocol callback. */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_report_descriptor;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = UX_NULL; /* Not used in this test. */
    hid_parameter.ux_slave_class_hid_instance_activate         = test_hid_instance_activate;
    hid_parameter.ux_slave_class_hid_instance_deactivate       = test_hid_instance_deactivate;
    hid_parameter.ux_device_class_hid_parameter_set_protocol_callback = test_set_protocol_callback;

    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,2, (VOID *)&hid_parameter);
    if(status!=UX_SUCCESS)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize simulated device controller and host HCD. */
    status =  _ux_dcd_sim_slave_initialize();
    if (status != UX_SUCCESS)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Wait for enumeration. */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Use host HID API to set protocol. */

    /* Fetch slave HID instance pointer. */
    UX_SLAVE_CLASS_HID *slave_hid = _ux_system_slave -> ux_system_slave_device.ux_slave_device_first_interface -> ux_slave_interface_class_instance;

    /* Sanity: default protocol is report (1). */
    if (ux_device_class_hid_protocol_get(slave_hid) != 1)
    {
        printf("Error on line %d, protocol not default report\n", __LINE__);
        test_control_return(1);
    }
    {
        USHORT host_protocol = 0xFFFF;
        status = ux_host_class_hid_protocol_get(hid, &host_protocol);
        if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
        {
            printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }
        if (status == UX_SUCCESS && host_protocol != UX_HOST_CLASS_HID_PROTOCOL_REPORT)
        {
            printf("Error on line %d, host get protocol not report\n", __LINE__);
            test_control_return(1);
        }
    }

    /* Issue SET_PROTOCOL to BOOT (0). */
    status =  ux_host_class_hid_protocol_set(hid, UX_HOST_CLASS_HID_PROTOCOL_BOOT);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Verify callback captured boot protocol and correct HID instance. */
    if (status == UX_SUCCESS)
    {
        if (g_set_protocol_callback_calls < 1 || g_set_protocol_last_value != 0 || g_set_protocol_last_hid != slave_hid)
        {
            printf("Error on line %d, callback verification failed (calls=%u, val=%lu, hid=%p vs %p)\n",
                   __LINE__, g_set_protocol_callback_calls, g_set_protocol_last_value, g_set_protocol_last_hid, slave_hid);
            test_control_return(1);
        }
        if (ux_device_class_hid_protocol_get(slave_hid) != 0)
        {
            printf("Error on line %d, protocol not set to boot\n", __LINE__);
            test_control_return(1);
        }
        {
            USHORT host_protocol = 0xFFFF;
            status = ux_host_class_hid_protocol_get(hid, &host_protocol);
            if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
            {
                printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
                test_control_return(1);
            }
            if (status == UX_SUCCESS && host_protocol != UX_HOST_CLASS_HID_PROTOCOL_BOOT)
            {
                printf("Error on line %d, host get protocol not boot\n", __LINE__);
                test_control_return(1);
            }
        }
    }

    /* Issue SET_PROTOCOL to REPORT (1). */
    status =  ux_host_class_hid_protocol_set(hid, UX_HOST_CLASS_HID_PROTOCOL_REPORT);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    if (status == UX_SUCCESS)
    {
        if (g_set_protocol_callback_calls < 2 || g_set_protocol_last_value != 1 || g_set_protocol_last_hid != slave_hid)
        {
            printf("Error on line %d, callback verification failed (calls=%u, val=%lu, hid=%p vs %p)\n",
                   __LINE__, g_set_protocol_callback_calls, g_set_protocol_last_value, g_set_protocol_last_hid, slave_hid);
            test_control_return(1);
        }
        if (ux_device_class_hid_protocol_get(slave_hid) != 1)
        {
            printf("Error on line %d, protocol not set to report\n", __LINE__);
            test_control_return(1);
        }
        {
            USHORT host_protocol = 0xFFFF;
            status = ux_host_class_hid_protocol_get(hid, &host_protocol);
            if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
            {
                printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
                test_control_return(1);
            }
            if (status == UX_SUCCESS && host_protocol != UX_HOST_CLASS_HID_PROTOCOL_REPORT)
            {
                printf("Error on line %d, host get protocol not report\n", __LINE__);
                test_control_return(1);
            }
        }
    }

    /* Now simulate NULL callback: disable callback pointer and ensure protocol change still works. */
    slave_hid -> ux_device_class_hid_set_protocol_callback = UX_NULL;

    /* Flip to BOOT again. */
    UINT calls_before = g_set_protocol_callback_calls;
    status =  ux_host_class_hid_protocol_set(hid, UX_HOST_CLASS_HID_PROTOCOL_BOOT);
    if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
    {
        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (status == UX_SUCCESS)
    {
        if (ux_device_class_hid_protocol_get(slave_hid) != 0)
        {
            printf("Error on line %d, protocol not set to boot with NULL callback\n", __LINE__);
            test_control_return(1);
        }
        {
            USHORT host_protocol = 0xFFFF;
            status = ux_host_class_hid_protocol_get(hid, &host_protocol);
            if (status != UX_SUCCESS && status != UX_TRANSFER_STALLED)
            {
                printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
                test_control_return(1);
            }
            if (status == UX_SUCCESS && host_protocol != UX_HOST_CLASS_HID_PROTOCOL_BOOT)
            {
                printf("Error on line %d, host get protocol not boot with NULL callback\n", __LINE__);
                test_control_return(1);
            }
        }
        if (g_set_protocol_callback_calls != calls_before)
        {
            printf("Error on line %d, callback should not be invoked when NULL\n", __LINE__);
            test_control_return(1);
        }
    }

    /* Cleanup */
    _ux_device_stack_disconnect();
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);
    UX_PARAMETER_NOT_USED(status);
    _ux_device_stack_uninitialize();
    _ux_system_uninitialize();

    printf("SUCCESS!\n");
    test_control_return(0);
}
