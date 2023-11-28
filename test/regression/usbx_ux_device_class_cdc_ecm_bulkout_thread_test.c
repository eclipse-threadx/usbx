/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

static ULONG                error_callback_counter;
static UCHAR                buffer[64];
static ULONG                error_header_sim;

static void count_error_callback(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{
UX_TEST_ERROR_CALLBACK_PARAMS *error = (UX_TEST_ERROR_CALLBACK_PARAMS *)params;

    // printf("error trap #%d: 0x%x, 0x%x, 0x%x\n", __LINE__, error->system_level, error->system_context, error->error_code);
    error_callback_counter ++;
}

static UX_TEST_HCD_SIM_ACTION count_on_error_trap[] = {
{   .usbx_function = UX_TEST_OVERRIDE_ERROR_CALLBACK,
    .action_func = count_error_callback,
},
{   0   }
};

static void simulate_ip_header_error(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{

UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *p = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS *)params;
UX_TRANSFER                                   *transfer = (UX_TRANSFER *)p->parameter;
UCHAR                                         *pretend_ptr = transfer->ux_transfer_request_data_pointer;


    if (error_header_sim & 1)
    {
        *(pretend_ptr + 12) = 0x08;
        *(pretend_ptr + 13) = 0x01;
    }
    else
    {
        *(pretend_ptr + 12) = 0x00;
        *(pretend_ptr + 13) = 0x00;
    }

    error_header_sim ++;
}

static UX_TEST_ACTION bulk_transfer_replace[] =
{
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
        .function = UX_HCD_TRANSFER_REQUEST,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x02,
        .req_actual_len = 64,
        .req_status = UX_SUCCESS,
        .action_func = simulate_ip_header_error,
        .status = UX_SUCCESS,
        .do_after = UX_FALSE,
        .no_return = UX_TRUE,
    },
{ 0 },
};


/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_device_class_cdc_ecm_bulkout_thread_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_device_class_cdc_ecm_bulkout_thread Test................. ");

    stepinfo("\n");

    /* Override error trap. */
    ux_test_link_hooks_from_array(count_on_error_trap);

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

UINT                        i;
UX_SLAVE_ENDPOINT           *endpoint;
USB_NETWORK_DEVICE_TYPE     *ux_nx_device;
NX_IP                       *ip;


    stepinfo(">>>>>>>>>>>>>>>>>>> Test activate check\n");
    UX_TEST_ASSERT(cdc_ecm_device != UX_NULL);

    /* Disable the other threads.  */
    _ux_utility_thread_suspend(&cdc_ecm_device->ux_slave_class_cdc_ecm_bulkin_thread);
    // _ux_utility_thread_suspend(&cdc_ecm_device->ux_slave_class_cdc_ecm_bulkout_thread);
    _ux_utility_thread_suspend(&cdc_ecm_device->ux_slave_class_cdc_ecm_interrupt_thread);

    stepinfo(">>>>>>>>>>>>>>>>>>> Test UX allocate error\n");
    error_callback_counter = 0;
    for (i = 0; i < 2000; i ++)
    {
        write_packet_udp(&udp_socket_host, &packet_pool_host, DEVICE_IP_ADDRESS, DEVICE_SOCKET_PORT_UDP, global_basic_test_num_writes_host++, "host");
        _ux_utility_thread_sleep(1);
        if (error_callback_counter)
            break;
    }
    UX_TEST_ASSERT(error_callback_counter);
    for(;i ; i --)
    {
        read_packet_udp(&udp_socket_device, global_basic_test_num_reads_device++, "device");
        _ux_utility_thread_sleep(1);
    }

    stepinfo(">>>>>>>>>>>>>>>>>>> Test packet header error\n");
    for (i = 0; i < 4; i ++)
    {
        ux_test_hcd_sim_host_set_actions(bulk_transfer_replace);
        write_packet_udp(&udp_socket_host, &packet_pool_host, DEVICE_IP_ADDRESS, DEVICE_SOCKET_PORT_UDP, global_basic_test_num_writes_host++, "host");
        _ux_utility_thread_sleep(1);
    }

    // stepinfo(">>>>>>>>>>>>>>>>>>> Test connect to avoid post post operation\n");
    // /* Connect. */
    // ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    // ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    // class_cdc_ecm_get_host();

    stepinfo(">>>>>>>>>>>>>>>>>>> Test IP instance attach\n");
    ux_nx_device = (USB_NETWORK_DEVICE_TYPE *)cdc_ecm_device->ux_slave_class_cdc_ecm_network_handle;
    ux_nx_device -> ux_network_device_link_status = UX_FALSE;
    ip = ux_nx_device -> ux_network_device_ip_instance;
    ux_nx_device -> ux_network_device_ip_instance = UX_NULL;
    cdc_ecm_device -> ux_slave_class_cdc_ecm_packet_pool = UX_NULL;
    write_packet_udp(&udp_socket_host, &packet_pool_host, DEVICE_IP_ADDRESS, DEVICE_SOCKET_PORT_UDP, global_basic_test_num_writes_host++, "host");
    _ux_utility_delay_ms(1);
    ux_nx_device -> ux_network_device_ip_instance = ip;
    ux_nx_device -> ux_network_device_link_status = UX_TRUE;
    _ux_utility_delay_ms(UX_DEVICE_CLASS_CDC_ECM_PACKET_POOL_INST_WAIT + 10);
    UX_TEST_ASSERT(cdc_ecm_device -> ux_slave_class_cdc_ecm_packet_pool == ip -> nx_ip_default_packet_pool);

    stepinfo(">>>>>>>>>>>>>>>>>>> Test endpoint not ready error\n");
    endpoint = cdc_ecm_device -> ux_slave_class_cdc_ecm_bulkout_endpoint;
    cdc_ecm_device -> ux_slave_class_cdc_ecm_bulkout_endpoint = UX_NULL;
    write_packet_udp(&udp_socket_host, &packet_pool_host, DEVICE_IP_ADDRESS, DEVICE_SOCKET_PORT_UDP, global_basic_test_num_writes_host++, "host");
    _ux_utility_delay_ms(1);
    cdc_ecm_device -> ux_slave_class_cdc_ecm_bulkout_endpoint = endpoint;
    _ux_utility_delay_ms(UX_DEVICE_CLASS_CDC_ECM_LINK_CHECK_WAIT + 10);

    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_host done\n");
}

static void post_init_device()
{
    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_device empty\n");
}