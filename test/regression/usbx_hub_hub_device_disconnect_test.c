/* This tests the case where the device connected to the hub is disconnected. 
   The specific test case is in ux_host_class_hub_port_change_connection_process.c. */

#include "usbx_ux_test_hub.h"

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_hub_device_disconnect_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Device Disconnect Test.................................. ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
    /* Now, let's tell the host that there's a device connection, and wait for host to enumerate it. */
    connect_device_to_hub();
    class_dpump_get();

    /* Now we need to disconnect the device. */
    disconnect_device_from_hub();
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_null((VOID**)&g_dpump_host_from_system_change_function));
    ux_test_wait_for_enum_thread_completion();
#endif
}

static void post_init_device()
{
}