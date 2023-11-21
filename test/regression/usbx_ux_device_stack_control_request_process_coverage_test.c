#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_device_stack.h"
#include "ux_test_utility_sim.h"


static UX_SLAVE_TRANSFER transfer_request;
static UX_SYSTEM_SLAVE system_slave;
static UX_SLAVE_CLASS  slave_class1;
static UX_SLAVE_INTERFACE slave_interface;

UINT _test_class_entry(struct UX_SLAVE_CLASS_COMMAND_STRUCT *cmd)
{
    return 0;
}

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_stack_control_request_process_coverage_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status = 0;
UCHAR                   request_type = UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
UCHAR                   request = 0;



    /* Inform user.  */
    printf("Running USB Device Stack Control Request Process Coverage Test ..... ");


    _ux_system_slave = &system_slave;
    transfer_request.ux_slave_transfer_request_completion_code = UX_SUCCESS;
    transfer_request.ux_slave_transfer_request_setup[0] = request_type;
    transfer_request.ux_slave_transfer_request_setup[UX_SETUP_REQUEST] = request;

    transfer_request.ux_slave_transfer_request_setup[UX_SETUP_INDEX] = 0;
    transfer_request.ux_slave_transfer_request_setup[UX_SETUP_INDEX + 1] = 1;
    system_slave.ux_system_slave_interface_class_array[0] = &slave_class1;
    slave_class1.ux_slave_class_interface = &slave_interface;
    slave_class1.ux_slave_class_entry_function = _test_class_entry;
    slave_interface.ux_slave_interface_descriptor.bInterfaceClass = 0x7;
    _ux_device_stack_control_request_process(&transfer_request);
    
    printf("SUCCESS!\n");
    test_control_return(0);

    return;
}
