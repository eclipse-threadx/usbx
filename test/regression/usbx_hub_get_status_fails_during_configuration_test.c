/* This tests the case where the GET_STATUS request has an invalid actual length. */

#include "usbx_ux_test_hub.h"

static DEVICE_INIT_DATA device_init_data = {
    .dont_enumerate = 1
};

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_get_status_fails_during_configuration_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Get Status Fails During Configuration Test.............. ");

    stepinfo("\n");

    initialize_hub_with_device_init_data(first_unused_memory, &device_init_data);
}

static void post_init_host()
{

int i;

    /* Setup setup for matching the GET_STATUS request. */
    UX_TEST_SETUP setup = {0};
    setup.ux_test_setup_request = UX_GET_STATUS;
    setup.ux_test_setup_type = UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    setup.ux_test_setup_value = 0;
    setup.ux_test_setup_index = 0;

    /* Make action to make transfer fail. */
    UX_TEST_ACTION action = {0};
    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_SETUP_MATCH_REQUEST | UX_TEST_SIM_REQ_ANSWER;
    action.req_setup = &setup;
    action.req_ep_address = 0x81;
    action.req_actual_len = 1;
    action.no_return = 0;
    action.status = UX_SUCCESS;

    /* We expect error to happen multiple times. */
    for (i = 0; i < UX_RH_ENUMERATION_RETRY; i++)
    {
        ux_test_add_action_to_main_list(action);
        ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HUB, UX_CONNECTION_INCOMPATIBLE));
    }
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ROOT_HUB, UX_DEVICE_ENUMERATION_FAILURE));

    /* Enumerate. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize, 0, 0));
    ux_test_wait_for_enum_thread_completion();

    /* Ensure all expected actions occurred. */
    UX_TEST_ASSERT(ux_test_check_actions_empty());
    UX_TEST_ASSERT(g_hub_host_from_system_change_function == UX_NULL);
}

static void post_init_device()
{
}