/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

#define CFG_DESC_POS (0x12)

#define DATA_IF_DESC_POS0 (DEFAULT_FRAMEWORK_LENGTH - 2*7 - 9 - 9)
#define DATA_IF_DESC_POS1 (DEFAULT_FRAMEWORK_LENGTH - 2*7 - 9)

#define INT_EP_DESC_POS   (0x12+0x09+0x08+0x09+0x05+0x0d+0x05)

#define BULK_EP_DESC_POS1 (_ux_system_slave -> ux_system_slave_device_framework_length - 14)
#define BULK_EP_DESC_POS2 (_ux_system_slave -> ux_system_slave_device_framework_length -  7)

static ULONG                   error_callback_counter;
static UCHAR                   framework_backup[DEFAULT_FRAMEWORK_LENGTH];
static UX_SLAVE_CLASS_CDC_ECM  *cdc_ecm_device_bak;

static void framework_rm_data_if_alt(void)
{
    // printf("Remove alternate setting of interface 1 (CDC DATA)\n");
    _ux_utility_memory_copy(default_device_framework + DATA_IF_DESC_POS0,
                            framework_backup + DATA_IF_DESC_POS1, 9 + 7 + 7);
    default_device_framework[CFG_DESC_POS + 2] -= 9;
    default_device_framework[DATA_IF_DESC_POS0 + 3] = 0;
    _ux_system_slave -> ux_system_slave_device_framework_length -= 9;
    _ux_system_slave -> ux_system_slave_device_framework_length_full_speed -= 9;
    _ux_system_slave -> ux_system_slave_device_framework_length_high_speed -= 9;
}

static void framework_copy(void)
{
    _ux_utility_memory_copy(framework_backup, default_device_framework, DEFAULT_FRAMEWORK_LENGTH);
}

static void framework_restore(void)
{
    // printf("Restore framework\n");
    _ux_utility_memory_copy(default_device_framework, framework_backup, DEFAULT_FRAMEWORK_LENGTH);
}

static void device_framework_restore(void)
{
    framework_restore();
    _ux_system_slave -> ux_system_slave_device_framework_length = DEFAULT_FRAMEWORK_LENGTH;
    _ux_system_slave -> ux_system_slave_device_framework_length_full_speed = DEFAULT_FRAMEWORK_LENGTH;
    _ux_system_slave -> ux_system_slave_device_framework_length_high_speed = DEFAULT_FRAMEWORK_LENGTH;
}

/* Setup requests */

static UX_TEST_SETUP _SetConfigure = UX_TEST_SETUP_SetConfigure;
static UX_TEST_SETUP _GetCfgDescr  = UX_TEST_SETUP_GetCfgDescr;
static UX_TEST_SETUP _SetAddress = UX_TEST_SETUP_SetAddress;
static UX_TEST_SETUP _GetDeviceDescriptor = UX_TEST_SETUP_GetDevDescr;
static UX_TEST_SETUP _GetConfigDescriptor = UX_TEST_SETUP_GetCfgDescr;

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

static UX_TEST_HCD_SIM_ACTION replace_configuration_descriptor[] = {
{   .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
    .function = UX_HCD_TRANSFER_REQUEST,
    .req_setup = &_GetConfigDescriptor,
    .req_action = UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V,
    .req_ep_address = 0,
    .req_data = (framework_backup + CFG_DESC_POS),
    .req_actual_len = 9,
    .req_status = UX_SUCCESS,
    .status = UX_SUCCESS,
    .action_func = UX_NULL,
    .no_return = UX_FALSE
},
{   .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
    .function = UX_HCD_TRANSFER_REQUEST,
    .req_setup = &_GetConfigDescriptor,
    .req_action = UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V,
    .req_ep_address = 0,
    .req_data = (framework_backup + CFG_DESC_POS),
    .req_actual_len = DEFAULT_FRAMEWORK_LENGTH - CFG_DESC_POS,
    .req_status = UX_SUCCESS,
    .status = UX_SUCCESS,
    .action_func = UX_NULL,
    .no_return = UX_FALSE
},
{   0   }
};

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_device_class_cdc_ecm_change_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_device_class_cdc_ecm_change Test......................... ");

    stepinfo("\n");

    /* Keep a copy of framework data.  */
    framework_copy();

    /* Override error trap. */
    ux_test_link_hooks_from_array(count_on_error_trap);

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static UINT _wait_host_inst_change_to(ULONG loop, UCHAR available)
{
    while(loop --)
    {
        _ux_utility_delay_ms(10);
        if (available)
        {
            if (cdc_ecm_host_from_system_change_function != UX_NULL)
                return UX_SUCCESS;
        }
        else
        {
            if (cdc_ecm_host_from_system_change_function == UX_NULL)
                return UX_SUCCESS;
        }
    }
    return UX_ERROR;
}

static void post_init_host()
{

    stepinfo(">>>>>>>>>>>>>>>>>>> Test instance connect\n");
    UX_TEST_ASSERT(cdc_ecm_device != UX_NULL);

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_disconnect_slave();
    ux_test_disconnect_host_wait_for_enum_completion();
    UX_TEST_CHECK_SUCCESS(_wait_host_inst_change_to(100, 0));

    stepinfo(">>>>>>>>>>>>>>>>>>> Test connect with data IF INT & BULK endpoint\n");
    default_device_framework[BULK_EP_DESC_POS1 + 3] = 0x3;
    default_device_framework[BULK_EP_DESC_POS1 + 6] = 0x1; /* avoid creation crush! */
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_set_actions(replace_configuration_descriptor);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    UX_TEST_CHECK_NOT_SUCCESS(_wait_host_inst_change_to(100, 1));
    default_device_framework[BULK_EP_DESC_POS1 + 3] = 0x2;

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_disconnect_slave();
    ux_test_disconnect_host_wait_for_enum_completion();
    UX_TEST_CHECK_SUCCESS(_wait_host_inst_change_to(100, 0));

    stepinfo(">>>>>>>>>>>>>>>>>>> Test connect with data IF BULK & INT endpoint\n");
    default_device_framework[BULK_EP_DESC_POS2 + 3] = 0x3;
    default_device_framework[BULK_EP_DESC_POS2 + 6] = 0x1; /* avoid creation crush! */
    ux_test_hcd_sim_host_set_actions(replace_configuration_descriptor);
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    UX_TEST_CHECK_NOT_SUCCESS(_wait_host_inst_change_to(100, 1));
    default_device_framework[BULK_EP_DESC_POS2 + 3] = 0x2;

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_disconnect_slave();
    ux_test_disconnect_host_wait_for_enum_completion();
    UX_TEST_CHECK_SUCCESS(_wait_host_inst_change_to(100, 0));

    /* Restore framework. */
    device_framework_restore();
    ux_test_hcd_sim_host_set_actions(UX_NULL);

    stepinfo(">>>>>>>>>>>>>>>>>>> Test connect to avoid post post operation\n");
    /* Connect. */
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    // class_cdc_ecm_get_host();

    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_host done\n");
}

static void post_init_device()
{
    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_device empty\n");
}