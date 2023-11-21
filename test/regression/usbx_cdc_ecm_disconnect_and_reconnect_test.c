/* This test ensures everything works after a disconnection and reconnection. */

#include "usbx_ux_test_cdc_ecm.h"

static UCHAR host_ready_for_test_after_reconnection;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_disconnect_and_reconnect_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM Disconnect And Reconnect Test....................... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

    cdc_ecm_basic_test(BASIC_TEST_HOST, BASIC_TEST_TCP);

    /* Now disconnect. */
    ux_test_disconnect_host_wait_for_enum_completion();

    /* Now reconnect. */
    ux_test_connect_host_wait_for_enum_completion();

    /* Get class instance. */
    class_cdc_ecm_get_host();

    /* Tell device we're ready. */
    host_ready_for_test_after_reconnection = 1;

    /* Now run the basic test again. */
    cdc_ecm_basic_test(BASIC_TEST_HOST, BASIC_TEST_TCP);
}

static void post_init_device()
{

    cdc_ecm_basic_test(BASIC_TEST_DEVICE, BASIC_TEST_TCP);

    /* Now wait for the host to tell us to run the test again. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&host_ready_for_test_after_reconnection, 1));

    cdc_ecm_basic_test(BASIC_TEST_DEVICE, BASIC_TEST_TCP);
}