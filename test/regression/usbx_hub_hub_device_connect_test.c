/* This tests the case where the device connected to the hub is disconnected. 
   The specific test case is in ux_host_class_hub_port_change_connection_process.c. */

#include "usbx_ux_test_hub.h"

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_hub_device_connect_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Device connect Test..................................... ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
UX_DEVICE   *device;
ULONG       temp;

    /* >>>>>>>>>> Test connect and enumerate. */
    g_host_change_count = 0;
    connect_device_to_hub();
    class_dpump_get();

    /* 2 events expected.  */
    UX_TEST_ASSERT(g_host_change_count == 2);

    /* 1st event : dpump class activated.  */
    UX_TEST_ASSERT(g_host_change_logs[0].event == UX_DEVICE_INSERTION);
    UX_TEST_ASSERT(g_host_change_logs[0].class != UX_NULL);
    UX_TEST_ASSERT(g_host_change_logs[0].instance != UX_NULL);

    /* 2nd event : device enumerated (connected).  */
    UX_TEST_ASSERT(g_host_change_logs[1].event == UX_DEVICE_CONNECTION);
    UX_TEST_ASSERT(g_host_change_logs[1].class == UX_NULL);
    UX_TEST_ASSERT(g_host_change_logs[1].instance != UX_NULL);

    /* >>>>>>>>>> Now we need to disconnect the device. */
    g_host_change_count = 0;
    disconnect_device_from_hub();
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_null((VOID**)&g_dpump_host_from_system_change_function));
    ux_test_wait_for_enum_thread_completion();

    /* 2 events expected.  */
    UX_TEST_ASSERT(g_host_change_count == 2);

    /* 1st event : dpump class deactivated.  */
    UX_TEST_ASSERT(g_host_change_logs[0].event == UX_DEVICE_REMOVAL);
    UX_TEST_ASSERT(g_host_change_logs[0].class != UX_NULL);
    UX_TEST_ASSERT(g_host_change_logs[0].instance != UX_NULL);

    /* 2nd event : device disconnected.  */
    UX_TEST_ASSERT(g_host_change_logs[1].event == UX_DEVICE_DISCONNECTION);
    UX_TEST_ASSERT(g_host_change_logs[1].class == UX_NULL);
    UX_TEST_ASSERT(g_host_change_logs[1].instance != UX_NULL);

    /* >>>>>>>>>> Now test case : UX_TOO_MANY_DEVICES.  */
    ux_test_ignore_all_errors();
    g_host_change_count = 0;
    temp = _ux_system_host -> ux_system_host_max_devices;
    _ux_system_host -> ux_system_host_max_devices = 1;
    connect_device_to_hub();

    /* One event expected.  */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&g_host_change_count, 1));

    /* event : device connected but no instance.  */
    UX_TEST_ASSERT(g_host_change_logs[0].event == UX_DEVICE_CONNECTION);
    UX_TEST_ASSERT(g_host_change_logs[0].class == UX_NULL);
    UX_TEST_ASSERT(g_host_change_logs[0].instance == UX_NULL);

    g_host_change_count = 0;
    disconnect_device_from_hub();
    ux_test_wait_for_enum_thread_completion();

    /* There is no change notification since no device instance allocated.  */
    UX_TEST_ASSERT(g_host_change_count == 0);

    /* Restore.  */
    _ux_system_host -> ux_system_host_max_devices = temp;
    ux_test_unignore_all_errors();

    /* >>>>>>>>>> Now test case : UX_NO_CLASS_MATCH.  */
    ux_test_ignore_all_errors();

    /* Unregister DPUMP.  */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_class_unregister(ux_host_class_dpump_entry));
    g_host_change_count = 0;
    connect_device_to_hub();

    /* One event expected.  */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&g_host_change_count, 1));

    /* event : device connected but not configured.  */
    UX_TEST_ASSERT(g_host_change_logs[0].event == UX_DEVICE_CONNECTION);
    UX_TEST_ASSERT(g_host_change_logs[0].class == UX_NULL);
    UX_TEST_ASSERT(g_host_change_logs[0].instance != UX_NULL);
    device = (UX_DEVICE*)g_host_change_logs[0].instance;
    UX_TEST_ASSERT(device->ux_device_state == UX_DEVICE_ADDRESSED);

    g_host_change_count = 0;
    disconnect_device_from_hub();

    /* One event expected.  */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&g_host_change_count, 1));

    ux_test_wait_for_enum_thread_completion();

    /* event : device disconnected but not configured.  */
    UX_TEST_ASSERT(g_host_change_logs[0].event == UX_DEVICE_DISCONNECTION);
    UX_TEST_ASSERT(g_host_change_logs[0].class == UX_NULL);
    UX_TEST_ASSERT(g_host_change_logs[0].instance == (VOID*)device);

    ux_test_unignore_all_errors();

#endif
}

static void post_init_device()
{
}