/* This tests the ux_host_class_hub_entry.c API. */

#include "usbx_ux_test_hub.h"

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_host_class_hub_entry_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_host_class_hub_entry Test................................ ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{

UX_HOST_CLASS_COMMAND command;

    /** Test unknown function. **/

    command.ux_host_class_command_request = (~0);

    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED));
    UX_TEST_ASSERT(_ux_host_class_hub_entry(&command) != UX_SUCCESS);
}

static void post_init_device()
{
}