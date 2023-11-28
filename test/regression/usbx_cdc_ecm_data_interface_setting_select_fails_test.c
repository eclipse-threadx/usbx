/* This tests the case where the setting the data interface with endpoints fails.  */

#include "usbx_ux_test_cdc_ecm.h"

static DEVICE_INIT_DATA device_init_data = {
    .framework = default_device_framework,
    .framework_length = sizeof(default_device_framework),
    .dont_register_hcd = 1,
};

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_data_interface_setting_select_fails_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Data Interface Setting Select Fails Test.................... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize_use_framework(first_unused_memory, &device_init_data);
}

static void post_init_host()
{

int i;

#if 0
    /* Create a transfer_request for the SET_INTERFACE request. No data for this request */
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_SET_INTERFACE;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_index =             (USHORT) interface -> ux_interface_descriptor.bInterfaceNumber;
    transfer_request -> ux_transfer_request_value =             (USHORT) interface -> ux_interface_descriptor.bAlternateSetting;
#endif

    UX_TEST_SETUP alternate_setting_set_setup;
    alternate_setting_set_setup.ux_test_setup_type = UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_INTERFACE;
    alternate_setting_set_setup.ux_test_setup_request = UX_SET_INTERFACE;
    alternate_setting_set_setup.ux_test_setup_value = 1; /* Alternate Setting */
    alternate_setting_set_setup.ux_test_setup_index = 1; /* Interface (data is 1) */

    /* We need to intercept the configuration descriptor. */
    UX_TEST_ACTION alternate_setting_fail_action = {0};
    alternate_setting_fail_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    alternate_setting_fail_action.function = UX_HCD_TRANSFER_REQUEST;
    alternate_setting_fail_action.req_action = UX_TEST_SETUP_MATCH_REQUEST;
    alternate_setting_fail_action.req_setup = &alternate_setting_set_setup;
    alternate_setting_fail_action.status = UX_ERROR;
    alternate_setting_fail_action.no_return = 0;

    /* We expect this to fail multiple times since enumeration tries multiple times. */
    for (i = 0; i < UX_RH_ENUMERATION_RETRY; i++)
    {
        ux_test_add_action_to_main_list(alternate_setting_fail_action);
    }
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ROOT_HUB, UX_DEVICE_ENUMERATION_FAILURE));

    /* Enumerate. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize, 0, 0));

    /* The HCD init function put()s the HCD semaphore, so we can do this here. */
    ux_test_wait_for_enum_thread_completion();

    /* Enumeration should've failed.  */
    UX_TEST_ASSERT(ux_test_check_actions_empty());
    UX_TEST_ASSERT(cdc_ecm_host_from_system_change_function == UX_NULL);
}

static void post_init_device()
{
}