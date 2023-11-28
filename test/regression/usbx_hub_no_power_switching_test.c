/* This tests the case where the hub has no power switching. Specific test case
   is in ux_host_class_hub_ports_power.c. */

#include "usbx_ux_test_hub.h"

static unsigned char hub_descriptor_no_power_switching[] = {

    /* Hub Descriptor */
    0x09, /* bLength */
    0x29, /* bDescriptorType */
    0x02, /* bNbrPorts */
    0x09 | UX_HOST_CLASS_HUB_NO_POWER_SWITCHING, 0x00, /* wHubCharacteristics - no power switching */
    0x32, /* bPwrOn2PwrGood */
    0x01, /* bHubContrCurrent */
    0x00, /* DeviceRemovable */
    0xff, /* PortPwrCtrlMask */

};

static DEVICE_INIT_DATA device_init_data = {
    .dont_enumerate = 1,
    .hub_descriptor = hub_descriptor_no_power_switching,
    .hub_descriptor_length = sizeof(hub_descriptor_no_power_switching),
};

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_no_power_switching_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub No Power Switching Test................................. ");

    stepinfo("\n");

    initialize_hub_with_device_init_data(first_unused_memory, &device_init_data);
}

static void post_init_host()
{

    /* Add action to match PORT_POWER request. We shouldn't match it, since USBX should never send it. */

    UX_TEST_SETUP port_power_setup = {0};
    port_power_setup.ux_test_setup_request = UX_SET_FEATURE;
    port_power_setup.ux_test_setup_type = UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_OTHER;
    port_power_setup.ux_test_setup_value = UX_HOST_CLASS_HUB_PORT_POWER;

    UX_TEST_ACTION port_power_match_action = {0};
    port_power_match_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    port_power_match_action.function = UX_HCD_TRANSFER_REQUEST;
    port_power_match_action.req_action = UX_TEST_SETUP_MATCH_REQUEST | UX_TEST_SETUP_MATCH_VALUE;
    port_power_match_action.req_setup = &port_power_setup;
    port_power_match_action.no_return = 1;

    ux_test_add_action_to_main_list(port_power_match_action);

    UX_TEST_CHECK_SUCCESS(ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize, 0, 0));
    ux_test_wait_for_enum_thread_completion();

    /* Make sure the action ws not matched. */
    UX_TEST_ASSERT(ux_test_check_actions_empty() == UX_FALSE);

    /* Good. Now clear the actions. */
    ux_test_clear_main_list_actions();
}

static void post_init_device()
{
}