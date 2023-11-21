/* This tests the case the nx_packet_allocate fails in the cdc-ecm thread. */

#define TEST_NX_PACKET_CHAIN
#include "usbx_ux_test_cdc_ecm.h"

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_host_thread_packet_allocate_fail_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM Host Thread Packet Append Fail Test................. ");
    stepinfo("\n");
#if (UX_DEVICE_ENDPOINT_BUFFER_OWNER == 1) && defined(UX_DEVICE_CLASS_CDC_ECM_ZERO_COPY)
    printf("Skipped\n");
    return;
#endif

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

ULONG   n_available;
int i;
int device_num_writes = 0;
int host_num_reads = 0;

    /* Have the device send the max number of packets to the host. */
    ux_utility_delay_ms(1000);
    n_available = cdc_ecm_host->ux_host_class_cdc_ecm_packet_pool->nx_packet_pool_available;
    for (i = 0; i < n_available; i++)
        write_udp(&udp_socket_device, &packet_pool_device, HOST_IP_ADDRESS, HOST_SOCKET_PORT_UDP, device_num_writes++, "device", 4);
    ux_utility_delay_ms(1000);

    /* Time to setup our action since we expect an error. */
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_CLASS_ETH_PACKET_ERROR));

    /* Now send the one that should make the append error (discarded). */
    write_packet_udp(&udp_socket_device, &packet_pool_device, HOST_IP_ADDRESS, HOST_SOCKET_PORT_UDP, device_num_writes, "device");

    /* Now wait for error to occur. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_empty_actions());

    /* Let's be good sports and make sure everything else works. */
    for (i = 0; i < n_available; i++)
        read_packet_udp(&udp_socket_host, host_num_reads++, "host");

    /* Send one more. */
    write_packet_udp(&udp_socket_device, &packet_pool_device, HOST_IP_ADDRESS, HOST_SOCKET_PORT_UDP, device_num_writes++, "device");
    /* Read one more.  */
    read_packet_udp(&udp_socket_host, host_num_reads++, "host");
}

static void post_init_device()
{
}