#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_test_utility_sim.h"


extern UINT  _ux_host_stack_new_endpoint_create(UX_INTERFACE *ux_interface, UCHAR * interface_endpoint);

#define UX_TEST_MEMORY_SIZE  (32 * 1024)
static UX_INTERFACE test_interface;
static struct UX_CONFIGURATION_STRUCT  test_configuration;
static UCHAR test_interface_data[] = {
    /* ---------------------------------  Endpoint Descriptor */
    /* 0 bLength,  bDescriptorType                            */ 0x04,     0x01, 
    /* 2 bEndpintAddress, bmAttributes                        */ 0x00,     0x03, 
    /* 4 wMaxPacketSize,                                      */ 0x00,     0x04, 
    /* 6 bInterval                                            */ 0x00};
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_host_stack_new_endpoint_create_overage_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status = 0;
UX_INTERFACE            *usbx_interface = &test_interface;
 UCHAR* interface_endpoint = test_interface_data;
 VOID* memory_pointer;

    /* Inform user.  */
    printf("Running USB host Stack New Endpoint Create Coverage Test ........... ");

    memory_pointer = first_unused_memory;
    
    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);
    
    test_interface.ux_interface_configuration = &test_configuration;

    status = _ux_host_stack_new_endpoint_create(usbx_interface, interface_endpoint);

    if (status == UX_DESCRIPTOR_CORRUPTED)
        printf("   Passed\n");
    else
        printf("   Failure\n");

    test_control_return(0);
    return;
}
