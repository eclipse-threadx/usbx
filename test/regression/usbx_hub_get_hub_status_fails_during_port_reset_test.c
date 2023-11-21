/* This tests the case where the get hub status commmand fails when a port is being reset. 
   The specific test case is in ux_host_class_hub_port_reset.c. */

#include "usbx_ux_test_hub.h"

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_get_hub_status_fails_during_port_reset_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Get Hub Status Fails During Port Reset Test............. ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
    /* Make get port status fail action. */

    UX_TEST_SETUP get_port_status_setup = {0};
    get_port_status_setup.ux_test_setup_request = UX_HOST_CLASS_HUB_GET_STATUS;
    get_port_status_setup.ux_test_setup_type = UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_OTHER;

    UX_TEST_ACTION get_port_status_fail_action = {0};
    get_port_status_fail_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    get_port_status_fail_action.function = UX_HCD_TRANSFER_REQUEST;
    get_port_status_fail_action.req_action = UX_TEST_SETUP_MATCH_REQUEST;
    get_port_status_fail_action.req_setup = &get_port_status_setup;
    get_port_status_fail_action.no_return = 0;
    get_port_status_fail_action.status = UX_ERROR;

    /* The host actually gets the port status three times - first is initial
       port status check, second is during port reset (the one we want to fail), third is necessary since 
       we retry enumeration. */
    UX_TEST_ACTION get_port_status_match_action = {0};
    get_port_status_match_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    get_port_status_match_action.function = UX_HCD_TRANSFER_REQUEST;
    get_port_status_match_action.req_action = UX_TEST_SETUP_MATCH_REQUEST;
    get_port_status_match_action.req_setup = &get_port_status_setup;
    get_port_status_match_action.no_return = 1;

    /* Add them. */
    ux_test_add_action_to_main_list(get_port_status_match_action);
    ux_test_add_action_to_main_list(get_port_status_fail_action);

    /* Tell the host that there's a device connection. */
    connect_device_to_hub();

    /* Wait for empty actions. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_empty_actions());
    ux_test_wait_for_enum_thread_completion();

    /* Ensure it failed. */
    UX_TEST_ASSERT(g_dpump_host_from_system_change_function == UX_NULL);

    /* USBX still thinks there's a device connected (since there is, technically), so it will try to remove it, but fail. */
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ENUMERATOR, UX_DEVICE_HANDLE_UNKNOWN));
#endif
}

static void post_init_device()
{
}