/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

static UCHAR device_is_finished;

#define CFG_DESC_POS (0x12)
static unsigned char local_device_framework[] = {

    /* Device Descriptor */
    0x12, /* bLength */
    0x01, /* bDescriptorType */
    0x10, 0x01, /* bcdUSB */
    0xef, /* bDeviceClass - Depends on bDeviceSubClass */
    0x02, /* bDeviceSubClass - Depends on bDeviceProtocol */
    0x01, /* bDeviceProtocol - There's an IAD */
    0x40, /* bMaxPacketSize0 */
    0x70, 0x07, /* idVendor */
    0x42, 0x10, /* idProduct */
    0x00, 0x01, /* bcdDevice */
    0x01, /* iManufacturer */
    0x02, /* iProduct */
    0x03, /* iSerialNumber */
    0x01, /* bNumConfigurations */

    /* Configuration Descriptor @ 18 */
    0x09, /* bLength */
    0x02, /* bDescriptorType */
    0x79, 0x00, /* wTotalLength */
    0x02, /* bNumInterfaces */
    0x01, /* bConfigurationValue */
    0x00, /* iConfiguration */
    0xc0, /* bmAttributes - Self-powered */
    0x00, /* bMaxPower */

    /* Interface Association Descriptor @ 18+9=27 */
    0x08, /* bLength */
    0x0b, /* bDescriptorType */
    0x00, /* bFirstInterface */
    0x03, /* bInterfaceCount */
    0x02, /* bFunctionClass - CDC - Communication */
    0x06, /* bFunctionSubClass - ECM */
    0x00, /* bFunctionProtocol - No class specific protocol required */
    0x00, /* iFunction */

    /* Interface Descriptor @ 27+8=35 */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x00, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x01, /* bNumEndpoints */
    0x02, /* bInterfaceClass - CDC - Communication */
    0x06, /* bInterfaceSubClass - ECM */
    0x00, /* bInterfaceProtocol - No class specific protocol required */
    0x00, /* iInterface */

    /* CDC Header Functional Descriptor @ 35+9=44 */
    0x05, /* bLength */
    0x24, /* bDescriptorType */
    0x00, /* bDescriptorSubType */
    0x10, 0x01, /* bcdCDC */

    /* CDC ECM Functional Descriptor @ 44+5=49 */
    0x0d, /* bLength */
    0x24, /* bDescriptorType */
    0x0f, /* bDescriptorSubType */
    0x04, /* iMACAddress */
    0x00, 0x00, 0x00, 0x00, /* bmEthernetStatistics */
    0xea, 0x05, /* wMaxSegmentSize */
    0x00, 0x00, /* wNumberMCFilters */
    0x00, /* bNumberPowerFilters */

    /* CDC Union Functional Descriptor @ 49+13=62 */
    0x06, /* bLength */
    0x24, /* bDescriptorType */
    0x06, /* bDescriptorSubType */
    0x00, /* bmMasterInterface */
    0x01, /* bmSlaveInterface0 */
    0x02, /* bmSlaveInterface1 */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x83, /* bEndpointAddress */
    0x03, /* bmAttributes - Interrupt */
    0x08, 0x00, /* wMaxPacketSize */
    0x08, /* bInterval */

    /* Data 0 (9+9+7+7=32) */
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
    0x81, /* bEndpointAddress */
    0x02, /* bmAttributes - Bulk */
    0x40, 0x00, /* wMaxPacketSize */
    0x00, /* bInterval */

    /* Data 1 (9+9+7+7=32) */
    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x02, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x00, /* bNumEndpoints */
    0x0a, /* bInterfaceClass - CDC - Data */
    0x00, /* bInterfaceSubClass - Should be 0x00 */
    0x00, /* bInterfaceProtocol - No class specific protocol required */
    0x00, /* iInterface */

    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x02, /* bInterfaceNumber */
    0x01, /* bAlternateSetting */
    0x02, /* bNumEndpoints */
    0x0a, /* bInterfaceClass - CDC - Data */
    0x00, /* bInterfaceSubClass - Should be 0x00 */
    0x00, /* bInterfaceProtocol - No class specific protocol required */
    0x00, /* iInterface */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x04, /* bEndpointAddress */
    0x02, /* bmAttributes - Bulk */
    0x40, 0x00, /* wMaxPacketSize */
    0x00, /* bInterval */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x85, /* bEndpointAddress */
    0x02, /* bmAttributes - Bulk */
    0x40, 0x00, /* wMaxPacketSize */
    0x00, /* bInterval */
};

