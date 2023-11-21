/* This tests the _ux_host_class_cdc_ecm_interrupt_notification function. */

#include "usbx_ux_test_cdc_ecm.h"

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_host_class_cdc_ecm_interrupt_notification_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_host_class_cdc_ecm_interrupt_notification Test........... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

UX_HOST_CLASS_CDC_ECM   my_cdc_ecm;
UX_TRANSFER             transfer_request;
UCHAR                   transfer_request_data[16];
UX_ENDPOINT             endpoint;  
ULONG                   notification_count;  

    /** Test the class in shutdown mode. **/

    my_cdc_ecm.ux_host_class_cdc_ecm_state = UX_HOST_CLASS_INSTANCE_SHUTDOWN;
    transfer_request.ux_transfer_request_class_instance = &my_cdc_ecm;
    transfer_request.ux_transfer_request_completion_code = UX_SUCCESS;
    _ux_host_class_cdc_ecm_interrupt_notification(&transfer_request);

    /** Test non-network connection message.  **/

    /* Create the action to ignore the transfer request. */
    UX_TEST_ACTION action = {0};
    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_EP;
    action.req_ep_address = 0xff;
    action.no_return = 0;
    action.status = UX_SUCCESS;
    ux_test_add_action_to_main_list(action);

    my_cdc_ecm.ux_host_class_cdc_ecm_state = UX_HOST_CLASS_INSTANCE_LIVE;
    transfer_request_data[UX_HOST_CLASS_CDC_ECM_NPF_NOTIFICATION_TYPE] = (UCHAR)(UX_HOST_CLASS_CDC_ECM_NOTIFICATION_NETWORK_CONNECTION - 1);
    endpoint.ux_endpoint_device = cdc_ecm_host->ux_host_class_cdc_ecm_device;
    endpoint.ux_endpoint_descriptor.bEndpointAddress = 0xff;

    transfer_request.ux_transfer_request_class_instance = &my_cdc_ecm;
    transfer_request.ux_transfer_request_data_pointer = transfer_request_data;
    transfer_request.ux_transfer_request_endpoint = &endpoint;
    transfer_request.ux_transfer_request_completion_code = UX_SUCCESS;

    _ux_host_class_cdc_ecm_interrupt_notification(&transfer_request);

    /** Test receiving LINK UP while link is already up. **/
    ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, 1);

    /** Test receiving LINK UP while link is pending up. **/

    cdc_ecm_host->ux_host_class_cdc_ecm_link_state = UX_HOST_CLASS_CDC_ECM_LINK_STATE_PENDING_UP;

    /* Save the interrupt count so we know when the host has received the message. */
    notification_count = cdc_ecm_host->ux_host_class_cdc_ecm_notification_count;

    ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, 1);

    /* Now wait for the host to receive it. Luckly, there's an increment count we can check! */
    ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_notification_count, notification_count + 1);

    cdc_ecm_host->ux_host_class_cdc_ecm_link_state = UX_HOST_CLASS_CDC_ECM_LINK_STATE_UP;

    /** Test receiving LINK DOWN while link is already down. **/

    /* Send the link down event. */
    ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, 0);

    /* Wait for host to set the link to down. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, 0));

    /* Save the interrupt count so we know when the host has received the message. */
    notification_count = cdc_ecm_host->ux_host_class_cdc_ecm_notification_count;

    /* Send the link down event again. */
    ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, 0);

    /* Now wait for the host to receive it. Luckly, there's an increment count we can check! */
    ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_notification_count, notification_count + 1);

    /** Test receiving LINK DOWN while link is pending down. **/

    cdc_ecm_host->ux_host_class_cdc_ecm_link_state = UX_HOST_CLASS_CDC_ECM_LINK_STATE_PENDING_DOWN;

    /* Save the interrupt count so we know when the host has received the message. */
    notification_count = cdc_ecm_host->ux_host_class_cdc_ecm_notification_count;

    ux_test_device_class_cdc_ecm_set_link_state(cdc_ecm_device, 0);

    /* Now wait for the host to receive it. Luckly, there's an increment count we can check! */
    ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_notification_count, notification_count + 1);

    cdc_ecm_host->ux_host_class_cdc_ecm_link_state = UX_HOST_CLASS_CDC_ECM_LINK_STATE_DOWN;
}

static void post_init_device()
{
}