/* This tests the ux_host_class_hub_status_get.c API. */

#include "usbx_ux_test_hub.h"

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_host_class_hub_status_get_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_host_class_hub_status_get Test........................... ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{

USHORT port_status;
USHORT port_change;

    /** Test sending request to port 0 (hub itself). **/

    /* Make get port status action for returning immediately. */

    UX_TEST_SETUP get_port_status_setup = {0};
    get_port_status_setup.ux_test_setup_request = UX_HOST_CLASS_HUB_GET_STATUS;
    get_port_status_setup.ux_test_setup_type = UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_DEVICE;
    get_port_status_setup.ux_test_setup_index = 0;

    UX_TEST_ACTION get_port_status_match_action = {0};
    get_port_status_match_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    get_port_status_match_action.function = UX_HCD_TRANSFER_REQUEST;
    get_port_status_match_action.req_action = UX_TEST_SETUP_MATCH_REQUEST | UX_TEST_SETUP_MATCH_INDEX;
    get_port_status_match_action.req_setup = &get_port_status_setup;
    get_port_status_match_action.no_return = 1;

    ux_test_add_action_to_main_list(get_port_status_match_action);

    UX_TEST_CHECK_SUCCESS(_ux_host_class_hub_status_get(g_hub_host, 0, &port_status, &port_change));

    UX_TEST_ASSERT(ux_test_check_actions_empty() == UX_TRUE);

    /** Test memory allocation failure. **/

    ux_test_utility_sim_mem_allocate_until_flagged(0, UX_REGULAR_MEMORY);
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_UTILITY, UX_MEMORY_INSUFFICIENT));
    UX_TEST_CHECK_NOT_SUCCESS(_ux_host_class_hub_status_get(g_hub_host, 0, &port_status, &port_change));
}

static void post_init_device()
{
}