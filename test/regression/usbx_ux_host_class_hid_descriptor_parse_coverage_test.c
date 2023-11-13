#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_host_class_hid.h"
#include "ux_test_utility_sim.h"

#define                             UX_DEMO_STACK_SIZE              2048
#define                             UX_DEMO_MEMORY_SIZE             (256*1024)
#define                             UX_DEMO_BUFFER_SIZE             2048

static UX_HOST_CLASS_HID hid;
static UX_DEVICE hid_device;
static UX_CONFIGURATION config;
static UCHAR packed_configuration[100];
// static UX_SYSTEM ux_system;
UCHAR usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_class_hid_descriptor_parse_coverage_test_application_define(void *first_unused_memory)
#endif
{

UINT status = 0;
UCHAR total_configuration_length = 5;
 UINT  descriptor_type = 0;
CHAR *stack_pointer;
CHAR *memory_pointer;

    /* Inform user.  */
    printf("Running USB host Class HID Descriptor Parse Converge Coverage Test . ");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);
    if (status != UX_SUCCESS)
    {
        printf("ux_system_initialize failed! status = %d\n", status);
        test_control_return(status);
        return;
    }

    // _ux_system = &ux_system;
    hid.ux_host_class_hid_device = &hid_device;
    hid_device.ux_device_current_configuration = &config;
    config.ux_configuration_descriptor.wTotalLength = total_configuration_length;
    hid_device.ux_device_packed_configuration = packed_configuration;
    packed_configuration[0] = total_configuration_length + 1;
    packed_configuration[1] = descriptor_type;
    _ux_host_class_hid_descriptor_parse(&hid);

    packed_configuration[0] = total_configuration_length;
    packed_configuration[1] = descriptor_type;
    _ux_host_class_hid_descriptor_parse(&hid);


    printf("SUCCESS!\n");

    test_control_return(0);
    return;
}
