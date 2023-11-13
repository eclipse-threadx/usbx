/* This tests the case where the CDC-ECM thread creation fails. */

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
void usbx_cdc_ecm_host_packet_pool_create_fail_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM Host Packet Pool Create Test........................ ");
    printf("Deprecated\n");
    test_control_return(0);
    return;

    stepinfo("\n");

    ux_test_cdc_ecm_initialize_use_framework(first_unused_memory, &device_init_data);
}

static void post_init_host()
{

int i;

    /* Enumerate first so we can initialize memory test. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize, 0, 0));

    /* The HCD init function put()s the HCD semaphore, so we can do this here. */
    ux_test_wait_for_enum_thread_completion();

    /* We want at least one memory test. */
    ux_test_memory_test_initialize();

    /* Disconnect so we can setup test. */
    ux_test_disconnect_host_wait_for_enum_completion();

    UX_TEST_ACTION packet_pool_create_fail_action = {0};
    packet_pool_create_fail_action.usbx_function = UX_TEST_OVERRIDE_NX_PACKET_POOL_CREATE;
    packet_pool_create_fail_action.name_ptr = "host CDC-ECM packet pool";
    packet_pool_create_fail_action.no_return = 0;
    packet_pool_create_fail_action.status = UX_ERROR;

    for (i = 0; i < UX_RH_ENUMERATION_RETRY; i++)
    {
        ux_test_add_action_to_main_list(packet_pool_create_fail_action);
        ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ENUMERATOR, UX_ERROR));
    }
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ROOT_HUB, UX_DEVICE_ENUMERATION_FAILURE));

    /* Start enumeration. */
    ux_test_connect_slave_and_host_wait_for_enum_completion();

    /* Disconnect. Note that this also does the memory check. */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();

    UX_TEST_ASSERT(ux_test_check_actions_empty());
    UX_TEST_ASSERT(cdc_ecm_host_from_system_change_function == UX_NULL);
}

static void post_init_device()
{
}