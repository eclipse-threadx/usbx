#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_host_class_hub.h"
#include "ux_test_utility_sim.h"


static UX_HOST_CLASS_HUB hub;
static UX_DEVICE hub_device;
static UX_DEVICE device;
static UX_INTERFACE hub_interface;
static UX_TRANSFER *transfer_request;
static UX_ENDPOINT transfer_request_endpoint;
static UX_SYSTEM_HOST ux_system_host;
static UX_HCD hcd;
static int count = 0;
static UINT entry_function(struct UX_HCD_STRUCT *parm1, UINT parm2, VOID *parm3)
{
    UX_TRANSFER *t_request;
    UCHAR *data;

    
    t_request = (UX_TRANSFER*)parm3;
    data = t_request -> ux_transfer_request_data_pointer;
    if(count == 0)
        data[2] = 1;
    else if(count == 1)
        data[2] = 9;

    count++;

    return(UX_SUCCESS);
}

#define UX_MEMORY_BUFFER_SIZE 4096

static UCHAR memory_buffer[UX_MEMORY_BUFFER_SIZE];
static UCHAR cache_safe_memory_buffer[UX_MEMORY_BUFFER_SIZE];

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_class_hub_descriptor_get_coverage_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status = 0;
UX_ENDPOINT             *control_endpoint;


    /* Inform user.  */
    printf("Running USB host Class Hub Descriptor Get Coverage Test ............ ");

    ux_system_initialize(memory_buffer, UX_MEMORY_BUFFER_SIZE, cache_safe_memory_buffer, UX_MEMORY_BUFFER_SIZE);

    _ux_system_host = &ux_system_host;
    ux_system_host.ux_system_host_hcd_array = &hcd;


    hub.ux_host_class_hub_device = &hub_device;
    control_endpoint = &hub_device.ux_device_control_endpoint;
    control_endpoint -> ux_endpoint_transfer_request.ux_transfer_request_actual_length = UX_HUB_DESCRIPTOR_LENGTH;
    hub.ux_host_class_hub_device->ux_device_descriptor.bDeviceProtocol = UX_HOST_CLASS_HUB_PROTOCOL_MULTIPLE_TT;
    hub.ux_host_class_hub_interface = &hub_interface;
    hub_interface.ux_interface_descriptor.bInterfaceProtocol = UX_HOST_CLASS_HUB_PROTOCOL_MULTIPLE_TT;
    transfer_request = &control_endpoint->ux_endpoint_transfer_request;
    transfer_request -> ux_transfer_request_endpoint = &transfer_request_endpoint;
    transfer_request_endpoint.ux_endpoint_device = &device;
    device.ux_device_state = UX_DEVICE_ATTACHED;
#if UX_MAX_HCD > 1
    device.ux_device_hcd = &hcd;
#endif
    transfer_request_endpoint.ux_endpoint_descriptor.bEndpointAddress = 0x70;
    /* Test to cover line 177 */
    hcd.ux_hcd_entry_function = entry_function;

    _ux_host_class_hub_descriptor_get(&hub);

    /* Test again to cover line 181 */
    _ux_host_class_hub_descriptor_get(&hub);
    

    printf("   Passed\n");
    test_control_return(0);
    return;
}
