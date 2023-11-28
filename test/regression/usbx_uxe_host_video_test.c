/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_host_class_video.h"

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
void usbx_uxe_host_video_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_host_video APIs Test.................................... ");
#if !defined(UX_HOST_CLASS_VIDEO_ENABLE_ERROR_CHECKING)
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
UX_HOST_CLASS_VIDEO                             dummy_video_inst;
UX_HOST_CLASS_VIDEO                             *dummy_video = &dummy_video_inst;
UCHAR                                           dummy_buffer[64];
UX_HOST_CLASS_VIDEO_CONTROL                     dummy_video_control;
UX_HOST_CLASS_VIDEO_TRANSFER_REQUEST            dummy_video_request;
ULONG                                           max_payload = 0xff;

    /* ux_host_class_video_control_get()  */
    status = ux_host_class_video_control_get(UX_NULL, &dummy_video_control);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_video_control_get(dummy_video, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_video_control_value_get()  */
    status = ux_host_class_video_control_value_get(UX_NULL, &dummy_video_control);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_video_control_value_get(dummy_video, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_video_control_value_set()  */
    status = ux_host_class_video_control_value_set(UX_NULL, &dummy_video_control);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_video_control_value_set(dummy_video, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_video_read()  */
    status = ux_host_class_video_read(UX_NULL, &dummy_video_request);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_video_read(dummy_video, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_video_ioctl()  */
    status = ux_host_class_video_ioctl(UX_NULL, 0, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_video_start()  */
    status = ux_host_class_video_start(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_video_stop()  */
    status = ux_host_class_video_stop(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_video_frame_parameters_set()  */
    status = ux_host_class_video_frame_parameters_set(UX_NULL, 0, 255, 255, 100);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_video_max_payload_get()  */
    max_payload = ux_host_class_video_max_payload_get(UX_NULL);
    UX_TEST_CHECK_CODE(0, max_payload);

    /* ux_host_class_video_transfer_buffer_add()  */
    status = ux_host_class_video_transfer_buffer_add(UX_NULL, dummy_buffer);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_video_transfer_buffer_add(dummy_video, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_video_transfer_buffers_add()  */
    status = ux_host_class_video_transfer_buffers_add(UX_NULL, &dummy_buffer, 1);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_video_transfer_buffers_add(dummy_video, UX_NULL, 1);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_host_class_video_transfer_buffers_add(dummy_video, &dummy_buffer, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_video_transfer_callback_set()  */
    ux_host_class_video_transfer_callback_set(UX_NULL, UX_NULL);

    /* ux_host_class_video_entities_parse()  */
    status = ux_host_class_video_entities_parse(UX_NULL, 0, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_host_class_video_control_request()  */
    status = ux_host_class_video_control_request(UX_NULL, 1, 1, 1, 1, UX_NULL, 2);
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
