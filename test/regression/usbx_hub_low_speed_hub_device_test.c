/* This tests the case where the hub device is low speed. The specific test case 
   is in ux_host_class_hub_port_change_connection_process.c. */

#include "usbx_ux_test_hub.h"

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_low_speed_hub_device_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Low Speed Hub Device Test............................... ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
    /* Tell the host that there's a device connection. */
    connect_device_to_hub_speed(UX_HOST_CLASS_HUB_PORT_STATUS_LOW_SPEED);
    class_dpump_get();
    ux_test_wait_for_enum_thread_completion();
#endif
}

static void post_init_device()
{
}