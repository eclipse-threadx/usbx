/* This tests the case where the host receives a non-IP packet. */

#include "usbx_ux_test_cdc_ecm.h"

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_host_non_ip_packet_received_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM Host Non IP Packet Received Test.................... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

NX_PACKET   *out_packet;

    /* Note that the packets we send to the host will just get released by NetX
       for being invalid. */

    /** Test '*(packet -> nx_packet_prepend_ptr + 12) != 0x08' **/

    UX_TEST_CHECK_SUCCESS(nx_packet_allocate(&packet_pool_device, &out_packet, NX_UDP_PACKET, NX_WAIT_FOREVER));

    out_packet->nx_packet_length = (ULONG)(out_packet->nx_packet_data_end - out_packet->nx_packet_prepend_ptr);
    memset(out_packet->nx_packet_prepend_ptr, 'a', out_packet->nx_packet_length);
    out_packet->nx_packet_prepend_ptr[12] = 0x00;

    /* Now send the packet. */
    _ux_device_class_cdc_ecm_write(cdc_ecm_device, out_packet);

    /* Wait for host to receive it. */
    tx_thread_sleep(500);

    /** Test '*(packet -> nx_packet_prepend_ptr + 13) != 0' **/

    UX_TEST_CHECK_SUCCESS(nx_packet_allocate(&packet_pool_device, &out_packet, NX_UDP_PACKET, NX_WAIT_FOREVER));

    out_packet->nx_packet_length = (ULONG)(out_packet->nx_packet_data_end - out_packet->nx_packet_prepend_ptr);
    memset(out_packet->nx_packet_prepend_ptr, 'a', out_packet->nx_packet_length);
    out_packet->nx_packet_prepend_ptr[12] = 0x08;
    out_packet->nx_packet_prepend_ptr[13] = 0x01;

    /* Now send the packet. */
    _ux_device_class_cdc_ecm_write(cdc_ecm_device, out_packet);

    /* Wait for host to receive it. */
    tx_thread_sleep(500);

    /* We're done!? */
}

static void post_init_device()
{
}
