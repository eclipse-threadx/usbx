#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_device_stack.h"
#include "ux_host_class_hub.h"
#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"
#include "ux_test.h"
#include "ux_test_actions.h"
#include "ux_device_class_dummy_hub.h"
#include "ux_dcd_sim_slave.h"
#include "usbx_ux_test_hub.h"

static ULONG set_addr_counter;

static ULONG rsc_mem_alloc_cnt_on_set_addr;
static ULONG rsc_sem_on_set_addr;
static ULONG rsc_sem_get_on_set_addr;
static ULONG rsc_mutex_on_set_addr;

static ULONG rsc_enum_sem_usage;
static ULONG rsc_enum_sem_get_count;
static ULONG rsc_enum_mutex_usage;
static ULONG rsc_enum_mem_alloc_count;

static ULONG rsc_cdc_sem_usage;
static ULONG rsc_cdc_sem_get_count;
static ULONG rsc_cdc_mutex_usage;
static ULONG rsc_cdc_mem_alloc_count;

static ULONG error_callback_counter;

static UX_TEST_SETUP _SetAddress = UX_TEST_SETUP_SetAddress;
static VOID ux_test_hcd_entry_set_addr(UX_TEST_ACTION *action, VOID *_params)
{

    set_addr_counter ++;

    rsc_mem_alloc_cnt_on_set_addr = ux_test_utility_sim_mem_alloc_count();

    rsc_sem_on_set_addr = ux_test_utility_sim_sem_create_count();
    rsc_sem_get_on_set_addr = ux_test_utility_sim_sem_get_count();
    rsc_mutex_on_set_addr = ux_test_utility_sim_mutex_create_count();
}
static UX_TEST_HCD_SIM_ACTION log_on_SetAddr[] = {
{   .function = UX_HCD_TRANSFER_REQUEST,
    .req_setup = &_SetAddress,
    .req_action = UX_TEST_SETUP_MATCH_REQ,
    .action_func = ux_test_hcd_entry_set_addr,
    .no_return = UX_TRUE
}, /* Invoke callback & continue */
{   0   }
};

static void count_error_callback(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{
UX_TEST_ERROR_CALLBACK_PARAMS *error = (UX_TEST_ERROR_CALLBACK_PARAMS *)params;

    // printf("error trap: 0x%x, 0x%x, 0x%x\n", error->system_level, error->system_context, error->error_code);
    error_callback_counter ++;
}

static UX_TEST_HCD_SIM_ACTION count_on_error_trap[] = {
{   .usbx_function = UX_TEST_OVERRIDE_ERROR_CALLBACK,
    .action_func = count_error_callback,
},
{   0   }
};

static UINT  sleep_break_on_error(VOID)
{

    if (error_callback_counter >= 3)
        return error_callback_counter;

    return UX_SUCCESS;
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_hub_basic_memory_test_application_define(void *first_unused_memory)
#endif
{

CHAR *memory_pointer = first_unused_memory;

    /* Inform user.  */
    printf("Running Hub Basic Memory Functionality Test......................... ");

    stepinfo("\n");

    /* Override error trap. */
    ux_test_link_hooks_from_array(count_on_error_trap);

    /* Reset testing counts. */
    ux_test_utility_sim_mem_alloc_log_enable(UX_TRUE);
    ux_test_utility_sim_mem_alloc_count_reset();
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    /* Reset error generations */
    ux_test_utility_sim_sem_error_generation_stop();
    ux_test_utility_sim_mutex_error_generation_stop();
    ux_test_utility_sim_sem_get_error_generation_stop();

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{

ULONG               mem_free;
ULONG               test_n;

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_disconnect_slave();
    ux_test_disconnect_host_wait_for_enum_completion();

    /* Reset testing counts. */
    ux_test_utility_sim_mem_alloc_count_reset();
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    ux_test_hcd_sim_host_set_actions(log_on_SetAddr);

    /* Save free memory usage. */
    mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    stepinfo(">>>>>>>>>>>>>>>>>>> Test connect\n");
    /* Connect. */
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    class_hub_get();

    /* Log create counts for further tests. */
    rsc_enum_mutex_usage = rsc_mutex_on_set_addr;
    rsc_enum_sem_usage = rsc_sem_on_set_addr;
    rsc_enum_mem_alloc_count = rsc_mem_alloc_cnt_on_set_addr;
    /* Log create counts when instances active for further tests. */
    rsc_cdc_mutex_usage = ux_test_utility_sim_mutex_create_count() - rsc_enum_mutex_usage;
    rsc_cdc_sem_usage = ux_test_utility_sim_sem_create_count() - rsc_enum_sem_usage;
    rsc_cdc_mem_alloc_count = ux_test_utility_sim_mem_alloc_count() - rsc_enum_mem_alloc_count;

    /* Lock log base for tests. */
    ux_test_utility_sim_mem_alloc_log_lock();

    /* Simulate detach and attach for FS enumeration,
       and check if there is memory error in normal enumeration.
     */
    stepinfo(">>>>>>>>>>>>>>>>>>> Enumeration test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < 3; test_n++)
    {
        stepinfo("%4ld / 2\n", test_n);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #%d.%ld: Memory level different after re-enumerations %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            test_control_return(1);
        }

        /* Connect. */
        error_callback_counter = 0;
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        /* Wait and break on error. */
        ux_test_breakable_sleep(200, sleep_break_on_error);

        /* Check */
        if (g_hub_host_from_system_change_function == UX_NULL)
        {

            printf("ERROR #%d.%ld: Enumeration fail\n", __LINE__, test_n);
            test_control_return(1);
        }
    }

    stepinfo("enum mem: %ld\n", rsc_enum_mem_alloc_count);
    stepinfo("cdc mem : %ld\n", rsc_cdc_mem_alloc_count);
    stepinfo("mem free: %ld, %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available);

    /* Simulate detach and attach for FS enumeration,
       and test possible memory allocation error handlings.
     */
    if (rsc_cdc_mem_alloc_count) stepinfo(">>>>>>>>>>>>>>>>>>> Memory errors enumeration test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < rsc_cdc_mem_alloc_count; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_cdc_mem_alloc_count - 1);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #%d.%ld: Memory level different after re-enumerations %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            test_control_return(1);
        }

        /* Set memory error generation */
        ux_test_utility_sim_mem_alloc_error_generation_start(test_n + rsc_enum_mem_alloc_count);

        /* Connect. */
        error_callback_counter = 0;
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        /* Wait and break on errors. */
        ux_test_breakable_sleep(200, sleep_break_on_error);

        /* Check error */
        if (g_hub_host_from_system_change_function != UX_NULL)
        {

            printf("ERROR #%d.%ld: device detected when there is memory error\n", __LINE__, test_n);
            test_control_return(1);
        }
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_mem_alloc_error_generation_stop();
    if (rsc_cdc_mem_alloc_count) stepinfo("\n");
}

static void post_init_device(ULONG input)
{
}