static UX_TEST_SETUP _GetConfigDescriptor = UX_TEST_SETUP_GetCfgDescr;
static UX_TEST_HCD_SIM_ACTION replace_configuration_descriptor[] = {
{   .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
    .function = UX_HCD_TRANSFER_REQUEST,
    .req_setup = &_GetConfigDescriptor,
    .req_action = UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V,
    .req_ep_address = 0,
    .req_data = (local_device_framework + CFG_DESC_POS),
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
    .req_data = (local_device_framework + CFG_DESC_POS),
    .req_actual_len = sizeof(local_device_framework) - CFG_DESC_POS,
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
    .req_data = (local_device_framework + CFG_DESC_POS),
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
    .req_data = (local_device_framework + CFG_DESC_POS),
    .req_actual_len = sizeof(local_device_framework) - CFG_DESC_POS,
    .req_status = UX_SUCCESS,
    .status = UX_SUCCESS,
    .action_func = UX_NULL,
    .no_return = UX_FALSE
},
{   .usbx_function = UX_TEST_OVERRIDE_ERROR_CALLBACK,
    .system_level = UX_SYSTEM_LEVEL_THREAD,
    .system_context = UX_SYSTEM_CONTEXT_CLASS,
    .error_code = UX_DESCRIPTOR_CORRUPTED,
    .no_return = 1,
},
{   .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
    .function = UX_HCD_TRANSFER_REQUEST,
    .req_setup = &_GetConfigDescriptor,
    .req_action = UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V,
    .req_ep_address = 0,
    .req_data = (local_device_framework + CFG_DESC_POS),
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
    .req_data = (local_device_framework + CFG_DESC_POS),
    .req_actual_len = sizeof(local_device_framework) - CFG_DESC_POS,
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
    .req_data = (local_device_framework + CFG_DESC_POS),
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
    .req_data = (local_device_framework + CFG_DESC_POS),
    .req_actual_len = sizeof(local_device_framework) - CFG_DESC_POS,
    .req_status = UX_SUCCESS,
    .status = UX_SUCCESS,
    .action_func = UX_NULL,
    .no_return = UX_FALSE
},
{   .usbx_function = UX_TEST_OVERRIDE_ERROR_CALLBACK,
    .system_level = UX_SYSTEM_LEVEL_THREAD,
    .system_context = UX_SYSTEM_CONTEXT_CLASS,
    .error_code = UX_DESCRIPTOR_CORRUPTED,
    .no_return = 1,
},
{   .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
    .function = UX_HCD_TRANSFER_REQUEST,
    .req_setup = &_GetConfigDescriptor,
    .req_action = UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V,
    .req_ep_address = 0,
    .req_data = (local_device_framework + CFG_DESC_POS),
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
    .req_data = (local_device_framework + CFG_DESC_POS),
    .req_actual_len = sizeof(local_device_framework) - CFG_DESC_POS,
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
    .req_data = (local_device_framework + CFG_DESC_POS),
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
    .req_data = (local_device_framework + CFG_DESC_POS),
    .req_actual_len = sizeof(local_device_framework) - CFG_DESC_POS,
    .req_status = UX_SUCCESS,
    .status = UX_SUCCESS,
    .action_func = UX_NULL,
    .no_return = UX_FALSE
},
{   .usbx_function = UX_TEST_OVERRIDE_ERROR_CALLBACK,
    .system_level = UX_SYSTEM_LEVEL_THREAD,
    .system_context = UX_SYSTEM_CONTEXT_CLASS,
    .error_code = UX_DESCRIPTOR_CORRUPTED,
    .no_return = 1,
},
{   .usbx_function = UX_TEST_OVERRIDE_ERROR_CALLBACK,
    .system_level = UX_SYSTEM_LEVEL_THREAD,
    .system_context = UX_SYSTEM_CONTEXT_ROOT_HUB,
    .error_code = UX_DEVICE_ENUMERATION_FAILURE,
    .no_return = 1,
},
{   0   }
};

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_msrc_81206_81225_cdc_ecm_2_data_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running MSRC 81206 & 81225 - CDC ECM 2 Data IFC Test................ ");

    stepinfo("\n");

    // ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_DESCRIPTOR_CORRUPTED));
    // ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_DESCRIPTOR_CORRUPTED));
    // ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_DESCRIPTOR_CORRUPTED));
    // ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ROOT_HUB, UX_DEVICE_ENUMERATION_FAILURE));
    ux_test_cdc_ecm_initialize_use_framework(first_unused_memory, &default_device_init_data);
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
ULONG   mem_free;
UINT    i;

    /* Wait connection.  */
    UX_TEST_CHECK_SUCCESS(_wait_host_inst_change_to(100, 1));

    /* Disconnect. */
    stepinfo(">>>>>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_disconnect_slave();
    ux_test_disconnect_host_wait_for_enum_completion();
    UX_TEST_CHECK_SUCCESS(_wait_host_inst_change_to(100, 0));

    /* Save free memory usage. */
    mem_free = ux_test_regular_memory_free();

    /* Connect.  */
    stepinfo(">>>>>>>>>>>>>>>>>>> Test connect fake descriptor\n");
    ux_test_hcd_sim_host_set_actions(replace_configuration_descriptor);
    ux_test_connect_slave_and_host_wait_for_enum_completion();
    UX_TEST_ASSERT(ux_test_check_actions_empty());
 
    for (i = 0; i < 5; i ++)
    {

        stepinfo(">>>>>>>>>>>>>>>>>>> Test connect fake descriptor memory %d\n", i);

        /* Disconnect. */
        ux_test_disconnect_slave();
        ux_test_disconnect_host_wait_for_enum_completion();

        /* Check memory level.  */
        UX_TEST_ASSERT(mem_free == ux_test_regular_memory_free());

        /* Connect.  */
        ux_test_hcd_sim_host_set_actions(replace_configuration_descriptor);
        ux_test_connect_slave_and_host_wait_for_enum_completion();
    }

    /* We're done.  */
}

static void post_init_device()
{
    device_is_finished = UX_TRUE;
}