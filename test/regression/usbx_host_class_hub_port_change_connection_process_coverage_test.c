#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_host_class_hub.h"
#include "ux_test_utility_sim.h"


static UINT count = 0;
static UINT hcd_entry_function(UX_HCD *hcd_ptr, UINT request, VOID *_transfer_request)
{
USHORT *port_status;
USHORT *port_change;
UX_TRANSFER *transfer_request = (UX_TRANSFER*)_transfer_request;
    if (request == UX_HCD_CREATE_ENDPOINT || request == UX_HCD_DESTROY_ENDPOINT)
        return(UX_SUCCESS);

    if(transfer_request -> ux_transfer_request_function == UX_HOST_CLASS_HUB_GET_STATUS)
    {
        port_status = (USHORT*)(transfer_request -> ux_transfer_request_data_pointer);
        port_change = (USHORT*)(transfer_request -> ux_transfer_request_data_pointer + 2);

        *port_change = UX_HOST_CLASS_HUB_PORT_CHANGE_RESET;
        if (count != 1)
            *port_status = UX_HOST_CLASS_HUB_PORT_STATUS_CONNECTION;
        else
            *port_status = 0;
        count++;
        transfer_request -> ux_transfer_request_actual_length = 4;
    }

    return UX_SUCCESS;
}

static UINT change_function(ULONG status, UX_HOST_CLASS *host_class_ptr, VOID* ptr)
{
    return (UX_SUCCESS);
}

static UX_HOST_CLASS_HUB  hub;
static UX_DEVICE hub_device;
static UX_DEVICE device1, device2;
extern UX_SYSTEM_HOST *_ux_system_host;
static UX_SYSTEM_HOST ux_system_host;
static UX_HCD         ux_hcd;
static UX_ENDPOINT    transfer_request_endpoint;
#define USB_MEMORY_POOL_SIZE            (16*1024)
#define USB_CACHE_SAFE_MEMORY_POOL_SIZE (16*1024)
static ULONG usb_memory_pool_area[USB_MEMORY_POOL_SIZE / sizeof(ULONG)];
static ULONG usb_cache_safe_memory_pool_area[USB_CACHE_SAFE_MEMORY_POOL_SIZE / sizeof(ULONG)];
static UX_DEVICE device_array[2];
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_host_class_hub_port_change_connection_process_coverage_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status = 0;
UINT                    port = 0;
UINT                    port_status;
UX_ENDPOINT     *control_endpoint;
UX_TRANSFER     *transfer_request;


    /* Inform user.  */
    printf("Running USB host Class Hub Port Change Coverage Test ............... ");
    
    ux_system_initialize(usb_memory_pool_area, USB_MEMORY_POOL_SIZE, 
        usb_cache_safe_memory_pool_area, USB_CACHE_SAFE_MEMORY_POOL_SIZE);


    _ux_system_host = &ux_system_host;
    ux_system_host.ux_system_host_hcd_array = &ux_hcd;
    ux_system_host.ux_system_host_device_array = device_array;
    device1.ux_device_handle = UX_UNUSED;
    device2.ux_device_handle = UX_UNUSED;
    ux_hcd.ux_hcd_entry_function = hcd_entry_function;
    hub.ux_host_class_hub_device = &hub_device;
#if UX_MAX_HCD > 1
    hub_device.ux_device_hcd = &ux_hcd;
#endif
    port_status = UX_HOST_CLASS_HUB_PORT_STATUS_CONNECTION;
    hub.ux_host_class_hub_port_state = 0;
    control_endpoint = &hub_device.ux_device_control_endpoint;
    transfer_request = &control_endpoint -> ux_endpoint_transfer_request;
    transfer_request -> ux_transfer_request_endpoint = &transfer_request_endpoint;
    transfer_request_endpoint.ux_endpoint_device = &hub_device;
    transfer_request_endpoint.ux_endpoint_descriptor.bEndpointAddress = UX_ENDPOINT_OUT | 1;
    hub_device.ux_device_state = UX_DEVICE_ATTACHED;

    /* Test line 150 */
    _ux_host_class_hub_port_change_connection_process(&hub, 1, 1);

    /* Test line 167 and 228*/
    hub_device.ux_device_power_source = UX_DEVICE_BUS_POWERED;
#if UX_MAX_HCD > 1
    ux_system_host.ux_system_host_max_devices = 2;
#endif
    ux_system_host.ux_system_host_change_function = UX_NULL;
    _ux_host_class_hub_port_change_connection_process(&hub, 1, 1);

    /* Test line 228 */
    ux_system_host.ux_system_host_change_function = change_function;
    _ux_host_class_hub_port_change_connection_process(&hub, 1, 1);



    printf("   Passed\n");


    test_control_return(0);
    return;
}
