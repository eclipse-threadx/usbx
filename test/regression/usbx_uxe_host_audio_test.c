/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_host_class_audio.h"

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
void usbx_uxe_host_audio_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_host_audio APIs Test.................................... ");
#if !defined(UX_HOST_CLASS_AUDIO_ENABLE_ERROR_CHECKING)
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

static UINT _dummy_desc_parse_func(VOID  *arg,
                              UCHAR *packed_interface_descriptor,
                              UCHAR *packed_endpoint_descriptor,
                              UCHAR *packed_audio_descriptor)
{
    return(0);
}

static UINT _dummy_sampling_parse_func(VOID  *arg,
                              UCHAR *packed_interface_descriptor,
                              UX_HOST_CLASS_AUDIO_SAMPLING_CHARACTERISTICS *sam_attr)
{
    return(0);
}

static VOID _dummy_int_callback_func(UX_HOST_CLASS_AUDIO_AC *audio,
                                                 UCHAR *message, ULONG length,
                                                 VOID *arg)
{
}

static void  ux_test_thread_simulation_0_entry(ULONG arg)
{
UINT                                            status;
UX_HOST_CLASS_AUDIO                             dummy_audio_inst;
UX_HOST_CLASS_AUDIO                             *dummy_audio = &dummy_audio_inst;
UX_HOST_CLASS_AUDIO_CONTROL                     dummy_audio_control;
UX_HOST_CLASS_AUDIO_TRANSFER_REQUEST            dummy_audio_request;
UX_HOST_CLASS_AUDIO_SAMPLING_CHARACTERISTICS    dummy_audio_sampling_prop;
UX_HOST_CLASS_AUDIO_SAMPLING                    dummy_audio_sampling;
UCHAR                                           dummy_buffer[64];


    /* ux_host_class_audio_control_get()  */
    status = ux_host_class_audio_control_get(UX_NULL, &dummy_audio_control);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_control_get(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_control_value_get()  */
    status = ux_host_class_audio_control_value_get(UX_NULL, &dummy_audio_control);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_control_value_get(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_control_value_set()  */
    status = ux_host_class_audio_control_value_set(UX_NULL, &dummy_audio_control);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_control_value_set(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_read()  */
    status = ux_host_class_audio_read(UX_NULL, &dummy_audio_request);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_read(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* _uxe_host_class_audio_streaming_sampling_get()  */
    status = _uxe_host_class_audio_streaming_sampling_get(UX_NULL, &dummy_audio_sampling_prop);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = _uxe_host_class_audio_streaming_sampling_get(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* _uxe_host_class_audio_streaming_sampling_set()  */
    status = _uxe_host_class_audio_streaming_sampling_set(UX_NULL, &dummy_audio_sampling);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = _uxe_host_class_audio_streaming_sampling_set(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* _uxe_host_class_audio_write()  */
    status = _uxe_host_class_audio_write(UX_NULL, &dummy_audio_request);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = _uxe_host_class_audio_write(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_feedback_get()  */
    status = ux_host_class_audio_feedback_get(UX_NULL, dummy_buffer);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_feedback_get(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_feedback_set()  */
    status = ux_host_class_audio_feedback_set(UX_NULL, dummy_buffer);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_feedback_set(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_control_request()  */
    status = ux_host_class_audio_control_request(UX_NULL, 0, 0, 0, 0, 0, UX_NULL, 0, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_descriptors_parse()  */
    status = ux_host_class_audio_descriptors_parse(UX_NULL, _dummy_desc_parse_func, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_descriptors_parse(dummy_audio, UX_NULL, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_entity_control_get()  */
    status = ux_host_class_audio_entity_control_get(UX_NULL, &dummy_audio_control);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_entity_control_get(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_entity_control_value_get()  */
    status = ux_host_class_audio_entity_control_value_get(UX_NULL, &dummy_audio_control);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_entity_control_value_get(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_entity_control_value_set()  */
    status = ux_host_class_audio_entity_control_value_set(UX_NULL, &dummy_audio_control);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_entity_control_value_set(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_raw_sampling_parse()  */
    status = ux_host_class_audio_raw_sampling_parse(UX_NULL, _dummy_sampling_parse_func, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_raw_sampling_parse(dummy_audio, UX_NULL, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_raw_sampling_start()  */
    status = ux_host_class_audio_raw_sampling_start(UX_NULL, &dummy_audio_sampling);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_audio_raw_sampling_start(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_stop()  */
    status = ux_host_class_audio_stop(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_audio_interrupt_start()  */
    status = ux_host_class_audio_interrupt_start(UX_NULL, _dummy_int_callback_func, UX_NULL);
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
