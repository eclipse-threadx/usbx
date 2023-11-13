/* This tests the case where there are no endpoints. Specific test case is in
   ux_host_class_hub_interrupt_endpoint_start.c. */

#include "usbx_ux_test_hub.h"

static unsigned char framework_no_endpoints_test[] = {

    /* Device Descriptor */
    0x12, /* bLength */
    0x01, /* bDescriptorType */
    0x00, 0x02, /* bcdUSB */
    0x09, /* bDeviceClass - Hub */
    0x00, /* bDeviceSubClass */
    0x01, /* bDeviceProtocol - multiple TTs */
    0x40, /* bMaxPacketSize0 */
    0x24, 0x04, /* idVendor */
    0x12, 0x24, /* idProduct */
    0xb2, 0x0b, /* bcdDevice */
    0x00, /* iManufacturer */
    0x00, /* iProduct */
    0x00, /* iSerialNumber */
    0x01, /* bNumConfigurations */

    /* Configuration Descriptor */
    0x09, /* bLength */
    0x02, /* bDescriptorType */
    0x12, 0x00, /* wTotalLength */
    0x01, /* bNumInterfaces */
    0x01, /* bConfigurationValue */
    0x00, /* iConfiguration */
    0xe0, /* bmAttributes - Self-powered */
    0x01, /* bMaxPower */

    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x00, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x00, /* bNumEndpoints */
    0x09, /* bInterfaceClass - Hub */
    0x00, /* bInterfaceSubClass */
    0x00, /* bInterfaceProtocol */
    0x00, /* iInterface */

};

static DEVICE_INIT_DATA device_init_data = {
    .dont_enumerate = 1,
    .framework = framework_no_endpoints_test,
    .framework_length = sizeof(framework_no_endpoints_test),
};

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_no_endpoints_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub No Endpoints Test....................................... ");

    stepinfo("\n");

    initialize_hub_with_device_init_data(first_unused_memory, &device_init_data);
}

static void post_init_host()
{

    ux_test_add_action_to_main_list_multiple(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ENUMERATOR, UX_ENDPOINT_HANDLE_UNKNOWN), 3);
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ROOT_HUB, UX_DEVICE_ENUMERATION_FAILURE));

    UX_TEST_CHECK_SUCCESS(ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize, 0, 0));
    ux_test_wait_for_enum_thread_completion();

    UX_TEST_ASSERT(ux_test_check_actions_empty() == UX_TRUE);
}

static void post_init_device()
{
}