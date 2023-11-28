/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_host_class_hid.h"
#include "ux_host_class_hid_keyboard.h"
#include "ux_host_class_hid_mouse.h"
#include "ux_host_class_hid_remote_control.h"

#include "ux_test.h"

/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_MEMORY_SIZE     (64*1024)


/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           error_callback_ignore = UX_FALSE;
static ULONG                           error_callback_counter;


/* Define USBX test global variables.  */


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static TX_THREAD           ux_test_thread_simulation_0;
static TX_THREAD           ux_test_thread_simulation_1;
static void                ux_test_thread_simulation_0_entry(ULONG);
static void                ux_test_thread_simulation_1_entry(ULONG);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_uxe_device_hid_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_device_hid APIs Test.................................... ");
#if !defined(UX_DEVICE_CLASS_HID_ENABLE_ERROR_CHECKING)             ||\
    !defined(UX_DEVICE_CLASS_HID_KEYBOARD_ENABLE_ERROR_CHECKING)    ||\
    !defined(UX_DEVICE_CLASS_HID_MOUSE_ENABLE_ERROR_CHECKING)       ||\
    !defined(UX_DEVICE_CLASS_HID_REMOTE_CONTROL_ENABLE_ERROR_CHECKING)
#warning Tests skipped due to compile option!
    printf("SKIP SUCCESS!\n");
    test_control_return(0);
    return;
#endif

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_simulation_0, "test simulation", ux_test_thread_simulation_0_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}


static void  ux_test_thread_simulation_0_entry(ULONG arg)
{
UINT                                    status;
UCHAR                                   *dummy_name = "dummy";
UX_HOST_CLASS_HID                       dummy_hid;
UX_HOST_CLASS_HID_KEYBOARD              dummy_keyboard;
UX_HOST_CLASS_HID_MOUSE                 dummy_mouse;
UX_HOST_CLASS_HID_REMOTE_CONTROL        dummy_remote_control;

UX_HOST_CLASS_HID_REPORT_CALLBACK       dummy_hid_report_callback;
UX_HOST_CLASS_HID_REPORT_GET_ID         dummy_get_id;
UX_HOST_CLASS_HID_CLIENT_REPORT         dummy_client_report;
USHORT                                  dummy_word;
ULONG                                   dummy_dw1, dummy_dw2;
SLONG                                   dummy_sl1, dummy_sl2;


    /* ux_host_class_hid_client_register()  */
    status = ux_host_class_hid_client_register(UX_NULL, ux_host_class_hid_keyboard_entry);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_client_register(dummy_name, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_hid_idle_get()  */
    status = ux_host_class_hid_idle_get(UX_NULL, &dummy_word, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_idle_get(&dummy_hid, UX_NULL, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_hid_idle_set()  */
    status = ux_host_class_hid_idle_set(UX_NULL, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_hid_periodic_report_start()  */
    status = ux_host_class_hid_periodic_report_start(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_hid_periodic_report_stop()  */
    status = ux_host_class_hid_periodic_report_stop(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_hid_report_callback_register()  */
    status = ux_host_class_hid_report_callback_register(UX_NULL, &dummy_hid_report_callback);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_report_callback_register(&dummy_hid, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_hid_report_id_get()  */
    status = ux_host_class_hid_report_id_get(UX_NULL, &dummy_get_id);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_report_id_get(&dummy_hid, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_hid_report_get()  */
    status = ux_host_class_hid_report_get(UX_NULL, &dummy_client_report);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_report_get(&dummy_hid, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_hid_report_set()  */
    status = ux_host_class_hid_report_set(UX_NULL, &dummy_client_report);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_report_set(&dummy_hid, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

#if 0//defined(UX_HOST_STANDALONE)

    /* ux_host_class_hid_idle_set_run()  */
    status = ux_host_class_hid_idle_set_run(UX_NULL, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_hid_report_set_run()  */
    status = ux_host_class_hid_report_set_run(UX_NULL, &dummy_client_report);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_report_set_run(&dummy_hid, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
#endif


    /* ux_host_class_hid_keyboard_key_get()  */
    status = ux_host_class_hid_keyboard_key_get(UX_NULL, &dummy_dw1, &dummy_dw2);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_keyboard_key_get(&dummy_keyboard, UX_NULL, &dummy_dw2);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_keyboard_key_get(&dummy_keyboard, &dummy_dw1, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_keyboard_key_get(&dummy_keyboard, &dummy_dw1, &dummy_dw1);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_hid_keyboard_ioctl()  */
    status = ux_host_class_hid_keyboard_ioctl(UX_NULL, 0, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);


    /* ux_host_class_hid_mouse_buttons_get()  */
    status = ux_host_class_hid_mouse_buttons_get(UX_NULL, &dummy_dw1);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_mouse_buttons_get(&dummy_mouse, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* _uxe_host_class_hid_mouse_position_get()  */
    status = _uxe_host_class_hid_mouse_position_get(UX_NULL, &dummy_sl1, &dummy_sl2);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = _uxe_host_class_hid_mouse_position_get(&dummy_mouse, UX_NULL, &dummy_sl2);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = _uxe_host_class_hid_mouse_position_get(&dummy_mouse, &dummy_sl1, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = _uxe_host_class_hid_mouse_position_get(&dummy_mouse, &dummy_sl1, &dummy_sl1);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_hid_mouse_wheel_get()  */
    status = ux_host_class_hid_mouse_wheel_get(UX_NULL, &dummy_sl1);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_mouse_wheel_get(&dummy_mouse, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);


    /* ux_host_class_hid_remote_control_usage_get()  */
    status = ux_host_class_hid_remote_control_usage_get(UX_NULL, &dummy_dw1, &dummy_dw2);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_remote_control_usage_get(&dummy_remote_control, UX_NULL, &dummy_dw2);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_remote_control_usage_get(&dummy_remote_control, &dummy_dw1, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_hid_remote_control_usage_get(&dummy_remote_control, &dummy_dw1, &dummy_dw1);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);


    /* Sleep for a tick to make sure everything is complete.  */
    tx_thread_sleep(1);

    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* Test error.  */
        printf("ERROR #%d: total %ld errors\n", __LINE__, error_counter);
        test_control_return(1);
    }
    else
    {

        /* Successful test.  */
        printf("SUCCESS!\n");
        test_control_return(0);
    }
}
