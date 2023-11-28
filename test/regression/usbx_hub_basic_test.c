/* This is a basic test for the host hub class. Note that there is no device hub
   class, so we create a barebones dummy one. We also test a device connected
   to the hub; for this, we use the DPUMP class since it is very simple. Note
   that we only test enumeration of the DPUMP device, since it is rather difficult
   and unnecessary to do test the DPUMP device behind the hub. */

#include "usbx_ux_test_hub.h"

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_basic_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Basic Functionality Test................................ ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{

    /* Let's initialize the memory test. */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();
    ux_test_memory_test_initialize();
    ux_test_connect_slave_and_host_wait_for_enum_completion();
    class_hub_get();
    UX_TEST_ASSERT(g_hub_device != UX_NULL);

    /* Now, let's tell the host that there's a device connection. */
    connect_device_to_hub();
#if UX_MAX_DEVICES > 1
    /* Now wait for the device hub to tell us the DPUMP device has been connected. */
    class_dpump_get();

    /* The host enumerated the DPUMP device. Awesome. Now let's disconnect and reconnect
       to test that we can reconnect. */
    /* Let's also test the case of null change function. */
    _ux_system_host->ux_system_host_change_function = UX_NULL;
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();
    ux_test_connect_slave_and_host_wait_for_enum_completion();
    /* Let's also test the case where the port data is 2 bytes instead of 1. */
    connect_device_to_hub_short_with_hub();
    class_dpump_get();
    /* Let's re-add the system change function.  */
    _ux_system_host->ux_system_host_change_function = system_change_function;

    /* The host enumerated the DPUMP device. Awesome. Now let's disconnect and reconnect
       to test that we can reconnect and stuff. */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();
    /* Ensure everything is cleared. */
    UX_TEST_ASSERT(g_hub_device == UX_NULL);
    UX_TEST_ASSERT(g_hub_host_from_system_change_function == UX_NULL);
    ux_test_connect_slave_and_host_wait_for_enum_completion();
    connect_device_to_hub();
    class_dpump_get();
#endif
    /* We're done. */
}

static void post_init_device()
{
}