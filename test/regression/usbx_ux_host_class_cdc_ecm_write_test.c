/* This functions tests the ux_host_class_cdc_ecm_write API. */

#include "usbx_ux_test_cdc_ecm.h"

static UCHAR has_host_write_failed_yet;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_host_class_cdc_ecm_write_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running _ux_host_class_cdc_ecm_write Test........................... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

    UX_HOST_CLASS_CDC_ECM local_cdc_ecm = {0};
    NX_PACKET *my_packet;

    /** Test packet size error.  */
    local_cdc_ecm.ux_host_class_cdc_ecm_state = UX_HOST_CLASS_INSTANCE_LIVE;
    UX_TEST_CHECK_SUCCESS(nx_packet_allocate(&packet_pool_host, &my_packet, NX_TCP_PACKET, NX_WAIT_FOREVER));
    my_packet->nx_packet_length = UX_HOST_CLASS_CDC_ECM_NX_PAYLOAD_SIZE + 1;
    UX_TEST_ASSERT(ux_host_class_cdc_ecm_write(&local_cdc_ecm, my_packet) != UX_SUCCESS);

#ifdef UX_HOST_CLASS_CDC_ECM_PACKET_CHAIN_SUPPORT

    /** Test memory not available.  */
    ux_test_utility_sim_mem_allocate_until_flagged(UX_HOST_CLASS_CDC_ECM_NX_PAYLOAD_SIZE, UX_CACHE_SAFE_MEMORY);
    my_packet->nx_packet_length = UX_HOST_CLASS_CDC_ECM_NX_PAYLOAD_SIZE;
    my_packet->nx_packet_next = my_packet;
    local_cdc_ecm.ux_host_class_cdc_ecm_xmit_buffer = UX_NULL;
    local_cdc_ecm.ux_host_class_cdc_ecm_xmit_queue_head = UX_NULL;
    local_cdc_ecm.ux_host_class_cdc_ecm_link_state = UX_HOST_CLASS_CDC_ECM_LINK_STATE_UP;
    UX_TEST_ASSERT(ux_host_class_cdc_ecm_write(&local_cdc_ecm, my_packet) != UX_SUCCESS);
    ux_test_utility_sim_mem_free_all_flagged(UX_CACHE_SAFE_MEMORY);
    my_packet->nx_packet_next = UX_NULL;
#endif

    /** Test instance not live. **/

    local_cdc_ecm.ux_host_class_cdc_ecm_state = UX_HOST_CLASS_INSTANCE_SHUTDOWN;
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_HOST_CLASS_INSTANCE_UNKNOWN));
    UX_TEST_ASSERT(ux_host_class_cdc_ecm_write(&local_cdc_ecm, 0) != UX_SUCCESS);

    /** Test link state down. **/

    local_cdc_ecm.ux_host_class_cdc_ecm_state = UX_HOST_CLASS_INSTANCE_LIVE;
    local_cdc_ecm.ux_host_class_cdc_ecm_link_state = UX_HOST_CLASS_CDC_ECM_LINK_STATE_DOWN;
    UX_TEST_CHECK_SUCCESS(nx_packet_allocate(&packet_pool_host, &my_packet, NX_TCP_PACKET, NX_WAIT_FOREVER));
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_CLASS_CDC_ECM_LINK_STATE_DOWN_ERROR));
    UX_TEST_ASSERT(ux_host_class_cdc_ecm_write(&local_cdc_ecm, my_packet) != UX_SUCCESS);
}

static void post_init_device()
{
}
