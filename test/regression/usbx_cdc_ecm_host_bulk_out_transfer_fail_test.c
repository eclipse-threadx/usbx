/* This tests the case where the bulk out transfer fails (in the cdc_ecm_write). */

#include "usbx_ux_test_cdc_ecm.h"

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_host_bulk_out_transfer_fail_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM Host Bulk Out Transfer Fail Test.................... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

    /* Create the action for having bulk in transfer fail. */
    UX_TEST_ACTION bulk_transfer_fail_action = {0};
    bulk_transfer_fail_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    bulk_transfer_fail_action.function = UX_HCD_TRANSFER_REQUEST;
    bulk_transfer_fail_action.req_action = UX_TEST_MATCH_EP;
    bulk_transfer_fail_action.req_ep_address = 0x02;
    bulk_transfer_fail_action.no_return = 0;
    bulk_transfer_fail_action.status = UX_ERROR;
    ux_test_add_action_to_main_list(bulk_transfer_fail_action);

    /* Now send a packet so write function will run. */
    write_packet_tcp(&tcp_socket_host, &packet_pool_host, 0, "host");

    /* Ensure write failed. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_empty_actions());

    /* Now we're done. */
}

static void post_init_device()
{
}
