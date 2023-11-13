#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_device_stack.h"
#include "ux_test_utility_sim.h"

extern UX_SYSTEM_SLAVE *_ux_system_slave;
static UX_SYSTEM_SLAVE system_slave;
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_stack_clear_feature_coverage_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status = 0;


    /* Inform user.  */
    printf("Running USB Device Stack Clear Feature Coverage Test ............... ");

    _ux_system_slave = &system_slave;

    _ux_device_stack_clear_feature(UX_REQUEST_TARGET_DEVICE, 0, 0);

    printf("   Passed\n");
    test_control_return(0);
    return;
}
