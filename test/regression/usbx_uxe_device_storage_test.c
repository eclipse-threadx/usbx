/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_device_class_storage.h"

#include "ux_test.h"

/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_MEMORY_SIZE     (64*1024)


/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           error_callback_ignore = UX_FALSE;
static ULONG                           error_callback_counter;


/* Define USBX test global variables.  */


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static TX_THREAD           ux_test_thread_simulation_0;
static TX_THREAD           ux_test_thread_simulation_1;
static void                ux_test_thread_simulation_0_entry(ULONG);
static void                ux_test_thread_simulation_1_entry(ULONG);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_uxe_device_storage_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_device_storage APIs Test................................ ");
#if !defined(UX_DEVICE_CLASS_STORAGE_ENABLE_ERROR_CHECKING)
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


static UINT _dummy_storage_media_read(VOID *storage, ULONG lun, UCHAR *data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
    return(0);
}
static UINT _dummy_storage_media_write(VOID *storage, ULONG lun, UCHAR *data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
    return(0);
}
static UINT _dummy_storage_media_flush(VOID *storage, ULONG lun, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
    return(0);
}
static UINT _dummy_storage_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{
    return(0);
}
static UINT _dummy_storage_media_notification(VOID *storage, ULONG lun, ULONG media_id, ULONG notification_class, UCHAR **media_notification, ULONG *media_notification_length)
{
    return(0);
}

static void  ux_test_thread_simulation_0_entry(ULONG arg)
{

UINT                                    status;
UX_SLAVE_CLASS_COMMAND                  dummy_command;
UX_SLAVE_CLASS_STORAGE_PARAMETER        dummy_parameter;

    /* uxe storage initialize.  */
    dummy_command.ux_slave_class_command_parameter = &dummy_parameter;
    dummy_command.ux_slave_class_command_request = UX_SLAVE_CLASS_COMMAND_INITIALIZE;

    dummy_parameter.ux_slave_class_storage_parameter_number_lun = 0x7F;
    status = ux_device_class_storage_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    dummy_parameter.ux_slave_class_storage_parameter_number_lun = 1;

    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read = UX_NULL;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write = _dummy_storage_media_write;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_flush = _dummy_storage_media_flush;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status = _dummy_storage_media_status;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_notification = _dummy_storage_media_notification;
    status = ux_device_class_storage_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read = _dummy_storage_media_read;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write = UX_NULL;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_flush = _dummy_storage_media_flush;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status = _dummy_storage_media_status;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_notification = _dummy_storage_media_notification;
    status = ux_device_class_storage_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read = _dummy_storage_media_read;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write = _dummy_storage_media_write;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_flush = _dummy_storage_media_flush;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status = UX_NULL;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_notification = _dummy_storage_media_notification;
    status = ux_device_class_storage_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

#if defined(UX_SLAVE_CLASS_STORAGE_INCLUDE_MMC)
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read = _dummy_storage_media_read;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write = _dummy_storage_media_write;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_flush = _dummy_storage_media_flush;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status = _dummy_storage_media_status;
    dummy_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_notification = UX_NULL;
    status = ux_device_class_storage_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
#endif

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
