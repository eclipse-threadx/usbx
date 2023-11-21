#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_test_utility_sim.h"
#include "ux_host_class_storage.h"
#include "ux_host_stack.h"


static UX_HOST_CLASS_STORAGE storage;
extern UINT _ux_host_class_storage_max_lun_get(UX_HOST_CLASS_STORAGE*);
static UX_DEVICE ux_device;
static UX_SYSTEM test_ux_system;
static UX_DEVICE endpoint_device;
static UX_ENDPOINT ux_endpoint;
#ifdef  UX_HOST_CLASS_STORAGE_INCLUDE_LEGACY_PROTOCOL_SUPPORT
static UX_INTERFACE storage_interface;
#endif
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_host_class_storage_max_lun_get_coverage_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status = 0;

    printf("Running USB host Class Storage Max Lun Get Coverage T............... ");
#ifndef UX_HOST_STANDALONE
    _ux_system = &test_ux_system;
    _ux_system->ux_system_thread_lowest_priority = 10;
    storage.ux_host_class_storage_device = &ux_device;

#ifdef  UX_HOST_CLASS_STORAGE_INCLUDE_LEGACY_PROTOCOL_SUPPORT
    storage.ux_host_class_storage_interface = &storage_interface;
    storage_interface.ux_interface_descriptor.bInterfaceProtocol = UX_HOST_CLASS_STORAGE_PROTOCOL_BO;
#endif
    ux_device.ux_device_control_endpoint.ux_endpoint_device = &endpoint_device;

    status = _ux_host_class_storage_max_lun_get(&storage);

    if (status == TX_SEMAPHORE_ERROR)
        printf("   Passed\n");
    else
        printf("   Failure\n");
#else

    printf("    N/A\n");
#endif
    test_control_return(0);
    return;
}
