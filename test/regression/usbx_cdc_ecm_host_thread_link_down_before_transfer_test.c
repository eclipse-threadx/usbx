/* This tests the case where right before the transfer is armed, the link state
   is down in the cdc-ecm thread. */

#include "usbx_ux_test_cdc_ecm.h"

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_host_thread_link_down_before_transfer_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM Host Thread Link Down Before Transfer Test.......... ");

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
       We need to create an action for having the cdc-ecm thread suspend during
       the packet allocate, then do a write from the device so the cdc-ecm thread
       call the packet allocate, then set the link to down, then resume the thread. */

    /* Create the action for having cdc-ecm thread suspend on packet allocate. */
    UX_TEST_ACTION packet_allocate_suspend_action = {0};
    packet_allocate_suspend_action.usbx_function = UX_TEST_OVERRIDE_NX_PACKET_ALLOCATE;
    packet_allocate_suspend_action.name_ptr = ip_pool_host_name;
    packet_allocate_suspend_action.action_func = suspend_cdc_ecm_thread_action_func;
    ux_test_add_action_to_main_list(packet_allocate_suspend_action);

    /* Write to the host so the cdc-ecm thread will redo the nx_packet_allocate. */
    write_packet_udp(&udp_socket_device, &packet_pool_device, HOST_IP_ADDRESS, HOST_SOCKET_PORT_UDP, 0, "device");

    /* Now wait for the cdc-ecm thread to suspend. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&is_cdc_ecm_thread_suspended, 1));

    /* Now set the link state to down. */
    ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, 0);

    /* Now wait for the link to be set to pending down (it won't be down since thread is suspended). */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, UX_HOST_CLASS_CDC_ECM_LINK_STATE_PENDING_DOWN));

    /* Now resume the cdc-ecm thread. */
    tx_thread_resume(&cdc_ecm_host->ux_host_class_cdc_ecm_thread);

    /* Now wait for link to be down. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, UX_HOST_CLASS_CDC_ECM_LINK_STATE_DOWN));

    /* Now we're done. */
}

static void post_init_device()
{
}