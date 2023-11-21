/* This test is designed to test the ux_utility_timer_....  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_MEMORY_SIZE     (64*1024)

/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           error_callback_ignore = UX_FALSE;
static ULONG                           error_callback_counter;

/* Define USBX test global variables.  */

static TX_THREAD           ux_test_thread_simulation_0;
static TX_THREAD           ux_test_thread_simulation_1;
static void                ux_test_thread_simulation_0_entry(ULONG);
static void                ux_test_thread_simulation_1_entry(ULONG);


/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);

/* Simulator actions. */


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}


static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        {
            /* Failed test.  */
            printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
            // test_control_return(1);
        }
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_test_USBX_6_application_define(void *first_unused_memory)
#endif
{

UINT                             status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;
CHAR                            *rpool_start;
CHAR                            *cpool_start;
ULONG                            rpool_free[2];
ULONG                            cpool_free[2];
VOID                            *ptr;
UINT                             n, i;
const CHAR                       flags[] = {
    UX_REGULAR_MEMORY, UX_CACHE_SAFE_MEMORY, 0xFF
};
const CHAR                       expect_error[] = {
    UX_FALSE, UX_FALSE, UX_TRUE
};

    /* Inform user.  */
    printf("Running USBX-6 _ux_version_id Test.................................. ");

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    rpool_start = memory_pointer;
    cpool_start = memory_pointer + UX_TEST_MEMORY_SIZE;

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(rpool_start, UX_TEST_MEMORY_SIZE, cpool_start, UX_TEST_MEMORY_SIZE);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* Create the simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_simulation_0, "test simulation", ux_test_thread_simulation_0_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}

static void  ux_test_thread_simulation_0_entry(ULONG arg)
{

    if (_ux_version_id == UX_NULL)
    {
        printf("ERROR #%d: _ux_version_id is NULL\n", __LINE__);
        error_counter ++;
    }
    else
    {

        /* Use _ux_version_id so if there is compile problem we know it.  */
        puts("\n_ux_version_id is: ");
        puts(_ux_version_id);
    }

    /* Sleep for a tick to make sure everything is complete.  */
    tx_thread_sleep(1);

    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* Test error.  */
        printf("ERROR #%d: total %ld errors\n", __LINE__, error_counter);
        test_control_return(1);
    }
    else
    {

        /* Successful test.  */
        printf("SUCCESS!\n");
        test_control_return(0);
    }
}