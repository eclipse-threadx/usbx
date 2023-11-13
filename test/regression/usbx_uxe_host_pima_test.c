/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_host_class_pima.h"

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
void usbx_uxe_host_pima_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_host_pima APIs Test.................................... ");
#if !defined(UX_HOST_CLASS_PIMA_ENABLE_ERROR_CHECKING)
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
UX_HOST_CLASS_PIMA                              dummy_pima_inst;
UX_HOST_CLASS_PIMA                             *dummy_pima = &dummy_pima_inst;
UX_HOST_CLASS_PIMA_SESSION                      pima_session;
UCHAR                                           object_buffer[64];
UX_HOST_CLASS_PIMA_OBJECT                       object;
ULONG                                           object_actual_length;
ULONG                                           storage_ids_array[32];
UX_HOST_CLASS_PIMA_STORAGE                      storage;
ULONG                                           object_handles_array[32];

    /* Unit test for function ux_host_class_pima_device_info_get() */
    status = ux_host_class_pima_device_info_get(NX_NULL, &pima_session);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_device_info_get(dummy_pima, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_object_info_send() */
    status = ux_host_class_pima_object_info_send(NX_NULL, &pima_session, 0, 0, &object);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_info_send(dummy_pima, NX_NULL, 0, 0, &object);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_info_send(dummy_pima, &pima_session, 0, 0, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Malloc fail test */
    pima_session.ux_host_class_pima_session_magic = UX_HOST_CLASS_PIMA_MAGIC_NUMBER;
    pima_session.ux_host_class_pima_session_state = UX_HOST_CLASS_PIMA_SESSION_STATE_OPENED;
    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start = UX_NULL;
    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_start = UX_NULL;
    status = ux_host_class_pima_object_info_send(dummy_pima, &pima_session, 0, 0, &object);
    UX_TEST_CHECK_CODE(UX_MEMORY_INSUFFICIENT ,status);

    /* Unit test for function ux_host_class_pima_object_info_get() */
    status = ux_host_class_pima_object_info_get(NX_NULL, &pima_session, 0, &object); 
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_info_get(dummy_pima, NX_NULL, 0, &object);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_info_get(dummy_pima, &pima_session, 0, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Malloc fail test */
    pima_session.ux_host_class_pima_session_magic = UX_HOST_CLASS_PIMA_MAGIC_NUMBER;
    pima_session.ux_host_class_pima_session_state = UX_HOST_CLASS_PIMA_SESSION_STATE_OPENED;
    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start = UX_NULL;
    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_start = UX_NULL;
    status = ux_host_class_pima_object_info_get(dummy_pima, &pima_session, 0, &object);
    UX_TEST_CHECK_CODE(UX_MEMORY_INSUFFICIENT ,status);

    /* Unit test for function ux_host_class_pima_object_open() */
    status = ux_host_class_pima_object_open(NX_NULL, &pima_session, 0, &object);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_open(dummy_pima, NX_NULL, 0, &object);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_open(dummy_pima, &pima_session, 0, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_object_get() */
    status = ux_host_class_pima_object_get(NX_NULL, &pima_session, 0, &object, object_buffer, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_get(dummy_pima, NX_NULL, 0, &object, object_buffer, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_get(dummy_pima, &pima_session, 0, NX_NULL, object_buffer, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_get(dummy_pima, &pima_session, 0, &object, NX_NULL, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_get(dummy_pima, &pima_session, 0, &object, object_buffer, 64, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_object_get() */
    status = ux_host_class_pima_object_get(NX_NULL, &pima_session, 0, &object, object_buffer, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_get(dummy_pima, NX_NULL, 0, &object, object_buffer, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_get(dummy_pima, &pima_session, 0, NX_NULL, object_buffer, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_get(dummy_pima, &pima_session, 0, &object, NX_NULL, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_get(dummy_pima, &pima_session, 0, &object, object_buffer, 64, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_thumb_get() */
    status = ux_host_class_pima_thumb_get(NX_NULL, &pima_session, 0, &object, object_buffer, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_thumb_get(dummy_pima, NX_NULL, 0, &object, object_buffer, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_thumb_get(dummy_pima, &pima_session, 0, NX_NULL, object_buffer, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_thumb_get(dummy_pima, &pima_session, 0, &object, NX_NULL, 64, &object_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_thumb_get(dummy_pima, &pima_session, 0, &object, object_buffer, 64, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_object_send() */
    status = ux_host_class_pima_object_send(NX_NULL, &pima_session, &object, object_buffer, 64);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_send(dummy_pima, NX_NULL, &object, object_buffer, 64);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_send(dummy_pima, &pima_session, NX_NULL, object_buffer, 64);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_send(dummy_pima, &pima_session, &object, NX_NULL, 64);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_object_delete() */
    status = ux_host_class_pima_object_delete(NX_NULL, &pima_session, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_delete(dummy_pima, NX_NULL, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_object_transfer_abort() */
    status = ux_host_class_pima_object_transfer_abort(NX_NULL, &pima_session, 0, &object);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_transfer_abort(dummy_pima, NX_NULL, 0, &object);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_transfer_abort(dummy_pima, &pima_session, 0, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_object_close() */
    status = ux_host_class_pima_object_close(NX_NULL, &pima_session, 0, &object);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_close(dummy_pima, NX_NULL, 0, &object);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_close(dummy_pima, &pima_session, 0, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_session_open() */
    status = ux_host_class_pima_session_open(NX_NULL, &pima_session);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_session_open(dummy_pima, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_session_close() */
    status = ux_host_class_pima_session_close(NX_NULL, &pima_session);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_session_close(dummy_pima, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_storage_ids_get() */
    status = ux_host_class_pima_storage_ids_get(NX_NULL, &pima_session, storage_ids_array, 32);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_storage_ids_get(dummy_pima, NX_NULL, storage_ids_array, 32);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_storage_ids_get(dummy_pima, &pima_session, NX_NULL, 32);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_storage_info_get() */
    status = ux_host_class_pima_storage_info_get(NX_NULL, &pima_session, 0, &storage);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_storage_info_get(dummy_pima, NX_NULL, 0, &storage);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_storage_info_get(dummy_pima, &pima_session, 0, NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_object_handles_get() */
    status = ux_host_class_pima_object_handles_get(NX_NULL, &pima_session, object_handles_array, 32, 0, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_handles_get(dummy_pima, NX_NULL, object_handles_array, 32, 0, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_handles_get(dummy_pima, &pima_session, NX_NULL, 32, 0, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    status = ux_host_class_pima_object_handles_get(dummy_pima, &pima_session, object_handles_array, 0, 0, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    /* Unit test for function ux_host_class_pima_num_objects_get() */
    status = ux_host_class_pima_num_objects_get(NX_NULL, &pima_session, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);
    
    status = ux_host_class_pima_num_objects_get(dummy_pima, NX_NULL, 0, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

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
