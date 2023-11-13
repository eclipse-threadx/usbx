/* This functions tests the ux_host_class_cdc_ecm_transmission_callback API. */

#include "usbx_ux_test_cdc_ecm.h"

static UCHAR has_host_write_failed_yet;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_host_class_cdc_ecm_transmission_callback_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running _ux_host_class_cdc_ecm_transmission_callback Test........... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

UX_TRANSFER             transfer_request;
UX_HOST_CLASS_CDC_ECM   my_cdc_ecm;
UX_ENDPOINT             endpoint;
NX_PACKET               tmp_packet;

    /** Test packet is null (WHAT ARE YOU, INSANE?). **/

    my_cdc_ecm.ux_host_class_cdc_ecm_link_state = UX_HOST_CLASS_CDC_ECM_LINK_STATE_UP;
    my_cdc_ecm.ux_host_class_cdc_ecm_state = UX_HOST_CLASS_INSTANCE_LIVE;
    my_cdc_ecm.ux_host_class_cdc_ecm_xmit_queue_head = UX_NULL;

    transfer_request.ux_transfer_request_completion_code = UX_SUCCESS;
    transfer_request.ux_transfer_request_class_instance = &my_cdc_ecm;

    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FATAL_ERROR));
    _ux_host_class_cdc_ecm_transmission_callback(&transfer_request);

    /** Test completion code failure. **/

    /* Create the action to ignore the transfer request. */
    UX_TEST_ACTION action = {0};
    action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    action.function = UX_HCD_TRANSFER_REQUEST;
    action.req_action = UX_TEST_MATCH_EP;
    action.req_ep_address = 0xff;
    action.no_return = 0;
    action.status = UX_SUCCESS;
    ux_test_add_action_to_main_list(action);

    my_cdc_ecm.ux_host_class_cdc_ecm_link_state = UX_HOST_CLASS_CDC_ECM_LINK_STATE_UP;
    my_cdc_ecm.ux_host_class_cdc_ecm_state = UX_HOST_CLASS_INSTANCE_LIVE;
    my_cdc_ecm.ux_host_class_cdc_ecm_xmit_queue_head = (NX_PACKET *)(ALIGN_TYPE)0xffffffff; /* just needs to be non-null */

    endpoint.ux_endpoint_device = cdc_ecm_host->ux_host_class_cdc_ecm_device;
    endpoint.ux_endpoint_descriptor.bEndpointAddress = 0xff;

    transfer_request.ux_transfer_request_class_instance = &my_cdc_ecm;
    transfer_request.ux_transfer_request_endpoint = &endpoint;
    transfer_request.ux_transfer_request_completion_code = UX_ERROR;

    _ux_host_class_cdc_ecm_transmission_callback(&transfer_request);

    /** Test transfer_request->ux_transfer_request_requested_length == transfer_request -> ux_transfer_request_packet_length **/
    endpoint.ux_endpoint_device->ux_device_state = UX_DEVICE_SUSPENDED;
    endpoint.ux_endpoint_descriptor.bEndpointAddress = 0;
    transfer_request.ux_transfer_request_requested_length = 512;
    transfer_request.ux_transfer_request_packet_length = 512;
    transfer_request.ux_transfer_request_class_instance = &my_cdc_ecm;
    transfer_request.ux_transfer_request_completion_code = UX_SUCCESS;

    _ux_host_class_cdc_ecm_transmission_callback(&transfer_request);

    /** Test transfer_request->ux_transfer_request_requested_length != transfer_request -> ux_transfer_request_packet_length **/
    endpoint.ux_endpoint_device->ux_device_state = UX_DEVICE_SUSPENDED;
    endpoint.ux_endpoint_descriptor.bEndpointAddress = 0;
    my_cdc_ecm.ux_host_class_cdc_ecm_link_state = UX_HOST_CLASS_CDC_ECM_LINK_STATE_UP;
    my_cdc_ecm.ux_host_class_cdc_ecm_state = UX_HOST_CLASS_INSTANCE_LIVE;
    my_cdc_ecm.ux_host_class_cdc_ecm_xmit_queue_head = &tmp_packet; /* just needs to be non-null */
    tmp_packet.nx_packet_queue_next = (NX_PACKET *)(ALIGN_TYPE)0;
    transfer_request.ux_transfer_request_requested_length = 510;
    transfer_request.ux_transfer_request_packet_length = 512;
    transfer_request.ux_transfer_request_class_instance = &my_cdc_ecm;
    transfer_request.ux_transfer_request_completion_code = UX_SUCCESS;

    _ux_host_class_cdc_ecm_transmission_callback(&transfer_request);

    /** Test transfer_request->ux_transfer_request_requested_length == 0 **/
    endpoint.ux_endpoint_device->ux_device_state = UX_DEVICE_SUSPENDED;
    endpoint.ux_endpoint_descriptor.bEndpointAddress = 0;
    my_cdc_ecm.ux_host_class_cdc_ecm_link_state = UX_HOST_CLASS_CDC_ECM_LINK_STATE_UP;
    my_cdc_ecm.ux_host_class_cdc_ecm_state = UX_HOST_CLASS_INSTANCE_LIVE;
    my_cdc_ecm.ux_host_class_cdc_ecm_xmit_queue_head = &tmp_packet; /* just needs to be non-null */
    tmp_packet.nx_packet_queue_next = (NX_PACKET *)(ALIGN_TYPE)0;
    transfer_request.ux_transfer_request_requested_length = 0;
    transfer_request.ux_transfer_request_packet_length = 512;
    transfer_request.ux_transfer_request_class_instance = &my_cdc_ecm;
    transfer_request.ux_transfer_request_completion_code = UX_SUCCESS;

    _ux_host_class_cdc_ecm_transmission_callback(&transfer_request);

    /* Ensure our action was used. */
    UX_TEST_ASSERT(ux_test_check_actions_empty() == UX_TRUE);
}

static void post_init_device()
{
}
