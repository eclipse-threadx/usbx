/* This tests the ux_host_class_cdc_ecm_mac_address_get function. */

#include "usbx_ux_test_cdc_ecm.h"

static UCHAR cdc_ecm_thread_suspended;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_host_class_cdc_ecm_mac_address_get_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_host_class_cdc_ecm_mac_address_get Test.................. ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

#define CONFIGURATION_INDEX 0
static UX_TEST_SETUP get_cfg_desc_setup = {
    .ux_test_setup_type = UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE,
    .ux_test_setup_request = UX_GET_DESCRIPTOR,
    .ux_test_setup_value = (USHORT) CONFIGURATION_INDEX | (UX_CONFIGURATION_DESCRIPTOR_ITEM << 8),
    .ux_test_setup_index = 0,
};

static void post_init_host()
{

int i;

    /* Create the matching one. */
    UX_TEST_ACTION get_cfg_desc_match_action = {0};
    get_cfg_desc_match_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    get_cfg_desc_match_action.function = UX_HCD_TRANSFER_REQUEST;
    get_cfg_desc_match_action.req_action = UX_TEST_SETUP_MATCH_REQUEST | UX_TEST_SETUP_MATCH_VALUE | UX_TEST_SETUP_MATCH_INDEX;
    get_cfg_desc_match_action.req_setup = &get_cfg_desc_setup;
    get_cfg_desc_match_action.no_return = 1;

    /* Create the action that modifies the length. */
    UX_TEST_ACTION get_cfg_desc_modify_length_action = {0};
    get_cfg_desc_modify_length_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    get_cfg_desc_modify_length_action.function = UX_HCD_TRANSFER_REQUEST;
    get_cfg_desc_modify_length_action.req_action = UX_TEST_SETUP_MATCH_REQUEST | UX_TEST_SETUP_MATCH_VALUE | UX_TEST_SETUP_MATCH_INDEX | UX_TEST_SIM_REQ_ANSWER;
    get_cfg_desc_modify_length_action.req_setup = &get_cfg_desc_setup;
    get_cfg_desc_modify_length_action.req_actual_len = 0xff;
    get_cfg_desc_modify_length_action.no_return = 0;
    get_cfg_desc_modify_length_action.status = UX_SUCCESS;

    /** Test configuration descriptor invalid transfer length returned. **/

    /* Since this happens during enumeration, disconnect. */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();

    /* In theory, this should be the third time we get the config desc (the first two
       happen during enumeration), so we need two matching, and one breaking. */
    for (i = 0; i < UX_RH_ENUMERATION_RETRY; i++)
    {

        ux_test_add_action_to_main_list(get_cfg_desc_match_action);
        ux_test_add_action_to_main_list(get_cfg_desc_match_action);
        ux_test_add_action_to_main_list(get_cfg_desc_modify_length_action);
        ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_DESCRIPTOR_CORRUPTED));
    }
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ROOT_HUB, UX_DEVICE_ENUMERATION_FAILURE));

    /* Now connect so that mac_address_get runs. */
    ux_test_connect_slave_and_host_wait_for_enum_completion();

    UX_TEST_ASSERT(ux_test_check_actions_empty());

    /* Disconnect for next test. */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();

    /** Test whole configuration descriptor invalid transfer length returned - note that device should still be disconnected from previous test. **/

    /* In theory, this should be the fourth time we get the config desc (the first two
       happen during enumeration, and the third in mac_address_get), so we need three matching, and one breaking. */
    for (i = 0; i < UX_RH_ENUMERATION_RETRY; i++)
    {

        ux_test_add_action_to_main_list(get_cfg_desc_match_action);
        ux_test_add_action_to_main_list(get_cfg_desc_match_action);
        ux_test_add_action_to_main_list(get_cfg_desc_match_action);
        ux_test_add_action_to_main_list(get_cfg_desc_modify_length_action);
        ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_DESCRIPTOR_CORRUPTED));
    }
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ROOT_HUB, UX_DEVICE_ENUMERATION_FAILURE));

    /* Now connect so that mac_address_get runs. */
    ux_test_connect_slave_and_host_wait_for_enum_completion();

    UX_TEST_ASSERT(ux_test_check_actions_empty());
}

static void post_init_device()
{
}