#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_host_class_storage.h"
#include "ux_test_utility_sim.h"

static UX_DEVICE device;
static UCHAR memory_buffer[4096];
static UX_ENDPOINT endpoint;
static UX_DEVICE endpoint_device;
static UX_HCD hcd;

static UINT entry_function(struct UX_HCD_STRUCT *parm1, UINT parm2, VOID *parm3)
{

    return(UX_SUCCESS);

}

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_class_stack_device_configuration_reset_coverage_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status = 0;
UX_TRANSFER             *transfer_request; 

    /* Inform user.  */
    printf("Running USB host stack device configuration Reset Coverage Test .... ");

    ux_system_initialize(memory_buffer, 4096, UX_NULL, 0);
    
    device.ux_device_state = UX_DEVICE_SELF_POWERED_STATE;
    
    device.ux_device_packed_configuration = (UCHAR*)_ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, 16);

    transfer_request = &device.ux_device_control_endpoint.ux_endpoint_transfer_request;
    transfer_request->ux_transfer_request_endpoint = &endpoint;
    endpoint.ux_endpoint_device = &endpoint_device;
    endpoint_device.ux_device_state = UX_DEVICE_CONFIGURED;
    endpoint.ux_endpoint_descriptor.bEndpointAddress = 0x7;
#if UX_MAX_HCD > 1
    endpoint_device.ux_device_hcd = &hcd;
#else
    _ux_system_host->ux_system_host_hcd_array = &hcd;
#endif
    hcd.ux_hcd_entry_function = entry_function;

    _ux_host_stack_device_configuration_reset(&device);

    printf("SUCCESS!\n");

    test_control_return(0);
    return;
}
