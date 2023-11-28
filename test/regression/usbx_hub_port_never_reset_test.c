/* This tests the case where the port reset fails because the hub never reports that the port was RESET. 
   The specific test case is in ux_host_class_hub_port_reset.c. */

#include "usbx_ux_test_hub.h"

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_port_reset_fails_due_to_unset_port_enabled_bit_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Port Reset Fails Due To Unset Port Enabled Bit Test..... ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
    /* Make sure it is never reset. */
    g_hub_device->dont_reset_port_when_commanded_to = 1;
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HUB, UX_PORT_RESET_FAILED));
    connect_device_to_hub();
    /* Wait for enum thread to pick it up. */
    tx_thread_sleep(100);
    ux_test_wait_for_enum_thread_completion();
    UX_TEST_ASSERT(g_dpump_host_from_system_change_function == UX_NULL);

    /* USBX still thinks there's a device connected (since there is, technically), so it will try to remove it, but fail. */
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ENUMERATOR, UX_DEVICE_HANDLE_UNKNOWN));
#endif
}

static void post_init_device()
{
}