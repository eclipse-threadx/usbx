/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

static UCHAR device_is_finished;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_basic_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC ECM Basic Functionality Test............................ ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

    /* Running TCP test. */
    stepinfo("running TCP test.\n");
    cdc_ecm_basic_test(BASIC_TEST_HOST, BASIC_TEST_TCP);

    /* Running UDP test. */
    stepinfo("running UDP test.\n");
    cdc_ecm_basic_test(BASIC_TEST_HOST, BASIC_TEST_UDP);

    /* Wait for device to finish.  */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&device_is_finished, UX_TRUE));

    /* Disconnect.  */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();

    /* Connect with null system change function. */
    _ux_system_host->ux_system_host_change_function = UX_NULL;

    /* Connect. */
    ux_test_connect_slave_and_host_wait_for_enum_completion();

    /* We're done.  */
}

static void post_init_device()
{

    cdc_ecm_basic_test(BASIC_TEST_DEVICE, BASIC_TEST_TCP);
    cdc_ecm_basic_test(BASIC_TEST_DEVICE, BASIC_TEST_UDP);

    device_is_finished = UX_TRUE;
}