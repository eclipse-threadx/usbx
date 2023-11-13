/* This tests the case where the hub reports a port OVER_CURRENT change. The specific 
   test case is in ux_host_class_hub_port_change_process.c. */

#include "usbx_ux_test_hub.h"

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_port_change_over_current_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Port Change Over Current Test........................... ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
    /* We expect USBX to report an error. */
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HUB, UX_OVER_CURRENT_CONDITION));

    /* Send port change enable to host. */
    set_and_send_port_event(0, 
                            UX_HOST_CLASS_HUB_PORT_CHANGE_OVER_CURRENT);

    /* Wait for enum thread to complete. */
    tx_thread_sleep(100);
    ux_test_wait_for_enum_thread_completion();

    UX_TEST_ASSERT(ux_test_check_actions_empty() == UX_TRUE);

    /* Ensure the port enable was cleared. */
    UX_TEST_ASSERT((g_hub_device->port_change & UX_HOST_CLASS_HUB_PORT_CHANGE_OVER_CURRENT) == 0);
#endif
}

static void post_init_device()
{
}