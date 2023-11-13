/* This test the case where a LINK DOWN event is received while transfers are
   ongoing.  */

#include "usbx_ux_test_cdc_ecm.h"
#include "ux_device_stack.h"

static ULONG global_basic_test_num_writes_host;
static ULONG global_basic_test_num_reads_host;

static ULONG global_basic_test_num_writes_device;
static ULONG global_basic_test_num_reads_device;

static UCHAR host_waiting_for_link_down;
static UCHAR host_waiting_for_link_up;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_link_down_while_ongoing_transfers_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM Host Link Down While Ongoing Transfers Test......... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void basic_test_host()
{

UINT    num_iters;
UINT    i;

    /*** Basic test - no transfers going on. ***/
    stepinfo("Basic test - no transfers going on.\n");

	/* Now wait for the link to be down.  */
    host_waiting_for_link_down = 1;
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, UX_HOST_CLASS_CDC_ECM_LINK_STATE_DOWN));

    /* Wait for everything to get cleaned up.  */
    tx_thread_sleep(MS_TO_TICK(1000));

	/* Now wait for the link to be up.  */
    host_waiting_for_link_up = 1;
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, UX_HOST_CLASS_CDC_ECM_LINK_STATE_UP));

    /* Run that basic test again. */
    for (num_iters = 0; num_iters < 10; num_iters++)
    {

        for (i = 0; i < 10; i++)
            write_packet_udp(&udp_socket_host, &packet_pool_host, DEVICE_IP_ADDRESS, DEVICE_SOCKET_PORT_UDP, global_basic_test_num_writes_host++, "host");

        for (i = 0; i < 10; i++)
            read_packet_udp(&udp_socket_host, global_basic_test_num_reads_host++, "host");
    }

    /* Wait for all transfers to complete. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&global_basic_test_num_reads_host, 100));
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&global_basic_test_num_reads_device, 100));
}

static void basic_test_device()
{

UINT  num_iters;
UINT  i;

    /*** Basic test. ***/

    /* Wait for host to wait for link down. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&host_waiting_for_link_down, 1));
    host_waiting_for_link_down = 0;

    /* Now set the link to down. */
    ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, UX_DEVICE_CLASS_CDC_ECM_LINK_STATE_DOWN);

    /* Wait for host to wait for link up. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&host_waiting_for_link_up, 1));
    host_waiting_for_link_up = 0;

    /* Now set the link to up. */
    ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, UX_DEVICE_CLASS_CDC_ECM_LINK_STATE_UP);

    /* Now do basic test. */
    for (num_iters = 0; num_iters < 10; num_iters++)
    {

        for (i = 0; i < 10; i++)
            write_packet_udp(&udp_socket_device, &packet_pool_device, HOST_IP_ADDRESS, HOST_SOCKET_PORT_UDP, global_basic_test_num_writes_device++, "device");

        for (i = 0; i < 10; i++)
            read_packet_udp(&udp_socket_device, global_basic_test_num_reads_device++, "device");
    }

    /* Wait for all transfers to complete. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&global_basic_test_num_reads_host, 100));
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&global_basic_test_num_reads_device, 100));
}

#define NUM_WRITES 500
#define NUM_WRITES_BEFORE_LINK_DOWN 10
#define MAX_WRITE_WHILE_LINK_DOWN (NUM_WRITES - NUM_WRITES_BEFORE_LINK_DOWN)

static void ongoing_writes_test_host()
{

UINT i;
UINT pre_write_fail_num_packet_pool_packets_available;
UINT num_writes = 0;

    /*** Link down when there are queued writes, and we try to add writes. ***/
    stepinfo("Ongoing writes test.\n");

    pre_write_fail_num_packet_pool_packets_available = packet_pool_host.nx_packet_pool_available;

    /* We expect some errors. */
    UX_TEST_ACTION error_match_action = create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_CLASS_CDC_ECM_LINK_STATE_DOWN_ERROR);
    ux_test_add_action_to_main_list_multiple(error_match_action, MAX_WRITE_WHILE_LINK_DOWN);

    /* Queue up some writes - device isn't going to read them, so they stay queued. */
    for (i = 0; i < NUM_WRITES; i++)
    {

        /* Write packet. */
        write_packet_udp(&udp_socket_host, &packet_pool_host, DEVICE_IP_ADDRESS, DEVICE_SOCKET_PORT_UDP, num_writes++, "host");

        /* Have we queued 10 packets? */
        if (i == NUM_WRITES_BEFORE_LINK_DOWN)
        {

            /* Set the link to down. The _hope_ here is to have the CDC-ECM thread
               process the link down while we're still writing packets. This
               should be improved in the future. I don't want to be the poor
               SOB that has to do it!
               
               We cheat here by calling a device API from the host. */
            ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, UX_DEVICE_CLASS_CDC_ECM_LINK_STATE_DOWN);

            while (cdc_ecm_host->ux_host_class_cdc_ecm_link_state != UX_HOST_CLASS_CDC_ECM_LINK_STATE_DOWN)
                tx_thread_sleep(10);
        }
    }

    /* Make sure everything is ok. */
    UX_TEST_ASSERT(cdc_ecm_host->ux_host_class_cdc_ecm_link_state == UX_HOST_CLASS_CDC_ECM_LINK_STATE_DOWN);
NX_PACKET_POOL *packet_pool_host_local = &packet_pool_host;
UX_HOST_CLASS_CDC_ECM *cdc_ecm_host_local = cdc_ecm_host;
    UX_TEST_ASSERT(packet_pool_host.nx_packet_pool_available == pre_write_fail_num_packet_pool_packets_available);

    /* Make sure at least one error was reported. */
    UX_TEST_ASSERT(ux_test_get_num_actions_left() != 90);

    /* Now clear the error match actions out. */
    ux_test_clear_main_list_actions();

    /* Run that basic test again. */

	/* Now wait for the link to be up.  */
    host_waiting_for_link_up = 1;
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, UX_HOST_CLASS_CDC_ECM_LINK_STATE_UP));

    cdc_ecm_basic_test(BASIC_TEST_HOST, BASIC_TEST_TCP);
}

static void ongoing_writes_test_device()
{

    /*** Ongoing writes test. ***/

    /* The host is gonna do all the work. At some point, he'll want the link back
       up to run the basic test again. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&host_waiting_for_link_up, 1));

    /* Set link to up. */
    ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, UX_DEVICE_CLASS_CDC_ECM_LINK_STATE_UP);

    /* Now do basic test. */
    cdc_ecm_basic_test(BASIC_TEST_DEVICE, BASIC_TEST_TCP);
}

static void post_init_host()
{

    //basic_test_host();
    ongoing_writes_test_host();
}

static void post_init_device(ULONG input)
{

    //basic_test_device();
    ongoing_writes_test_device();
}
