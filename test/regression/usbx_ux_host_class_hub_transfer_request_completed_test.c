/* This tests the ux_host_class_hub_transfer_request_completed.c API. */

#include "usbx_ux_test_hub.h"

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_host_class_hub_transfer_request_completed_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_host_class_hub_status_get Test........................... ");

    stepinfo("\n");

    initialize_hub(first_unused_memory);
}

static void post_init_host()
{

UX_TRANSFER transfer;
UX_TRANSFER *actual_interrupt_transfer;
UX_HOST_CLASS_HUB hub;

    transfer.ux_transfer_request_class_instance = (VOID *)&hub;

    /** Test class in shutdown. **/
    hub.ux_host_class_hub_state = UX_HOST_CLASS_INSTANCE_SHUTDOWN;
    _ux_host_class_hub_transfer_request_completed(&transfer);

    /** Test aborted transfer. **/
    hub.ux_host_class_hub_state = UX_HOST_CLASS_INSTANCE_LIVE;
    transfer.ux_transfer_request_completion_code = UX_TRANSFER_STATUS_ABORT;
    _ux_host_class_hub_transfer_request_completed(&transfer);

    /** Test no answer from transfer. **/
    hub.ux_host_class_hub_state = UX_HOST_CLASS_INSTANCE_LIVE;
    transfer.ux_transfer_request_completion_code = UX_TRANSFER_NO_ANSWER;
    _ux_host_class_hub_transfer_request_completed(&transfer);

    /** Test completion code that triggers interrupt transfer reactivation. **/

    actual_interrupt_transfer = &g_hub_host->ux_host_class_hub_interrupt_endpoint->ux_endpoint_transfer_request;
    actual_interrupt_transfer->ux_transfer_request_completion_code = UX_TRANSFER_STALLED;

    /* First, abort current transfer. */
    ux_host_stack_transfer_request_abort(actual_interrupt_transfer);

    /* Now call it. */
    _ux_host_class_hub_transfer_request_completed(actual_interrupt_transfer);

#if UX_MAX_DEVICES > 1
    /* Make sure the interrupt EP still works. */
    connect_device_to_hub();

    class_dpump_get();
#endif
}

static void post_init_device()
{
}