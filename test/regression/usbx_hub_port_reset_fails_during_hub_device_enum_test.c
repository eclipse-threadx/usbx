/* This tests the case where the port reset commmand fails 
   when a device connected to the hub is being enumerated. The specific test case 
   is in ux_host_class_hub_port_change_connection_process.c. */

#include "usbx_ux_test_hub.h"

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_port_reset_fails_during_hub_device_enumeration_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Port Reset Fails During Hub Device Enumeration Test..... ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
    /* Make port reset fail action. */

    UX_TEST_SETUP port_reset_setup = {0};
    port_reset_setup.ux_test_setup_request = UX_SET_FEATURE;
    port_reset_setup.ux_test_setup_type = UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_OTHER;

    UX_TEST_ACTION port_reset_fail_action = {0};
    port_reset_fail_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    port_reset_fail_action.function = UX_HCD_TRANSFER_REQUEST;
    port_reset_fail_action.req_action = UX_TEST_SETUP_MATCH_REQUEST;
    port_reset_fail_action.req_setup = &port_reset_setup;
    port_reset_fail_action.no_return = 0;
    port_reset_fail_action.status = UX_ERROR;

    /* Add them. */
    ux_test_add_action_to_main_list(port_reset_fail_action);

    /* Tell the host that there's a device connection. */
    connect_device_to_hub();

    /* Wait for empty actions. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_empty_actions());
    ux_test_wait_for_enum_thread_completion();

    /* When enumeration fails, USBX still reports the device on the port, so it
       will try to remove the device when the the upper-test-layer disconnects the hub.
       This will result in an error that we want to ignore. */
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ENUMERATOR, UX_DEVICE_HANDLE_UNKNOWN));
#endif
}

static void post_init_device()
{
}