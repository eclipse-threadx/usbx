/* This tests the case where the hub has multiple TTs. Specific test case
   is in ux_host_class_hub_descriptor_get.c. */

#include "usbx_ux_test_hub.h"

static unsigned char framework_multiple_tts[] = {

    /* Device Descriptor */
    0x12, /* bLength */
    0x01, /* bDescriptorType */
    0x00, 0x02, /* bcdUSB */
    0x09, /* bDeviceClass - Hub */
    0x00, /* bDeviceSubClass */
    0x02, /* bDeviceProtocol - multiple TTs */
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

static DEVICE_INIT_DATA device_init_data = {
    .framework = framework_multiple_tts,
    .framework_length = sizeof(framework_multiple_tts),
};

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_multiple_tt_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub With Multiple TT Test................................... ");

    stepinfo("\n");

    initialize_hub_with_device_init_data(first_unused_memory, &device_init_data);
}

static void post_init_host()
{
}

static void post_init_device()
{
}