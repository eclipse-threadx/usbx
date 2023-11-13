/* This tests the case where a bus-powered hub is connected to a self-powered hub. */

#include "usbx_ux_test_hub.h"

static unsigned char device_framework_bus_powered[] = {

    /* Device Descriptor */
    0x12, /* bLength */
    0x01, /* bDescriptorType */
    0x00, 0x02, /* bcdUSB */
    0x09, /* bDeviceClass - Hub */
    0x00, /* bDeviceSubClass */
    0x01, /* bDeviceProtocol */
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
    0xa0, /* bmAttributes - Bus-powered */
    0x01, /* bMaxPower */

    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x00, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x01, /* bNumEndpoints */
    0x09, /* bInterfaceClass - Hub */
    0x00, /* bInterfaceSubClass */
    0x00, /* bInterfaceProtocol */
    0x00, /* iInterface */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x81, /* bEndpointAddress */
    0x03, /* bmAttributes - Interrupt */
    0x02, 0x00, /* wMaxPacketSize */
    0x0c, /* bInterval */

};

static DEVICE_INIT_DATA device_init_data = {
    .framework = device_framework_bus_powered,
    .framework_length = sizeof(device_framework_bus_powered),
};

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_bus_powered_hub_connected_to_self_and_bus_powered_hub_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
#if UX_MAX_DEVICES > 1
    printf("Running Hub Bus Powered Hub Connected....Test....................... ");
#else
    printf("Running Hub Bus Powered Hub Connected....Skip max 1 device.......... SUCCESS!\n");
    test_control_return(0);
    return;
#endif

    stepinfo("\n");

    initialize_hub_with_device_init_data(first_unused_memory, &device_init_data);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
UX_DEVICE parent;

    stepinfo("test this hub connected to a self-powered hub\n");

    parent.ux_device_power_source = UX_DEVICE_SELF_POWERED;

    g_hub_host->ux_host_class_hub_device->ux_device_parent = &parent;
    UX_TEST_CHECK_SUCCESS(_ux_host_class_hub_configure(g_hub_host));

    /* Should've been null originally. */
    g_hub_host->ux_host_class_hub_device->ux_device_parent = UX_NULL;

    stepinfo("test this hub connected to a bus-powered hub\n");

    parent.ux_device_power_source = UX_DEVICE_BUS_POWERED;

    g_hub_host->ux_host_class_hub_device->ux_device_parent = &parent;
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HUB, UX_CONNECTION_INCOMPATIBLE));
    UX_TEST_CHECK_NOT_SUCCESS(_ux_host_class_hub_configure(g_hub_host));

    /* Should've been null originally. */
    g_hub_host->ux_host_class_hub_device->ux_device_parent = UX_NULL;
#endif
}

static void post_init_device()
{
}