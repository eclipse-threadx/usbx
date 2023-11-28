/* Include necessary system files.  */

#define TEST_NX_PACKET_CHAIN
#define BASIC_TEST_NUM_ITERATIONS               5
#define BASIC_TEST_NUM_PACKETS_PER_ITERATION    5
#include "usbx_ux_test_cdc_ecm.h"

static UCHAR        device_is_finished;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_nx_packet_chain_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC ECM NX Packet Chain Test................................ ");

    stepinfo("\n");

#if (UX_DEVICE_ENDPOINT_BUFFER_OWNER == 1) && defined(UX_DEVICE_CLASS_CDC_ECM_ZERO_COPY)
    printf("Skipped for Zero Copy mode\n");
#else
    ux_test_cdc_ecm_initialize(first_unused_memory);
#endif
}

static void post_init_host()
{
UCHAR       *temp_buf = UX_NULL;
ULONG       n_available;
int         i;
int         device_num_writes = 0;

    /*======== RX buffer allocation fail test.  */

    if (cdc_ecm_host->ux_host_class_cdc_ecm_receive_buffer)
        temp_buf = cdc_ecm_host->ux_host_class_cdc_ecm_receive_buffer;
    tx_thread_suspend(&cdc_ecm_host -> ux_host_class_cdc_ecm_thread);
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_MEMORY_INSUFFICIENT));
    ux_test_utility_sim_mem_allocate_until_flagged(UX_HOST_CLASS_CDC_ECM_NX_PAYLOAD_SIZE, UX_CACHE_SAFE_MEMORY);
    cdc_ecm_host -> ux_host_class_cdc_ecm_receive_buffer = UX_NULL;
    tx_thread_resume(&cdc_ecm_host -> ux_host_class_cdc_ecm_thread);
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_empty_actions());
    if (temp_buf)
        cdc_ecm_host -> ux_host_class_cdc_ecm_receive_buffer = temp_buf;
    ux_test_utility_sim_mem_free_all_flagged(UX_CACHE_SAFE_MEMORY);


    /* Running UDP test. */
    stepinfo("running UDP test.\n");
    cdc_ecm_basic_test(BASIC_TEST_HOST, BASIC_TEST_UDP);

    /* Wait for device to finish.  */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&device_is_finished, UX_TRUE));

    /* Disconnect.  */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();

    /* Connect with null system change function. */
    _ux_system_host->ux_system_host_change_function = UX_NULL;

    /* Connect. */
    ux_test_connect_slave_and_host_wait_for_enum_completion();

    /* We're done.  */
}

static void post_init_device()
{

    /* Running UDP test. */
    cdc_ecm_basic_test(BASIC_TEST_DEVICE, BASIC_TEST_UDP);
    device_is_finished = UX_TRUE;
}