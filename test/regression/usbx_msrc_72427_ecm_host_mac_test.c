/* This tests the case where the mac address string is too long. This is an
   error. */

#include "usbx_ux_test_cdc_ecm.h"

static unsigned char invalid_mac_address_string_length[] = {

    /* Manufacturer string descriptor : Index 1 - "Express Logic" */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72, 0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 - "EL CDCECM Device" */
        0x09, 0x04, 0x02, 0x10,
        0x45, 0x4c, 0x20, 0x43, 0x44, 0x43, 0x45, 0x43,
        0x4d, 0x20, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65,

    /* Serial Number string descriptor : Index 3 - "0001" */
        0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31,

    /* MAC Address string descriptor : Index 4 - "001E5841B878" */
        0x09, 0x04, 0x04, 
        0x1, /* This byte is the length of the string. It just needs to be small (look in mac_address_get.c). */
        0x30, 0x30,

};

static DEVICE_INIT_DATA device_init_data = {
    .string_framework = invalid_mac_address_string_length,
    .string_framework_length = sizeof(invalid_mac_address_string_length),
    .dont_register_hcd = 1,
};

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_msrc_72427_ecm_mac_address_invalid_length_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running MSRC 72427 : CDC-ECM Mac Address Invalid Length Test ....... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize_use_framework(first_unused_memory, &device_init_data);
}

static void post_init_host()
{

int i;

    /* We expect this to fail multiple times since enumeration tries multiple times. */
    for (i = 0; i < UX_RH_ENUMERATION_RETRY; i++)
    {

        ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_DESCRIPTOR_CORRUPTED));
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