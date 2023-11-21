/* This tests the CDC-ECM entry function. */

#include "usbx_ux_test_cdc_ecm.h"

static UCHAR cdc_ecm_thread_suspended;

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_host_class_cdc_ecm_entry_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_host_class_cdc_ecm_entry Test............................ ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

UX_HOST_CLASS_COMMAND command = {0};

    /* Test unknown command. */
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED));
    command.ux_host_class_command_request = 0xff;
    UX_TEST_ASSERT(_ux_host_class_cdc_ecm_entry(&command) != UX_SUCCESS);

    /* Test command class is data, but subclass is non-zero. */
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_QUERY;
    command.ux_host_class_command_usage = UX_HOST_CLASS_COMMAND_USAGE_CSP;
    command.ux_host_class_command_class = UX_HOST_CLASS_CDC_DATA_CLASS;
    command.ux_host_class_command_subclass = 1; /* non-zero */
    UX_TEST_ASSERT(_ux_host_class_cdc_ecm_entry(&command) != UX_SUCCESS);

    /* Test command class is control, but subclass is not control subclass. */
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_QUERY;
    command.ux_host_class_command_usage = UX_HOST_CLASS_COMMAND_USAGE_CSP;
    command.ux_host_class_command_class = UX_HOST_CLASS_CDC_CONTROL_CLASS;
    command.ux_host_class_command_subclass = UX_HOST_CLASS_CDC_ECM_CONTROL_SUBCLASS - 1;
    UX_TEST_ASSERT(_ux_host_class_cdc_ecm_entry(&command) != UX_SUCCESS);

    /* Test IAD with class = zero, but subclass = non-zero. */
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_QUERY;
    command.ux_host_class_command_usage = UX_HOST_CLASS_COMMAND_USAGE_CSP;
    command.ux_host_class_command_class = UX_HOST_CLASS_CDC_CONTROL_CLASS;
    command.ux_host_class_command_subclass = UX_HOST_CLASS_CDC_ECM_CONTROL_SUBCLASS;
    command.ux_host_class_command_iad_class = 0;
    command.ux_host_class_command_iad_subclass = 1;
    UX_TEST_ASSERT(_ux_host_class_cdc_ecm_entry(&command) != UX_SUCCESS);

    /* Test IAD wrong class. */
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_QUERY;
    command.ux_host_class_command_usage = UX_HOST_CLASS_COMMAND_USAGE_CSP;
    command.ux_host_class_command_class = UX_HOST_CLASS_CDC_CONTROL_CLASS;
    command.ux_host_class_command_subclass = UX_HOST_CLASS_CDC_ECM_CONTROL_SUBCLASS;
    command.ux_host_class_command_iad_class = 1;
    UX_TEST_ASSERT(_ux_host_class_cdc_ecm_entry(&command) != UX_SUCCESS);

    /* Test IAD wrong subclass. */
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_QUERY;
    command.ux_host_class_command_usage = UX_HOST_CLASS_COMMAND_USAGE_CSP;
    command.ux_host_class_command_class = UX_HOST_CLASS_CDC_CONTROL_CLASS;
    command.ux_host_class_command_subclass = UX_HOST_CLASS_CDC_ECM_CONTROL_SUBCLASS;
    command.ux_host_class_command_iad_class = UX_HOST_CLASS_CDC_CONTROL_CLASS;
    command.ux_host_class_command_iad_subclass = UX_HOST_CLASS_CDC_ECM_CONTROL_SUBCLASS - 1;
    UX_TEST_ASSERT(_ux_host_class_cdc_ecm_entry(&command) != UX_SUCCESS);
}

static void post_init_device()
{
}