/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_device_class_cdc_ecm_deactivate_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_device_class_cdc_ecm_deactivate Test..................... ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{


    stepinfo(">>>>>>>>>>>>>>>>>>> Test modify ux_slave_class_cdc_ecm_instance_deactivate\n");
    UX_TEST_ASSERT(cdc_ecm_device != UX_NULL);
    cdc_ecm_device -> ux_slave_class_cdc_ecm_parameter.ux_slave_class_cdc_ecm_instance_deactivate = UX_NULL;

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_disconnect_slave();
    ux_test_disconnect_host_wait_for_enum_completion();

    UX_TEST_ASSERT(cdc_ecm_device != UX_NULL);

    stepinfo(">>>>>>>>>>>>>>>>>>> Test connect to avoid post post operation\n");
    /* Connect. */
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    class_cdc_ecm_get_host();

    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_host done\n");
}

static void post_init_device()
{
    stepinfo(">>>>>>>>>>>>>>>>>>> post_init_device empty\n");
}