/* This tests the case where the bulk in transfer fails (in the cdc-ecm thread). */

#include "usbx_ux_test_cdc_ecm.h"

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_host_bulk_in_transfer_fail_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM Host Bulk In Transfer Fail Test..................... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static UCHAR is_cdc_ecm_thread_suspended;

static VOID suspend_cdc_ecm_thread_action_func(UX_TEST_ACTION *action, VOID *params)
{

    is_cdc_ecm_thread_suspended = 1;

    /* We're being called by nx_packet_allocate in cdc-ecm thread. Wait for test
       thread to resume us. */
    tx_thread_suspend(tx_thread_identify());
}

static void post_init_host()
{

    /* Currently, the cdc-ecm thread should be waiting for a transfer to complete.
       We need to create an action for having the bulk in transfer fail, add
       the action, then send data to host so the cdc-ecm thread will re-arm the
       transfer (which should fail). */

    /* Create the action for having bulk in transfer fail. */
    UX_TEST_ACTION bulk_transfer_fail_action = {0};
    bulk_transfer_fail_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    bulk_transfer_fail_action.function = UX_HCD_TRANSFER_REQUEST;
    bulk_transfer_fail_action.req_action = UX_TEST_MATCH_EP;
    bulk_transfer_fail_action.req_ep_address = 0x81;
    bulk_transfer_fail_action.no_return = 0;
    bulk_transfer_fail_action.status = UX_ERROR;
    ux_test_add_action_to_main_list(bulk_transfer_fail_action);

    /* Write to the host so the cdc-ecm thread will redo the bulk in transfer. */
    write_packet_udp(&udp_socket_device, &packet_pool_device, HOST_IP_ADDRESS, HOST_SOCKET_PORT_UDP, 0, "device");

    /* Wait for the arm and fail. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_empty_actions());

    /* Wait for the host to re-arm and wait for the transfer. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uint(&cdc_ecm_host->ux_host_class_cdc_ecm_bulk_in_endpoint->ux_endpoint_transfer_request.ux_transfer_request_semaphore.tx_semaphore_suspended_count, 1));

    /* Now we're done. */
}

static void post_init_device()
{
}
