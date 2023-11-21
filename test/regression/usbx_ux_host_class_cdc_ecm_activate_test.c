/* Include necessary system files.  */

#include "usbx_ux_test_cdc_ecm.h"

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_host_class_cdc_ecm_activate_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_host_class_cdc_ecm_activate Test............................ ");

    stepinfo("\n");

    ux_test_cdc_ecm_initialize(first_unused_memory);
}

static void post_init_host()
{
}

static void post_init_device()
{
}