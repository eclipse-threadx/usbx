/* This tests general cases in the mac address string like hexadecimal values. */

#include "usbx_ux_test_cdc_ecm.h"

static unsigned char mac_address_test_string_framework[] = {

    /* Manufacturer string descriptor : Index 1 - "Express Logic" */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72, 0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 - "EL CDCECM Device" */
        0x09, 0x04, 0x02, 0x10,
        0x45, 0x4c, 0x20, 0x43, 0x44, 0x43, 0x45, 0x43,
        0x4d, 0x20, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65,

    /* Serial Number string descriptor : Index 3 - "0001" */
        0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31,

    /* MAC Address string descriptor : Index 4 - "001E5841B878" */
        0x09, 0x04, 0x04, 0x0C,
        'A', 'A', /* Test capital upper and lower element. */
        '0', '1', 
        '2', '3', 
        '4', 'b', 
        'c', 'D', 
        'e', 'f', 

};

static DEVICE_INIT_DATA device_init_data = {
    .string_framework = mac_address_test_string_framework,
    .string_framework_length = sizeof(mac_address_test_string_framework),
};

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_mac_address_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC-ECM Mac Address Test.................................... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize_use_framework(first_unused_memory, &device_init_data);
}

static void post_init_host()
{
    /* Ensure the mac address is correct. */
    UX_TEST_ASSERT(cdc_ecm_host->ux_host_class_cdc_ecm_node_id[0] == 0xaa);
    UX_TEST_ASSERT(cdc_ecm_host->ux_host_class_cdc_ecm_node_id[1] == 0x01);
    UX_TEST_ASSERT(cdc_ecm_host->ux_host_class_cdc_ecm_node_id[2] == 0x23);
    UX_TEST_ASSERT(cdc_ecm_host->ux_host_class_cdc_ecm_node_id[3] == 0x4b);
    UX_TEST_ASSERT(cdc_ecm_host->ux_host_class_cdc_ecm_node_id[4] == 0xcd);
    UX_TEST_ASSERT(cdc_ecm_host->ux_host_class_cdc_ecm_node_id[5] == 0xef);

    /* Configuration descriptor invalid while parsing MAC address */
    /* descriptor_length < 3  */
    default_device_framework[18] = 0;
    UX_TEST_ASSERT(UX_DESCRIPTOR_CORRUPTED == _ux_host_class_cdc_ecm_mac_address_get(cdc_ecm_host));
    /* descriptor_length > total_configuration_length  */
    default_device_framework[18] = 0xFF;
    UX_TEST_ASSERT(UX_DESCRIPTOR_CORRUPTED == _ux_host_class_cdc_ecm_mac_address_get(cdc_ecm_host));
    /* Restore.  */
    default_device_framework[18] = 9;
    UX_TEST_ASSERT(UX_SUCCESS == _ux_host_class_cdc_ecm_mac_address_get(cdc_ecm_host));

}

static void post_init_device()
{
}