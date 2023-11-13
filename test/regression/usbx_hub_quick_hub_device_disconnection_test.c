/* This tests the case where the hub reports a device disconnection even though
   no device was previously on that port. This can happen if the device is connected
   then quickly disconnected, before USBX processes the connection. The specific 
   test case is in ux_host_class_hub_port_change_connection_process.c. */

#include "usbx_ux_test_hub.h"

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_quick_hub_device_disconnection_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Quick Hub Device Disconnection Test..................... ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{

    /* We test this by just reporting a disconnection when no device has been
       connected - USBX won't know the difference! */

    disconnect_device_from_hub();
    /* Wait for enum thread to detect it. */
    tx_thread_sleep(100);
    ux_test_wait_for_enum_thread_completion();
}

static void post_init_device()
{
}