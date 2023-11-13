#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_test_utility_sim.h"
#include "ux_host_class_storage.h"

static UX_SYSTEM ux_system;
static UX_CONFIGURATION storage_configuration;
static UX_DEVICE device;

static UX_HOST_CLASS_STORAGE storage;
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_class_storage_configure_overage_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running USB host Class Storage Configure Coverage Test ............. ");

    _ux_system = &ux_system;
    storage.ux_host_class_storage_device = &device;
    device.ux_device_state = UX_DEVICE_RESET;
    device.ux_device_handle = (ULONG)&device;
    device.ux_device_first_configuration = &storage_configuration;
    device.ux_device_power_source = UX_DEVICE_SELF_POWERED;
    storage_configuration.ux_configuration_handle = 0;
    
    _ux_host_class_storage_configure(&storage);

    printf("   Passed\n");

    test_control_return(0);
    return;
}
