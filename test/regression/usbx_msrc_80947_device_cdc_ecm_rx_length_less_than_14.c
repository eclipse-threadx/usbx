/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

static UCHAR device_is_finished;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_msrc_80947_device_cdc_ecm_rx_length_less_than_14_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running MSRC 80947 - Device CDC ECM RX length less than 14 Test............ ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void msrc_8947_test(void)
{
NX_PACKET_POOL  *packet_pool;
NX_PACKET       *packet;
UINT            i;

    packet_pool = cdc_ecm_host->ux_host_class_cdc_ecm_packet_pool;
    UX_TEST_ASSERT(packet_pool != UX_NULL);

    /* Simulate a packet with less than 14 bytes data on host side.  */
    for (i = 0; i < 100; i ++)
    {
        ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_CLASS_MALFORMED_PACKET_RECEIVED_ERROR));

        UX_TEST_CHECK_SUCCESS(nx_packet_allocate(packet_pool, &packet, 20, UX_MS_TO_TICK(UX_HOST_CLASS_CDC_ECM_PACKET_POOL_WAIT)));
        *packet->nx_packet_append_ptr ++= 0; packet->nx_packet_length = 1;
        UX_TEST_CHECK_SUCCESS(_ux_host_class_cdc_ecm_write(cdc_ecm_host, packet));
        tx_thread_sleep(1);
    }
}

static void post_init_host()
{

    /* Test MSRC 8947, packet should be discarded service continue.  */
    msrc_8947_test();

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