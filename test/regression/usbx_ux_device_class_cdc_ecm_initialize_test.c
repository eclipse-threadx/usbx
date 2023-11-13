/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

static ULONG                rsc_cdc_thread_usage;
static ULONG                rsc_cdc_mutex_usage;
static ULONG                rsc_cdc_event_usage;
static ULONG                rsc_cdc_sem_usage;

static ULONG                error_callback_counter;

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
void usbx_ux_device_class_cdc_ecm_initialize_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_device_class_cdc_ecm_initialize Test..................... ");

    stepinfo("\n");

    /* Override error trap. */
    ux_test_link_hooks_from_array(count_on_error_trap);

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{

UINT                                                status;
ULONG                                               mem_free;
ULONG                                               test_n;


    /* Test device deinit. */
    stepinfo(">>>>>>>>>>>>>>>>>>> Test class deinit\n");
    ux_test_disconnect_slave();
    ux_test_disconnect_host_wait_for_enum_completion();
    ux_device_stack_class_unregister(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry);

    /* Reset testing counts. */
    ux_test_utility_sim_thread_create_count_reset();
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_event_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();

    stepinfo(">>>>>>>>>>>>>>>>>>> Test class init\n");
    UX_TEST_CHECK_SUCCESS(ux_device_stack_class_register(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry, 1, 0, &cdc_ecm_parameter));

    /* Log create counts for further tests. */
    rsc_cdc_thread_usage = ux_test_utility_sim_thread_create_count();
    rsc_cdc_mutex_usage = ux_test_utility_sim_mutex_create_count();
    rsc_cdc_event_usage = ux_test_utility_sim_event_create_count();
    rsc_cdc_sem_usage = ux_test_utility_sim_sem_create_count();

    if (rsc_cdc_thread_usage) stepinfo(">>>>>>>>>>>>>>>>>>> Thread errors cdc_ecm init test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < rsc_cdc_thread_usage; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_cdc_thread_usage - 1);

        /* Deinit. */
        ux_device_stack_class_unregister(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry);

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #%d.%ld: Memory level different after re-init %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            test_control_return(1);
        }

        /* Set thread error generation */
        ux_test_utility_sim_thread_error_generation_start(test_n);

        /* Init. */

        status = ux_device_stack_class_register(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry, 1, 0, &cdc_ecm_parameter);

        /* Check error */
        if (status != UX_THREAD_ERROR && status != NX_PTR_ERROR)
        {

            printf("ERROR #%d.%ld: code 0x%x\n", __LINE__, test_n, status);
            test_control_return(1);
        }
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_thread_error_generation_stop();
    if (rsc_cdc_thread_usage) stepinfo("\n");

    if (rsc_cdc_mutex_usage) stepinfo(">>>>>>>>>>>>>>>>>>> Mutex errors cdc_ecm init test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < rsc_cdc_mutex_usage; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_cdc_mutex_usage - 1);

        /* Deinit. */
        ux_device_stack_class_unregister(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry);

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #%d.%ld: Memory level different after re-init %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            test_control_return(1);
        }

        /* Set thread error generation */
        ux_test_utility_sim_mutex_error_generation_start(test_n);

        /* Init. */

        status = ux_device_stack_class_register(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry, 1, 0, &cdc_ecm_parameter);

        /* Check error */
        if (status != UX_MUTEX_ERROR)
        {

            printf("ERROR #%d.%ld: code 0x%x\n", __LINE__, test_n, status);
            test_control_return(1);
        }
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_mutex_error_generation_stop();
    if (rsc_cdc_mutex_usage) stepinfo("\n");

    if (rsc_cdc_sem_usage) stepinfo(">>>>>>>>>>>>>>>>>>> Semaphore errors cdc_ecm init test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < rsc_cdc_sem_usage; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_cdc_sem_usage - 1);

        /* Deinit. */
        ux_device_stack_class_unregister(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry);

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #%d.%ld: Memory level different after re-init %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            test_control_return(1);
        }

        /* Set thread error generation */
        ux_test_utility_sim_sem_error_generation_start(test_n);

        /* Init. */

        status = ux_device_stack_class_register(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry, 1, 0, &cdc_ecm_parameter);

        /* Check error */
        if (status != UX_SEMAPHORE_ERROR)
        {

            printf("ERROR #%d.%ld: code 0x%x\n", __LINE__, test_n, status);
            test_control_return(1);
        }
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_sem_error_generation_stop();
    if (rsc_cdc_sem_usage) stepinfo("\n");

    if (rsc_cdc_event_usage) stepinfo(">>>>>>>>>>>>>>>>>>> EventFlag errors cdc_ecm init test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < rsc_cdc_event_usage; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_cdc_event_usage - 1);

        /* Deinit. */
        ux_device_stack_class_unregister(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry);

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #%d.%ld: Memory level different after re-init %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            test_control_return(1);
        }

        /* Set thread error generation */
        ux_test_utility_sim_event_error_generation_start(test_n);

        /* Init. */

        status = ux_device_stack_class_register(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry, 1, 0, &cdc_ecm_parameter);

        /* Check error */
        if (status != UX_EVENT_ERROR)
        {

            printf("ERROR #%d.%ld: code 0x%x\n", __LINE__, test_n, status);
            test_control_return(1);
        }
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_event_error_generation_stop();
    if (rsc_cdc_event_usage) stepinfo("\n");

    stepinfo(">>>>>>>>>>>>>>>>>>> Test connect to avoid post post operation\n");
    /* Connect. */
    UX_TEST_CHECK_SUCCESS(ux_device_stack_class_register(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry, 1, 0, &cdc_ecm_parameter));
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    class_cdc_ecm_get_host();

    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_host done\n");
}

static void post_init_device()
{
    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_device empty\n");
}