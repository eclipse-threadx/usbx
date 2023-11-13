/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_msrc73716_cdc_ecm_mac_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running MSRC 73716 CDC ECM MAC get Test............................. ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{
UINT        status;
UX_DEVICE   *device;

    ux_test_ignore_all_errors();

    /* Modify bLength of CDC Header Functional Descriptor @ 44 (5).  */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();
    status = ux_host_stack_device_get(0, &device);
    UX_TEST_ASSERT(status == UX_DEVICE_HANDLE_UNKNOWN);
    default_device_framework[44] = 0;
    ux_test_connect_slave_and_host_wait_for_enum_completion();
    status = ux_host_stack_device_get(0, &device);
    UX_TEST_ASSERT(status == UX_SUCCESS && device -> ux_device_state != UX_DEVICE_CONFIGURED);
    default_device_framework[44] = 5;

    /* Modify bLength of CDC Header Functional Descriptor @ 44 (5).  */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();
    status = ux_host_stack_device_get(0, &device);
    UX_TEST_ASSERT(status == UX_DEVICE_HANDLE_UNKNOWN);
    default_device_framework[44] = 1;
    ux_test_connect_slave_and_host_wait_for_enum_completion();
    status = ux_host_stack_device_get(0, &device);
    UX_TEST_ASSERT(status == UX_SUCCESS && device -> ux_device_state != UX_DEVICE_CONFIGURED);
    default_device_framework[44] = 5;

    /* Modify bLength of CDC Header Functional Descriptor @ 44 (5).  */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();
    status = ux_host_stack_device_get(0, &device);
    UX_TEST_ASSERT(status == UX_DEVICE_HANDLE_UNKNOWN);
    default_device_framework[44] = sizeof(default_device_framework)+1;
    ux_test_connect_slave_and_host_wait_for_enum_completion();
    status = ux_host_stack_device_get(0, &device);
    UX_TEST_ASSERT(status == UX_SUCCESS && device -> ux_device_state != UX_DEVICE_CONFIGURED);
    default_device_framework[44] = 5;

}

static void post_init_device()
{

}