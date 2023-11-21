/* This tests the case where the hub reports a port ENABLE change. The specific 
   test case is in ux_host_class_hub_port_change_process.c. */

#include "usbx_ux_test_hub.h"

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_port_change_enable_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Port Change Enable Test................................. ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
    /* Send port change enable to host. */
    set_and_send_port_event(0, 
                            UX_HOST_CLASS_HUB_PORT_CHANGE_ENABLE);

    /* Wait for enum thread to complete. */
    tx_thread_sleep(100);
    ux_test_wait_for_enum_thread_completion();

    /* Ensure the port enable was cleared. */
    UX_TEST_ASSERT((g_hub_device->port_change & UX_HOST_CLASS_HUB_PORT_CHANGE_ENABLE) == 0);
#endif
}

static void post_init_device()
{
}