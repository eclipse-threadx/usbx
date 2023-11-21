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

static UX_TEST_ACTION interrupt_transfer_replace[] =
{
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION,
        .function = UX_DCD_TRANSFER_REQUEST,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP | UX_TEST_SIM_REQ_ANSWER,
        .req_ep_address = 0x83,
        .req_actual_len = UX_DEVICE_CLASS_CDC_ECM_INTERRUPT_RESPONSE_LENGTH,
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
void usbx_ux_device_class_cdc_ecm_interrupt_thread_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_device_class_cdc_ecm_interrupt_thread Test............... ");

    stepinfo("\n");

    /* Override error trap. */
    ux_test_link_hooks_from_array(count_on_error_trap);

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{


    stepinfo(">>>>>>>>>>>>>>>>>>> Test activate check\n");
    UX_TEST_ASSERT(cdc_ecm_device != UX_NULL);

    /* Disable the other threads.  */
    _ux_utility_thread_suspend(&cdc_ecm_device->ux_slave_class_cdc_ecm_bulkin_thread);
    _ux_utility_thread_suspend(&cdc_ecm_device->ux_slave_class_cdc_ecm_bulkout_thread);

#if 0
    stepinfo(">>>>>>>>>>>>>>>>>>> Test get ux_slave_class_cdc_ecm_event_flags_group error\n");
    _ux_utility_event_flags_delete(&cdc_ecm_device -> ux_slave_class_cdc_ecm_event_flags_group);
    _ux_utility_thread_sleep(1);
#endif
#if 0
    stepinfo(">>>>>>>>>>>>>>>>>>> Test get ux_slave_class_cdc_ecm_event_flags_group actual flag error\n");
    _ux_utility_event_flags_create(&cdc_ecm_device -> ux_slave_class_cdc_ecm_event_flags_group, "ux_device_class_cdc_ecm_event_flag");
    _ux_utility_event_flags_set(&cdc_ecm_device -> ux_slave_class_cdc_ecm_event_flags_group, UX_DEVICE_CLASS_CDC_ECM_NEW_INTERRUPT_EVENT, TX_OR);
    _ux_utility_thread_sleep(1);
#endif

    stepinfo(">>>>>>>>>>>>>>>>>>> Test interrupt transfer UX_TRANSFER_BUS_RESET\n");
    interrupt_transfer_replace[0].req_status = UX_TRANSFER_BUS_RESET;
    interrupt_transfer_replace[0].status = UX_TRANSFER_BUS_RESET;
    ux_test_dcd_sim_slave_set_actions(interrupt_transfer_replace);
    _ux_utility_event_flags_set(&cdc_ecm_device -> ux_slave_class_cdc_ecm_event_flags_group, UX_DEVICE_CLASS_CDC_ECM_NETWORK_NOTIFICATION_EVENT, TX_OR);
    _ux_utility_thread_sleep(2);

    stepinfo(">>>>>>>>>>>>>>>>>>> Test interrupt transfer UX_ERROR\n");
    interrupt_transfer_replace[0].req_status = UX_ERROR;
    interrupt_transfer_replace[0].status = UX_ERROR;
    ux_test_dcd_sim_slave_set_actions(interrupt_transfer_replace);
    error_callback_counter = 0;
    _ux_utility_event_flags_set(&cdc_ecm_device -> ux_slave_class_cdc_ecm_event_flags_group, UX_DEVICE_CLASS_CDC_ECM_NETWORK_NOTIFICATION_EVENT, TX_OR);
    _ux_utility_thread_sleep(2);
    UX_TEST_ASSERT(error_callback_counter > 0);

    stepinfo(">>>>>>>>>>>>>>>>>>> Test connect to avoid post post operation\n");
    /* Connect. */
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    class_cdc_ecm_get_host();

    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_host done\n");
}

static void post_init_device()
{
    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_device empty\n");
}