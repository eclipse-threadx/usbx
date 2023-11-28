/* This tests the case where the host sends a request to the hub itself and
   not a port. */

#include "usbx_ux_test_hub.h"

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_request_to_hub_itself_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Request To Hub Itself Test.............................. ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{

    /* We perform this test by creating an action match the hub device request.
       If it matches, then the request USBX sends is correct. */

    UINT command = UX_SET_FEATURE;
    UINT function = UX_HOST_CLASS_HUB_PORT_POWER;

    UX_TEST_SETUP setup = {0};
    setup.ux_test_setup_request = command;
    setup.ux_test_setup_type = UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_DEVICE;

    UX_TEST_ACTION action = {0};
    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_SETUP_MATCH_REQUEST;
    action.req_setup = &setup;
    action.no_return = 0;
    action.status = UX_SUCCESS;
    ux_test_add_action_to_main_list(action);

    UX_TEST_CHECK_SUCCESS(_ux_host_class_hub_feature(g_hub_host, 0, command, function));

    UX_TEST_ASSERT(ux_test_check_actions_empty() == UX_TRUE);
}

static void post_init_device()
{
}