#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_host_class_storage.h"
#include "ux_test_utility_sim.h"


static UX_HOST_CLASS_COMMAND  command;


#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_host_class_storage_entry_overage_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status = 0;


    /* Inform user.  */
    printf("Running USB host Class Storage Entry Coverage Test ................. ");

    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_ACTIVATE_WAIT;
    status = _ux_host_class_storage_entry(&command);

    if (status == UX_STATE_NEXT)
        printf("   Passed\n");
    else
        printf("   Failure\n");

    test_control_return(0);
    return;
}
