#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_test_utility_sim.h"

static UINT class_entry_func(struct UX_HOST_CLASS_COMMAND_STRUCT* command_ptr)
{
    return 0;

}

static UX_HOST_CLASS host_class_array[2];

extern UX_SYSTEM_HOST *_ux_system_host;
static UX_SYSTEM_HOST ux_host;

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_host_stack_class_unregister_coverage_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status = 0;


    /* Inform user.  */
    printf("Running USB host Stack Class Unregister Coverage Test .............. ");
#if UX_MAX_CLASS_DRIVER > 1
    _ux_system_host = &ux_host;
    _ux_system_host -> ux_system_host_max_class = 2;
    _ux_system_host -> ux_system_host_class_array = host_class_array;
    host_class_array[0].ux_host_class_entry_function = UX_NULL;
    host_class_array[1].ux_host_class_entry_function = UX_NULL;

    status = _ux_host_stack_class_unregister(class_entry_func);

    if (status == UX_NO_CLASS_MATCH)
        printf("   Passed\n");
    else
        printf("   Failure\n");
#else
    printf("   N/A\n");
#endif
    test_control_return(0);
    return;
}
