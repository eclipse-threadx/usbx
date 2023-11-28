/* This tests the case where the hub device's enumeration keeps failing. The 
   specific test case is in ux_host_class_hub_port_change_connection_process.c. */

#include "usbx_ux_test_hub.h"

static UINT num_insertions;
static UINT num_removals;

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_hub_device_enumeration_keeps_failing_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running Hub Hub Device Enumeration Kepps Failing Test............... ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{
#if UX_MAX_DEVICES > 1
int i;

    /* We do this by having the semaphore create for EP0 fail multiple times. */

    /* Create action to make semaphore creation fail. */
    UX_TEST_ACTION sem_create_fail_action = {0};
    sem_create_fail_action.usbx_function = UX_TEST_OVERRIDE_TX_SEMAPHORE_CREATE;
    sem_create_fail_action.semaphore_name = "ux_host_endpoint0_semaphore";
    sem_create_fail_action.no_return = 0;
    sem_create_fail_action.status = UX_ERROR;

    for (i = 0; i < UX_HOST_CLASS_HUB_ENUMERATION_RETRY; i++)
    {
        ux_test_add_action_to_main_list(sem_create_fail_action);
        ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_UTILITY, UX_ERROR));
    }
    ux_test_add_action_to_main_list(create_error_match_action(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ROOT_HUB, UX_DEVICE_ENUMERATION_FAILURE));

    /* Tell the host that there's a device connection. */
    connect_device_to_hub();

    /* Wait for empty actions. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_empty_actions());
#endif
}

static void post_init_device()
{
}