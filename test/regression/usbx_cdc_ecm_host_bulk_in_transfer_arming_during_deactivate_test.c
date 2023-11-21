/* This tests the case where there is an ongoing transfer on the bulk in endpoint
   during deactivation. The deactivate routine should wait for it to finish. 
   
   We do this by having the CDC-ECM thread suspend during the transfer arm,
   and then begin deactivation. We resume the CDC-ECM thread later! */

#include "usbx_ux_test_cdc_ecm.h"

static UCHAR cdc_ecm_thread_suspended;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_host_bulk_in_transfer_arming_during_deactivate_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM Host Bulk In Transfer Arming During... Test......... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static VOID suspend_cdc_ecm_thread_action_func(UX_TEST_ACTION *action, VOID *params)
{

    /* The CDC-ECM thread is calling us. */

    /* Notify test thread. */
    cdc_ecm_thread_suspended = 1;

    /* Suspend. Test thread should wake us back up. */
    tx_thread_suspend(tx_thread_identify());
}

static void post_init_host()
{

    /* Right now, host CDC-ECM thread is waiting for a bulk in transfer. We
       need for it to re-do a transfer so that we can get it to suspend via our
       action. First, we need to create and add our actions. */
    UX_TEST_ACTION bulk_in_transfer_suspend_action = {0};
    bulk_in_transfer_suspend_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    bulk_in_transfer_suspend_action.function = UX_HCD_TRANSFER_REQUEST;
    bulk_in_transfer_suspend_action.req_action = UX_TEST_MATCH_EP;
    bulk_in_transfer_suspend_action.req_ep_address = 0x81;
    bulk_in_transfer_suspend_action.action_func = suspend_cdc_ecm_thread_action_func;
    bulk_in_transfer_suspend_action.no_return = 1;
    ux_test_add_action_to_main_list(bulk_in_transfer_suspend_action);

    /* Now have the CDC-ECM thread re-do the transfer. */
    write_packet_tcp(&tcp_socket_device, &packet_pool_device, 1, "device");

    /* Wait for CDC-ECM thread to stall. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&cdc_ecm_thread_suspended, 1));

    /* Just for the hell of it, null out the system change function so we can hit that case too. */
    _ux_system_host->ux_system_host_change_function = UX_NULL;

    /* Now disconnect the host. */
    ux_test_disconnect_host_no_wait();

    /* Now wait for deactivation to suspend, waiting for bulk in to finish. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&cdc_ecm_host -> ux_host_class_cdc_ecm_bulk_in_transfer_waiting_for_check_and_arm_to_finish, UX_TRUE));

    /* Now resume the CDC-ECM thread. */
    tx_thread_resume(&cdc_ecm_host->ux_host_class_cdc_ecm_thread);

    /* And now wait for deactivation to complete. */
    ux_test_wait_for_enum_thread_completion();

    /* And now wait for... wait a second, we're done! Smiley face! That was easier than I thought! I'M SO FRICKEN HAPPY RIGHT NOWWWWWW! */
}

static void post_init_device()
{
}