/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_host_class_prolific.h"

#include "ux_test.h"

/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_MEMORY_SIZE     (64*1024)


/* Define the counters used in the test application...  */

static ULONG                           error_counter = 0;

/* Define USBX test global variables.  */


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static TX_THREAD           ux_test_thread_simulation_0;
static void                ux_test_thread_simulation_0_entry(ULONG);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_uxe_host_prolific_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_host_prolific APIs Test................................. ");
#if !defined(UX_HOST_CLASS_PROLIFIC_ENABLE_ERROR_CHECKING)
#warning Tests skipped due to compile option!
    printf("SKIP SUCCESS!\n");
    test_control_return(0);
    return;
#endif

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
UINT                                            status;
UX_HOST_CLASS_PROLIFIC                          dummy_prolific_inst;
UX_HOST_CLASS_PROLIFIC                          *dummy_prolific = &dummy_prolific_inst;
UX_HOST_CLASS_PROLIFIC_RECEPTION                dummy_reception;
UCHAR                                           dummy_buffer[64];
ULONG                                           dummy_ul;

    /* ux_host_class_prolific_read()  */
    status = ux_host_class_prolific_read(UX_NULL, dummy_buffer, 8, &dummy_ul);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_prolific_read(dummy_prolific, UX_NULL, 8, &dummy_ul);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_prolific_read(dummy_prolific, dummy_buffer, 8, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_prolific_write()  */
    status = ux_host_class_prolific_write(UX_NULL, dummy_buffer, 8, &dummy_ul);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_prolific_write(dummy_prolific, UX_NULL, 8, &dummy_ul);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_prolific_write(dummy_prolific, dummy_buffer, 8, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_prolific_ioctl()  */
    status = ux_host_class_prolific_ioctl(UX_NULL, 0, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_prolific_command()  */
    status = ux_host_class_prolific_command(UX_NULL, 0, 0, UX_NULL, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_prolific_reception_start()  */
    status = ux_host_class_prolific_reception_start(UX_NULL, &dummy_reception);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_prolific_reception_start(dummy_prolific, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_prolific_reception_stop()  */
    status = ux_host_class_prolific_reception_stop(UX_NULL, &dummy_reception);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_prolific_reception_stop(dummy_prolific, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

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
