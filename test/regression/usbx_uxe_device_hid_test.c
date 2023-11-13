/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_device_class_hid.h"

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
#if !defined(UX_DEVICE_CLASS_HID_ENABLE_ERROR_CHECKING)
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
UX_SLAVE_CLASS_HID                      *dummy_hid_ptr;
UX_SLAVE_CLASS_HID                      dummy_hid_inst;
UX_SLAVE_CLASS_HID_EVENT                dummy_event;
UX_DEVICE_CLASS_HID_RECEIVED_EVENT      dummy_rx_event;
UCHAR                                   dummy_buffer[32];
ULONG                                   dummy_length;
UX_SLAVE_CLASS_HID_PARAMETER            dummy_parameter;
UX_SLAVE_CLASS                          dummy_class_array[2];

    /* Initialize struct for register ...  */
    ux_utility_memory_set(dummy_class_array, 0, sizeof(dummy_class_array));
    _ux_system_slave -> ux_system_slave_class_array = dummy_class_array;
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    UX_SYSTEM_DEVICE_MAX_CLASS_SET(1);
#endif

    /* Device HID parameters.  */
    dummy_parameter.ux_device_class_hid_parameter_report_address = UX_NULL;
    dummy_parameter.ux_device_class_hid_parameter_report_length  = 10;
    status = ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                              1, 1, &dummy_parameter);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_parameter.ux_device_class_hid_parameter_report_address = dummy_buffer;
    dummy_parameter.ux_device_class_hid_parameter_report_length  = 0;
    status = ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                              1, 1, &dummy_parameter);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_parameter.ux_device_class_hid_parameter_report_address = dummy_buffer;
    dummy_parameter.ux_device_class_hid_parameter_report_length  = UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH + 1;
    status = ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                              1, 1, &dummy_parameter);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_hid_event_set()  */
    status = ux_device_class_hid_event_set(UX_NULL, &dummy_event);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_hid_event_set(&dummy_hid_inst, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_hid_event_get()  */
    status = ux_device_class_hid_event_get(UX_NULL, &dummy_event);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_hid_event_get(&dummy_hid_inst, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_hid_protocol_get()  */
    dummy_hid_ptr = UX_NULL;
    status = ux_device_class_hid_protocol_get(dummy_hid_ptr);
    UX_TEST_CHECK_CODE(UX_ERROR, status);

#if !defined(UX_DEVICE_STANDALONE)
    /* ux_device_class_hid_read()  */
    status = ux_device_class_hid_read(UX_NULL, dummy_buffer, 8, &dummy_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_hid_read(&dummy_hid_inst, UX_NULL, 8, &dummy_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_hid_read(&dummy_hid_inst, dummy_buffer, 0, &dummy_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_hid_read(&dummy_hid_inst, dummy_buffer, 8, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

#else
    /* ux_device_class_hid_read_run()  */
    status = ux_device_class_hid_read_run(UX_NULL, dummy_buffer, 8, &dummy_length);
    UX_TEST_CHECK_CODE(UX_STATE_ERROR, status);
    status = ux_device_class_hid_read_run(&dummy_hid_inst, UX_NULL, 8, &dummy_length);
    UX_TEST_CHECK_CODE(UX_STATE_ERROR, status);
    status = ux_device_class_hid_read_run(&dummy_hid_inst, dummy_buffer, 0, &dummy_length);
    UX_TEST_CHECK_CODE(UX_STATE_ERROR, status);
    status = ux_device_class_hid_read_run(&dummy_hid_inst, dummy_buffer, 8, UX_NULL);
    UX_TEST_CHECK_CODE(UX_STATE_ERROR, status);

#endif

    /* _uxe_device_class_hid_receiver_event_get()  */
    status = _uxe_device_class_hid_receiver_event_get(UX_NULL, &dummy_rx_event);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = _uxe_device_class_hid_receiver_event_get(&dummy_hid_inst, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_hid_receiver_event_free()  */
    status = ux_device_class_hid_receiver_event_free(UX_NULL);
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
