/* This tests the case where the transfer length of the GetHubStatus request is invalid.
   The specific test case is in ux_host_class_hub_status_get.c. */

#include "usbx_ux_test_hub.h"

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_hub_status_get_invalid_length_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Hub Status Get Invalid Length Test...................... ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
    UX_TEST_SETUP get_port_status_setup = {0};
    get_port_status_setup.ux_test_setup_request = UX_HOST_CLASS_HUB_GET_STATUS;
    get_port_status_setup.ux_test_setup_type = UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_OTHER;

    UX_TEST_ACTION get_port_status_match_action = {0};
    get_port_status_match_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    get_port_status_match_action.function = UX_HCD_TRANSFER_REQUEST;
    get_port_status_match_action.req_action = UX_TEST_SETUP_MATCH_REQUEST | UX_TEST_SIM_REQ_ANSWER;
    get_port_status_match_action.req_setup = &get_port_status_setup;
    get_port_status_match_action.req_actual_len = 3; /* valid length is supposed to be 4. */
    get_port_status_match_action.no_return = 0;
    get_port_status_match_action.status = UX_SUCCESS;

    ux_test_add_action_to_main_list(get_port_status_match_action);

    connect_device_to_hub();
    tx_thread_sleep(100);
    ux_test_wait_for_enum_thread_completion();
    UX_TEST_ASSERT(g_dpump_host_from_system_change_function == UX_NULL);
#endif
}

static void post_init_device()
{
}