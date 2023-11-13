/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_device_class_dummy.h"
#include "ux_host_class_dummy.h"
#include "ux_test_hcd_sim_host.h"

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
void usbx_uxe_system_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_system APIs Test........................................ ");
#if !defined(UX_DEVICE_CLASS_AUDIO_ENABLE_ERROR_CHECKING)
#warning Tests skipped due to compile option!
    printf("SKIP SUCCESS!\n");
    test_control_return(0);
    return;
#endif

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);
#if 1
    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
#endif
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
UCHAR                                  *buffer = (UCHAR *)&status;
UX_HOST_CLASS                          dummy_class;
UX_HOST_CLASS                          *dummy_class_ptr = &dummy_class;
UX_DEVICE                              dummy_device;
UX_DEVICE                              *dummy_device_ptr = &dummy_device;
UX_CONFIGURATION                       dummy_configuration;
UX_CONFIGURATION                       *dummy_configuration_ptr = &dummy_configuration;
UX_INTERFACE                           dummy_interface;
UX_INTERFACE                           *dummy_interface_ptr = &dummy_interface;
UX_ENDPOINT                            dummy_endpoint;
UX_ENDPOINT                            *dummy_endpoint_ptr = &dummy_endpoint;
UX_TRANSFER                            dummy_transfer;

    /* ux_host_stack_class_get()  */
    status =  ux_host_stack_class_get(UX_NULL, &dummy_class_ptr);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status =  ux_host_stack_class_get("dummy", UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_class_instance_get()  */
    status =  ux_host_stack_class_instance_get(UX_NULL, 0, (VOID**)buffer);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status =  ux_host_stack_class_instance_get(&dummy_class, 0, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_class_register()  */
    status =  ux_host_stack_class_register(UX_NULL, _ux_host_class_dummy_entry);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status =  ux_host_stack_class_register("dummy", UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_configuration_interface_get()  */
    status =  ux_host_stack_configuration_interface_get(UX_NULL, 0, 0, &dummy_interface_ptr);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status =  ux_host_stack_configuration_interface_get(&dummy_configuration, 0, 0, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_device_configuration_activate()  */
    status =  ux_host_stack_device_configuration_activate(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_device_configuration_deactivate()  */
    status =  ux_host_stack_device_configuration_deactivate(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_device_configuration_get()  */
    status =  ux_host_stack_device_configuration_get(UX_NULL, 0, &dummy_configuration_ptr);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status =  ux_host_stack_device_configuration_get(&dummy_device, 0, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_device_get()  */
    status =  ux_host_stack_device_get(0, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_device_string_get()  */
    status =  ux_host_stack_device_string_get(UX_NULL, buffer, 64, 0x1234, 1);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status =  ux_host_stack_device_string_get(&dummy_device, UX_NULL, 64, 0x1234, 1);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status =  ux_host_stack_device_string_get(&dummy_device, buffer, 0, 0x1234, 1);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_endpoint_transfer_abort()  */
    status =  ux_host_stack_endpoint_transfer_abort(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_hcd_register()  */
    status =  ux_host_stack_hcd_register(UX_NULL, _ux_test_hcd_sim_host_initialize, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status =  ux_host_stack_hcd_register("dummy", UX_NULL, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_hcd_unregister()  */
    status =  ux_host_stack_hcd_unregister(UX_NULL, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_interface_endpoint_get()  */
    status =  ux_host_stack_interface_endpoint_get(UX_NULL, 0, &dummy_endpoint_ptr);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status =  ux_host_stack_interface_endpoint_get(&dummy_interface, 0, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_interface_setting_select()  */
    status =  ux_host_stack_interface_setting_select(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_stack_transfer_request()  */
    status =  ux_host_stack_transfer_request(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_transfer.ux_transfer_request_endpoint = UX_NULL;
    status =  ux_host_stack_transfer_request(&dummy_transfer);
    UX_TEST_CHECK_CODE(UX_ENDPOINT_HANDLE_UNKNOWN, status);
    dummy_transfer.ux_transfer_request_endpoint = &dummy_endpoint;
    dummy_endpoint.ux_endpoint_device = UX_NULL;
    status =  ux_host_stack_transfer_request(&dummy_transfer);
    UX_TEST_CHECK_CODE(UX_DEVICE_HANDLE_UNKNOWN, status);
#if UX_MAX_HCD > 1
    dummy_endpoint.ux_endpoint_device = &dummy_device;
    dummy_device.ux_device_hcd = UX_NULL;
    status =  ux_host_stack_transfer_request(&dummy_transfer);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
#endif

#if defined(UX_HOST_STANDALONE)

    /* ux_host_stack_transfer_run()  */
    status =  ux_host_stack_transfer_run(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_transfer.ux_transfer_request_endpoint = UX_NULL;
    status =  ux_host_stack_transfer_run(&dummy_transfer);
    UX_TEST_CHECK_CODE(UX_ENDPOINT_HANDLE_UNKNOWN, status);
    dummy_transfer.ux_transfer_request_endpoint = &dummy_endpoint;
    dummy_endpoint.ux_endpoint_device = UX_NULL;
    status =  ux_host_stack_transfer_run(&dummy_transfer);
    UX_TEST_CHECK_CODE(UX_DEVICE_HANDLE_UNKNOWN, status);
#if UX_MAX_HCD > 1
    dummy_endpoint.ux_endpoint_device = &dummy_device;
    dummy_device.ux_device_hcd = UX_NULL;
    status =  ux_host_stack_transfer_run(&dummy_transfer);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
#endif

#endif

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
