/* This tests the case where the host sets the device's alternate setting to
   zero.  */

#include "usbx_ux_test_cdc_ecm.h"
#include "ux_host_stack.h"

static UCHAR host_ready_for_basic_test;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_ecm_alternate_setting_change_to_zero_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running CDC ECM Alternate Setting Change To Zero Test............... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

    /* Change the data interface alternate setting to zero. */
    _ux_host_stack_interface_set(cdc_ecm_host->ux_host_class_cdc_ecm_interface_data);

    /* Wait for the host to change settings, which means the device has too. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, 0));

    /* Change the data interface alternate setting back to one. */
    _ux_host_stack_interface_set(cdc_ecm_host->ux_host_class_cdc_ecm_interface_data->ux_interface_next_interface);

    /* Wait for the host to change settings, which means the device has too. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, 1));

    /* Try a basic test. */
    host_ready_for_basic_test = 1;
    cdc_ecm_basic_test(BASIC_TEST_HOST, BASIC_TEST_TCP);

    /* Huzzah! */

    /* Change the data interface alternate setting to zero again - this time, with no deactivate callback. */
    cdc_ecm_device->ux_slave_class_cdc_ecm_parameter.ux_slave_class_cdc_ecm_instance_deactivate = UX_NULL;
    _ux_host_stack_interface_set(cdc_ecm_host->ux_host_class_cdc_ecm_interface_data);

    /* Wait for the host to change settings, which means the device has too. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, 0));

    /* Huzzah! Again! */
}

static void post_init_device()
{

    /* Wait for basic test. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&host_ready_for_basic_test, 1));
    cdc_ecm_basic_test(BASIC_TEST_DEVICE, BASIC_TEST_TCP);
}