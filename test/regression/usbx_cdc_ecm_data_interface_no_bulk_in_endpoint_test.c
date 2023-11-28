/* This tests the case where there is no bulk in endpoint in the CDC-ECM data
   interface. Host should report an error.
   
   Unfortunately, the device fails during activation if we use the 'no bulk-in
   endpoint' descriptor, so we have the device use a correct descriptor, then
   force the host to receive the invalid descriptor when it sends the GetDescriptor
   request for the configuration descriptor. */

#include "usbx_ux_test_cdc_ecm.h"

static unsigned char no_bulk_in_endpoint_framework[] = {

    /* Configuration Descriptor */
    0x09, /* bLength */
    0x02, /* bDescriptorType */
    0x58, 0x00, /* wTotalLength */
    0x02, /* bNumInterfaces */
    0x01, /* bConfigurationValue */
    0x00, /* iConfiguration */
    0xc0, /* bmAttributes - Self-powered */
    0x00, /* bMaxPower */

    /* Interface Association Descriptor */
    0x08, /* bLength */
    0x0b, /* bDescriptorType */
    0x00, /* bFirstInterface */
    0x02, /* bInterfaceCount */
    0x02, /* bFunctionClass - CDC - Communication */
    0x06, /* bFunctionSubClass - ECM */
    0x00, /* bFunctionProtocol - No class specific protocol required */
    0x00, /* iFunction */

    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x00, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x01, /* bNumEndpoints */
    0x02, /* bInterfaceClass - CDC - Communication */
    0x06, /* bInterfaceSubClass - ECM */
    0x00, /* bInterfaceProtocol - No class specific protocol required */
    0x00, /* iInterface */

    /* CDC Header Functional Descriptor */
    0x05, /* bLength */
    0x24, /* bDescriptorType */
    0x00, /* bDescriptorSubType */
    0x10, 0x01, /* bcdCDC */

    /* CDC ECM Functional Descriptor */
    0x0d, /* bLength */
    0x24, /* bDescriptorType */
    0x0f, /* bDescriptorSubType */
    0x04, /* iMACAddress */
    0x00, 0x00, 0x00, 0x00, /* bmEthernetStatistics */
    0xea, 0x05, /* wMaxSegmentSize */
    0x00, 0x00, /* wNumberMCFilters */
    0x00, /* bNumberPowerFilters */

    /* CDC Union Functional Descriptor */
    0x05, /* bLength */
    0x24, /* bDescriptorType */
    0x06, /* bDescriptorSubType */
    0x00, /* bmMasterInterface */
    0x01, /* bmSlaveInterface0 */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x83, /* bEndpointAddress */
    0x03, /* bmAttributes - Interrupt */
    0x08, 0x00, /* wMaxPacketSize */
    0x08, /* bInterval */

    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x01, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x00, /* bNumEndpoints */
    0x0a, /* bInterfaceClass - CDC - Data */
    0x00, /* bInterfaceSubClass - Should be 0x00 */
    0x00, /* bInterfaceProtocol - No class specific protocol required */
    0x00, /* iInterface */

    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x01, /* bInterfaceNumber */
    0x01, /* bAlternateSetting */
    0x02, /* bNumEndpoints */
    0x0a, /* bInterfaceClass - CDC - Data */
    0x00, /* bInterfaceSubClass - Should be 0x00 */
    0x00, /* bInterfaceProtocol - No class specific protocol required */
    0x00, /* iInterface */

    /* We use two bulk-out endpoints so that the lengths are the same (makes things easier). */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x02, /* bEndpointAddress */
    0x02, /* bmAttributes - Bulk */
    0x40, 0x00, /* wMaxPacketSize */
    0x00, /* bInterval */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x01, /* bEndpointAddress */
    0x02, /* bmAttributes - Bulk */
    0x40, 0x00, /* wMaxPacketSize */
    0x00, /* bInterval */

};

static DEVICE_INIT_DATA device_init_data = {
    .framework = default_device_framework, 
    .framework_length = sizeof(default_device_framework), 
    .dont_register_hcd = 1,
};

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_data_interface_no_bulk_in_endpoint_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM No Bulk In Endpiont Test............................ ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize_use_framework(first_unused_memory, &device_init_data);
}

static void post_init_host()
{

int i;

    /* We need to intercept the configuration descriptor. */
    UX_TEST_ACTION change_config_desc_action = {0};
    change_config_desc_action.usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY;
    change_config_desc_action.function = UX_HCD_TRANSFER_REQUEST;
    change_config_desc_action.req_action = UX_TEST_MATCH_REQ_LEN | UX_TEST_SIM_REQ_ANSWER;
    change_config_desc_action.req_requested_len = 0x58; /* Length of config descriptor. */
    change_config_desc_action.req_data = no_bulk_in_endpoint_framework;
    change_config_desc_action.req_actual_len = sizeof(no_bulk_in_endpoint_framework);
    change_config_desc_action.req_status = UX_SUCCESS;
    change_config_desc_action.do_after = 0;
    change_config_desc_action.no_return = 0;

    /* We expect this to fail multiple times since enumeration tries multiple times. */
    for (i = 0; i < UX_RH_ENUMERATION_RETRY; i++)
    {
        ux_test_add_action_to_main_list(change_config_desc_action);
        ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_ENDPOINT_HANDLE_UNKNOWN));
    }
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ROOT_HUB, UX_DEVICE_ENUMERATION_FAILURE));

    /* Enumerate. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize, 0, 0));

    /* The HCD init function put()s the HCD semaphore, so we can do this here. */
    ux_test_wait_for_enum_thread_completion();

    /* Enumeration should've failed.  */
    UX_TEST_ASSERT(ux_test_check_actions_empty());
    UX_TEST_ASSERT(cdc_ecm_host_from_system_change_function == UX_NULL);
}

static void post_init_device()
{
}