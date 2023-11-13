/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_device_class_ccid.h"

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

#if 0
static UINT  ux_test_ccid_icc_power_on(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_icc_power_off(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_get_slot_status(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_xfr_block(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_get_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_reset_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_set_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_escape(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_icc_clock(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_t0_apdu(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_secure(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_mechanical(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_abort(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT  ux_test_ccid_set_data_rate_and_clock_frequency(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);

static UX_DEVICE_CLASS_CCID_HANDLES device_ccid_handles =
{
    ux_test_ccid_icc_power_on,
    ux_test_ccid_icc_power_off,
    ux_test_ccid_get_slot_status,
    ux_test_ccid_xfr_block,
    ux_test_ccid_get_parameters,
    ux_test_ccid_reset_parameters,
    ux_test_ccid_set_parameters,
    ux_test_ccid_escape,
    ux_test_ccid_icc_clock,
    ux_test_ccid_t0_apdu,
    ux_test_ccid_secure,
    ux_test_ccid_mechanical,
    ux_test_ccid_abort,
    ux_test_ccid_set_data_rate_and_clock_frequency,
};

static UINT ux_test_ccid_icc_power_on(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_icc_power_off(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_get_slot_status(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_xfr_block(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_get_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_reset_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_set_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_escape(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_icc_clock(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_t0_apdu(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_secure(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_mechanical(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_abort(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

static UINT  ux_test_ccid_set_data_rate_and_clock_frequency(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
    return UX_SUCCESS;
}

#endif

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_uxe_device_ccid_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_device_printer APIs Test.................................. ");
#if !defined(UX_DEVICE_CLASS_CCID_ENABLE_ERROR_CHECKING)
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
UINT                                    status;
UX_DEVICE_CLASS_CCID                    dummy_ccid_inst;
UX_DEVICE_CLASS_CCID                    *dummy_ccid = &dummy_ccid_inst;
UCHAR                                   dummy_buffer[32];
ULONG                                   dummy_actual_length;

UX_DEVICE_CLASS_CCID_PARAMETER          device_ccid_parameter;


    status = ux_device_class_ccid_icc_insert(UX_NULL, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_ccid_icc_remove(UX_NULL, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_ccid_auto_seq_done(UX_NULL, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_ccid_time_extension(UX_NULL, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_ccid_time_extension(UX_NULL, UX_DEVICE_CLASS_CCID_MAX_N_SLOTS, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = _uxe_device_class_ccid_hardware_error(UX_NULL, 0,  1);
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
