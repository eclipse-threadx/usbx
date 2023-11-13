/* This tests the case where there is an ongoing transfer on the bulk out endpoint
   during link down. The interrupt notification function should wait for it to finish. 
   
   We do this by having the CDC-ECM thread suspend during the transfer arm,
   and then begin link down event. We resume the HCD thread later! */

#include "usbx_ux_test_cdc_ecm.h"

static UCHAR        write_thread_suspended;
static TX_THREAD    write_thread;
static UCHAR        write_thread_stack[2048];

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_host_bulk_out_transfer_arming_during_link_down_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM Host Bulk Out Transfer Arming During... Test........ ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static VOID suspend_write_thread_action_func(UX_TEST_ACTION *action, VOID *params)
{

    /* The write thread is calling us. */

    /* Notify test thread. */
    write_thread_suspended = 1;

    /* Suspend. Test thread should wake us back up. */
    tx_thread_suspend(tx_thread_identify());
}

static void write_thread_entry(ULONG input)
{

    /* Do the write. */
    write_packet_tcp(&tcp_socket_host, &packet_pool_host, 0, "host");
}

static void post_init_host()
{

    /* Create the thread that will do the write. */
    UX_TEST_CHECK_SUCCESS(tx_thread_create(&write_thread, "write thread", write_thread_entry, 0, write_thread_stack, sizeof(write_thread_stack), 20, 20, 0, TX_DONT_START));

    /* Right now, host CDC-ECM thread is waiting for a bulk out transfer. We
       need for it to re-do a transfer so that we can get it to suspend via our
       action. First, we need to create and add our actions. */
    UX_TEST_ACTION bulk_out_transfer_suspend_action = {0};
    bulk_out_transfer_suspend_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    bulk_out_transfer_suspend_action.function = UX_HCD_TRANSFER_REQUEST;
    bulk_out_transfer_suspend_action.req_action = UX_TEST_MATCH_EP;
    bulk_out_transfer_suspend_action.req_ep_address = 0x02;
    bulk_out_transfer_suspend_action.action_func = suspend_write_thread_action_func;
    bulk_out_transfer_suspend_action.no_return = 1;
    ux_test_add_action_to_main_list(bulk_out_transfer_suspend_action);

    /* Now start the write thread. */
    tx_thread_resume(&write_thread);

    /* Now wait for it to be suspended. */
    ux_test_wait_for_value_uchar(&write_thread_suspended, 1);

    /* Now change the link to down. */
    ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, 0);

    /* Now wait for CDC-ECM thread to suspend, waiting for bulk out to finish. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&cdc_ecm_host -> ux_host_class_cdc_ecm_bulk_out_transfer_waiting_for_check_and_arm_to_finish, UX_TRUE));

    /* Now resume the write thread. */
    tx_thread_resume(&write_thread);

    /* And now wait for write instance to go down. */
    ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, 0);

    /* And now wait for... wait a second, we're done! Smiley face! That was easier than I thought! I'M SO FRICKEN HAPPY RIGHT NOWWWWWW! */
}

static void post_init_device()
{
}