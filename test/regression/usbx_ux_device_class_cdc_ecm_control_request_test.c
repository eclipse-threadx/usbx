/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

static ULONG                error_callback_counter;
static UCHAR                buffer[64];

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

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_device_class_cdc_ecm_control_request_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_device_class_cdc_ecm_control_request Test................ ");

    stepinfo("\n");

    /* Override error trap. */
    ux_test_link_hooks_from_array(count_on_error_trap);

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

UINT                     status;
UX_DEVICE                *device;
UX_ENDPOINT              *control_endpoint;
UX_TRANSFER              *transfer_request;


    stepinfo(">>>>>>>>>>>>>>>>>>> Test activate check\n");
    UX_TEST_ASSERT(cdc_ecm_device != UX_NULL);

    UX_TEST_CHECK_SUCCESS(ux_host_stack_device_get(0, &device));
    control_endpoint = &device->ux_device_control_endpoint;
    transfer_request = &control_endpoint->ux_endpoint_transfer_request;

    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_index =             0;

    stepinfo(">>>>>>>>>>>>>>>>>>> Test UX_DEVICE_CLASS_CDC_ECM_SET_ETHERNET_MULTICAST_FILTER\n");
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_CDC_ECM_SET_ETHERNET_MULTICAST_FILTER;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             1;
    UX_TEST_CHECK_SUCCESS(ux_host_stack_transfer_request(transfer_request));

    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_CDC_ECM_SET_ETHERNET_MULTICAST_FILTER;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0;
    UX_TEST_CHECK_SUCCESS(ux_host_stack_transfer_request(transfer_request));

    stepinfo(">>>>>>>>>>>>>>>>>>> Test UX_DEVICE_CLASS_CDC_ECM_SET_ETHERNET_POWER_MANAGEMENT_FILTER\n");
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_CDC_ECM_SET_ETHERNET_POWER_MANAGEMENT_FILTER;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             1;
    UX_TEST_CHECK_SUCCESS(ux_host_stack_transfer_request(transfer_request));

    stepinfo(">>>>>>>>>>>>>>>>>>> Test UX_DEVICE_CLASS_CDC_ECM_GET_ETHERNET_POWER_MANAGEMENT_FILTER\n");
    transfer_request -> ux_transfer_request_requested_length =  2;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_CDC_ECM_GET_ETHERNET_POWER_MANAGEMENT_FILTER;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0;
    status = (ux_host_stack_transfer_request(transfer_request));
    if (status == UX_SUCCESS)
    {
        UX_TEST_ASSERT(buffer[0] == 1);
    }
    else
    {
        UX_TEST_CHECK_CODE(UX_TRANSFER_STALLED, status);
    }

    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_CDC_ECM_SET_ETHERNET_POWER_MANAGEMENT_FILTER;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0;
    UX_TEST_CHECK_SUCCESS(ux_host_stack_transfer_request(transfer_request));

    stepinfo(">>>>>>>>>>>>>>>>>>> Test UX_DEVICE_CLASS_CDC_ECM_SET_ETHERNET_PACKET_FILTER\n");
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_CDC_ECM_SET_ETHERNET_PACKET_FILTER;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             1;
    UX_TEST_CHECK_SUCCESS(ux_host_stack_transfer_request(transfer_request));

    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_CDC_ECM_SET_ETHERNET_PACKET_FILTER;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0;
    UX_TEST_CHECK_SUCCESS(ux_host_stack_transfer_request(transfer_request));

    stepinfo(">>>>>>>>>>>>>>>>>>> Test unknown request\n");
    transfer_request -> ux_transfer_request_requested_length =  0;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_CDC_ECM_SET_ETHERNET_PACKET_FILTER + 7;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_value =             0;
    UX_TEST_CHECK_CODE(UX_TRANSFER_STALLED, ux_host_stack_transfer_request(transfer_request));

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