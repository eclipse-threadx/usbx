/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

static ULONG                   error_callback_counter;

static void count_error_callback(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{
UX_TEST_ERROR_CALLBACK_PARAMS *error = (UX_TEST_ERROR_CALLBACK_PARAMS *)params;

    // printf("error trap #%d: 0x%x, 0x%x, 0x%x\n", __LINE__, error->system_level, error->system_context, error->error_code);
    error_callback_counter ++;
}

static UX_TEST_HCD_SIM_ACTION count_on_error_trap[] = {
{   .usbx_function = UX_TEST_OVERRIDE_ERROR_CALLBACK,
    .action_func = count_error_callback,
},
{   0   }
};

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_device_class_cdc_ecm_entry_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_device_class_cdc_ecm_entry Test.......................... ");

    stepinfo("\n");

    /* Override error trap. */
    ux_test_link_hooks_from_array(count_on_error_trap);

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

UX_SLAVE_CLASS_COMMAND command;


    stepinfo(">>>>>>>>>>>>>>>>>>> Test device connect\n");
    UX_TEST_ASSERT(cdc_ecm_device != UX_NULL);

    stepinfo(">>>>>>>>>>>>>>>>>>> Test invalid entry command request\n");
    error_callback_counter = 0;
    command.ux_slave_class_command_request = 0xFF;
    UX_TEST_CHECK_CODE(UX_FUNCTION_NOT_SUPPORTED, _ux_device_class_cdc_ecm_entry(&command));
    UX_ASSERT(error_callback_counter);

    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_host done\n");
}

static void post_init_device()
{
    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_device empty\n");
}