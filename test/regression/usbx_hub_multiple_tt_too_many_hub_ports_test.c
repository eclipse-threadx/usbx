/* This tests the case where the hub reports an invalid device protocol. Specific 
   test case is in ux_host_class_hub_descriptor_get.c. */

#include "usbx_ux_test_hub.h"

static unsigned char framework_invalid_device_protocol[] = {

    /* Device Descriptor */
    0x12, /* bLength */
    0x01, /* bDescriptorType */
    0x00, 0x02, /* bcdUSB */
    0x09, /* bDeviceClass - Hub */
    0x00, /* bDeviceSubClass */
    0x02, /* bDeviceProtocol - multiple */
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
    0x19, 0x00, /* wTotalLength */
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
    0x01, /* bNumEndpoints */
    0x09, /* bInterfaceClass - Hub */
    0x00, /* bInterfaceSubClass */
    0x01, /* bInterfaceProtocol */
    0x00, /* iInterface */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x81, /* bEndpointAddress */
    0x03, /* bmAttributes - Interrupt */
    0x02, 0x00, /* wMaxPacketSize */
    0x0c, /* bInterval */

};

static unsigned char hub_descriptor_too_many_ports[] = {

    /* Hub Descriptor */
    0x09, /* bLength */
    0x29, /* bDescriptorType */
    100, /* bNbrPorts */
    0x09, 0x00, /* wHubCharacteristics */
    0x32, /* bPwrOn2PwrGood */
    0x01, /* bHubContrCurrent */
    0x00, /* DeviceRemovable */
    0xff, /* PortPwrCtrlMask */

};

static DEVICE_INIT_DATA device_init_data = {
    .dont_enumerate = 1,
    .framework = framework_invalid_device_protocol,
    .framework_length = sizeof(framework_invalid_device_protocol),
	.hub_descriptor = hub_descriptor_too_many_ports,
	.hub_descriptor_length = sizeof(hub_descriptor_too_many_ports),
};

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_too_many_hub_ports_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Too Many Hub Ports Test................................. ");

    stepinfo("\n");

    initialize_hub_with_device_init_data(first_unused_memory, &device_init_data);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
int i;

    /* We expect error to happen multiple times. */
    for (i = 0; i < UX_RH_ENUMERATION_RETRY; i++)
    {
        ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HUB, UX_TOO_MANY_HUB_PORTS));
    }
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ROOT_HUB, UX_DEVICE_ENUMERATION_FAILURE));

    /* Enumerate. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize, 0, 0));
    ux_test_wait_for_enum_thread_completion();

    /* Ensure all expected actions occurred. */
    UX_TEST_ASSERT(ux_test_check_actions_empty());
    UX_TEST_ASSERT(g_hub_host_from_system_change_function == UX_NULL);
#endif
}

static void post_init_device()
{
}