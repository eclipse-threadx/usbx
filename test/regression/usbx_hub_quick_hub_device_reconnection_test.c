/* This tests the case where the device is connected, disconnected, then reconnected
   very quickly (in other words, so fast that the HCD thread doesn't detect the
   disconnection). The specific test case is in ux_host_class_hub_port_change_connection_process.c. */

#include "usbx_ux_test_hub.h"

static UINT num_insertions;
static UINT num_removals;

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_quick_hub_device_reconnection_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Quick Hub Device Reconnection Test...................... ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static UINT my_system_change_function(ULONG event, UX_HOST_CLASS *class, VOID *instance)
{

    if (event == UX_DEVICE_INSERTION)
    {
        if (!memcmp(class->ux_host_class_name, "ux_host_class_dpump", strlen("ux_host_class_dpump")))
        {
            num_insertions++;
        }
    }
    else if (event == UX_DEVICE_REMOVAL)
    {
        if (!memcmp(class->ux_host_class_name, "ux_host_class_dpump", strlen("ux_host_class_dpump")))
        {
            num_removals++;
        }
    }
    return(UX_SUCCESS);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
    /* We just test this by connecting the device twice with no disconnection. */

    /* Tell the host that there's a device connection, and wait for host to enumerate it. */
    connect_device_to_hub();
    class_dpump_get();

    _ux_system_host->ux_system_host_change_function = my_system_change_function;

    /* Now do it again. */
    connect_device_to_hub();

    /* Wait for the removal. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uint(&num_removals, 1));

    /* Wait for reconnection. */
    ux_test_wait_for_enum_thread_completion();

    /* Ensure we're all good. */
    UX_TEST_ASSERT(num_insertions == 1);
    class_dpump_get();
#endif
}

static void post_init_device()
{
}