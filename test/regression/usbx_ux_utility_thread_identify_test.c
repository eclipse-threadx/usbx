/* This test is designed to test the ux_utility_thread_identify.  */

#include <stdio.h>
#include "tx_api.h"
#include "tx_thread.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_device_class_cdc_acm.h"
#include "ux_host_class_cdc_acm.h"

#include "ux_host_class_dpump.h"
#include "ux_device_class_dpump.h"

#include "ux_host_class_hid.h"
#include "ux_device_class_hid.h"

#include "ux_host_class_storage.h"
#include "ux_device_class_storage.h"

/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_RUN             1
#define UX_TEST_MEMORY_SIZE     (64*1024)

/* Define the counters used in the test application...  */

static CHAR                            *stack_pointer;
static CHAR                            *memory_pointer;

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           error_callback_ignore = UX_FALSE;
static ULONG                           error_callback_counter;

static UCHAR                           buffer[UX_TEST_BUFFER_SIZE];

/* Define USBX test global variables.  */

/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

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
void    usbx_ux_utility_thread_identify_test_application_define(void *first_unused_memory)
#endif
{

UINT status;


    /* Inform user.  */
    printf("Running ux_utility_thread_identify Test............................. ");

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* Create the simulation thread.  */
    status =  ux_utility_thread_create(&ux_test_thread_simulation_0, "test simulation 0", ux_test_thread_simulation_0_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d: thread create fail 0x%x\n", __LINE__, status);
        error_counter ++;
    }

    /* Reset threads. */
    _ux_utility_memory_set(&ux_test_thread_simulation_1, 0, sizeof(ux_test_thread_simulation_1));
}


static void  ux_test_thread_simulation_0_entry(ULONG arg)
{

TX_THREAD *this_thread;


    /* Create the simulation thread.  */
    this_thread =  ux_utility_thread_identify();

    /* Check for error.  */
    if (this_thread != &ux_test_thread_simulation_0)
    {

        printf("ERROR #%d: thread different %p <> %p\n", __LINE__, this_thread, &ux_test_thread_simulation_0);
        error_counter ++;
    }

    /* Simulate ISR enter */
    _tx_thread_system_state ++;

    this_thread =  ux_utility_thread_identify();

    /* Check for error.  */
    if (this_thread != UX_NULL)
    {

        printf("ERROR #%d: thread different %p <> NULL\n", __LINE__, this_thread);
        error_counter ++;
    }

    /* Simulate ISR exit.  */
    _tx_thread_system_state --;

    /* Check for errors.  */
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

static void  ux_test_thread_simulation_1_entry(ULONG arg)
{
    while(1)
    {
        _ux_utility_delay_ms(100);
    }
}
