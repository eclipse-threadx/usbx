/* This tests the case where the link is down and a thread is waiting for the
   bulk in transfer check-and-arm to finish. */

#include "usbx_ux_test_cdc_ecm.h"

static UCHAR cdc_ecm_thread_suspended;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_host_bulk_in_transfer_arming_fails_due_to_link_down_and_thread_waiting_test_application_define(void *first_unused_memory)
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

VOID cdc_ecm_thread_suspend_action_func(UX_TEST_ACTION *action, VOID *params)
{
    cdc_ecm_thread_suspended = 1;
    tx_thread_suspend(tx_thread_identify());
}

static void post_init_host()
{

    /* Right now, host CDC-ECM thread is waiting for a bulk in transfer. We
       need for it to re-do a transfer so that we can get it to suspend via our
       action. First, we need to create and add our actions. */
    UX_TEST_ACTION bulk_in_transfer_suspend_action = {0};
    bulk_in_transfer_suspend_action.usbx_function = UX_TEST_OVERRIDE_NX_PACKET_ALLOCATE;
    bulk_in_transfer_suspend_action.name_ptr = ip_pool_host_name;
    bulk_in_transfer_suspend_action.action_func = cdc_ecm_thread_suspend_action_func;
    ux_test_add_action_to_main_list(bulk_in_transfer_suspend_action);

    /* Send data so thread does packet allocate. */
    write_packet_udp(&udp_socket_device, &packet_pool_device, HOST_IP_ADDRESS, HOST_SOCKET_PORT_UDP, 0, "device");

    /* Wait for the thread to be suspended. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&cdc_ecm_thread_suspended, 1));

    /* Set link state to down. */
    ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, 0);

    /* Now wait for the link to be set to pending down (it won't be down since thread is suspended). */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, UX_HOST_CLASS_CDC_ECM_LINK_STATE_PENDING_DOWN));

    /* Make CDC-ECM thread think a thread is indeed waiting. */
    cdc_ecm_host->ux_host_class_cdc_ecm_bulk_in_transfer_waiting_for_check_and_arm_to_finish = UX_TRUE;

    /* Now resume CDC-ECM thread. */
    tx_thread_resume(&cdc_ecm_host->ux_host_class_cdc_ecm_thread);

    /* Now wait for the semaphore to be put. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_bulk_in_transfer_waiting_for_check_and_arm_to_finish_semaphore.tx_semaphore_count, 1));

    /* Now get the semaphore so the count is okay. */
    tx_semaphore_get(&cdc_ecm_host->ux_host_class_cdc_ecm_bulk_in_transfer_waiting_for_check_and_arm_to_finish_semaphore, TX_WAIT_FOREVER);

    /* And now we're done. */
}

static void post_init_device()
{
}