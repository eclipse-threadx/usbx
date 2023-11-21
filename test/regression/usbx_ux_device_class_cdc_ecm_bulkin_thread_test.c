/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

static ULONG                error_callback_counter;

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

static UX_TEST_ACTION bulk_transfer_replace[] =
{
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION,
        .function = UX_DCD_TRANSFER_REQUEST,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP | UX_TEST_SIM_REQ_ANSWER,
        .req_ep_address = 0x81,
        .req_actual_len = 0,
        .req_status = UX_SUCCESS,
        .status = UX_SUCCESS,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION,
        .function = UX_DCD_TRANSFER_REQUEST,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP | UX_TEST_SIM_REQ_ANSWER,
        .req_ep_address = 0x81,
        .req_actual_len = 0,
        .req_status = UX_SUCCESS,
        .status = UX_SUCCESS,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
{ 0 },
};


/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_device_class_cdc_ecm_bulkin_thread_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_device_class_cdc_ecm_bulkin_thread Test.................. ");

    stepinfo("\n");

    /* Override error trap. */
    ux_test_link_hooks_from_array(count_on_error_trap);

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

ULONG              link_state[] = {0,1,2,3,0,1};
UINT               i;
UCHAR              buffer[64];
NX_PACKET          packet = {0};

    /* Point the prepend ptr to some valid memory so there's no crash. */
    packet.nx_packet_prepend_ptr = buffer;

    stepinfo(">>>>>>>>>>>>>>>>>>> Test activate check\n");
    UX_TEST_ASSERT(cdc_ecm_device != UX_NULL);

    /* Disable the other threads.  */
    // _ux_utility_thread_suspend(&cdc_ecm_device->ux_slave_class_cdc_ecm_bulkin_thread);
    _ux_utility_thread_suspend(&cdc_ecm_device->ux_slave_class_cdc_ecm_bulkout_thread);
    _ux_utility_thread_suspend(&cdc_ecm_device->ux_slave_class_cdc_ecm_interrupt_thread);

#if 0
    stepinfo(">>>>>>>>>>>>>>>>>>> Test endpoint closed\n");
    ep = cdc_ecm_device->ux_slave_class_cdc_ecm_bulkin_endpoint;
    cdc_ecm_device->ux_slave_class_cdc_ecm_bulkin_endpoint = UX_NULL;
    _ux_utility_event_flags_set(&cdc_ecm_device -> ux_slave_class_cdc_ecm_event_flags_group, UX_DEVICE_CLASS_CDC_ECM_NEW_DEVICE_STATE_CHANGE_EVENT, TX_OR);
    _ux_utility_thread_sleep(2);
    cdc_ecm_device->ux_slave_class_cdc_ecm_bulkin_endpoint = ep;
    _ux_utility_thread_resume(&cdc_ecm_device->ux_slave_class_cdc_ecm_bulkin_thread);
#endif

    i = (cdc_ecm_device -> ux_slave_class_cdc_ecm_link_state == 0) ? 1 : 0;
    for (;i < 5; i ++)
    {

        stepinfo(">>>>>>>>>>>>>>>>>>> Test %d write length overflow\n", i);
        packet.nx_packet_length = 1 + UX_SLAVE_REQUEST_DATA_MAX_LENGTH;
        ux_device_class_cdc_ecm_write(cdc_ecm_device, &packet);

        stepinfo(">>>>>>>>>>>>>>>>>>> Test %d bulkin UX_ERROR\n", i);
        bulk_transfer_replace[0].req_status = UX_ERROR;
        bulk_transfer_replace[0].status = UX_ERROR;
        bulk_transfer_replace[1].req_status = UX_ERROR;
        bulk_transfer_replace[1].status = UX_ERROR;
        ux_test_dcd_sim_slave_set_actions(bulk_transfer_replace);
        error_callback_counter = 0;
        packet.nx_packet_length = 1;
        ux_device_class_cdc_ecm_write(cdc_ecm_device, &packet);
        _ux_utility_thread_sleep(2);
        if (cdc_ecm_device -> ux_slave_class_cdc_ecm_link_state == UX_DEVICE_CLASS_CDC_ECM_LINK_STATE_UP)
            UX_TEST_ASSERT(error_callback_counter);

        stepinfo(">>>>>>>>>>>>>>>>>>> Test %d bulkin UX_TRANSFER_BUS_RESET\n", i);
        bulk_transfer_replace[0].req_status = UX_TRANSFER_BUS_RESET;
        bulk_transfer_replace[0].status = UX_TRANSFER_BUS_RESET;
        bulk_transfer_replace[1].req_status = UX_TRANSFER_BUS_RESET;
        bulk_transfer_replace[1].status = UX_TRANSFER_BUS_RESET;
        ux_test_dcd_sim_slave_set_actions(bulk_transfer_replace);
        packet.nx_packet_length = 1;
        ux_device_class_cdc_ecm_write(cdc_ecm_device, &packet);
        _ux_utility_thread_sleep(2);

        stepinfo(">>>>>>>>>>>>>>>>>>> Test %d link state change\n", i);
        cdc_ecm_device -> ux_slave_class_cdc_ecm_link_state = link_state[i];
        if (i & 1)
            cdc_ecm_device -> ux_slave_class_cdc_ecm_xmit_queue = &packet;
        _ux_utility_event_flags_set(&cdc_ecm_device -> ux_slave_class_cdc_ecm_event_flags_group, UX_DEVICE_CLASS_CDC_ECM_NEW_BULKIN_EVENT, TX_OR);
        if (i & 1)
            cdc_ecm_device -> ux_slave_class_cdc_ecm_xmit_queue = &packet;
        _ux_utility_event_flags_set(&cdc_ecm_device -> ux_slave_class_cdc_ecm_event_flags_group, UX_DEVICE_CLASS_CDC_ECM_NEW_DEVICE_STATE_CHANGE_EVENT, TX_OR);
        _ux_utility_thread_sleep(2);
    }

    // stepinfo(">>>>>>>>>>>>>>>>>>> Test connect to avoid post post operation\n");
    // /* Connect. */
    // ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    // ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    // class_cdc_ecm_get_host();

    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_host done\n");
}

static void post_init_device()
{
    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_device empty\n");
}