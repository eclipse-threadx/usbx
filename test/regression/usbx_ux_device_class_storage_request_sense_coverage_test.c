#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_device_class_storage.h"
#include "ux_test_utility_sim.h"


static UX_SYSTEM_SLAVE system_slave;
static UX_SLAVE_CLASS_STORAGE slave_storage;
static UCHAR cbwcb_data[64];
static UX_SLAVE_ENDPOINT endpoint_in;
static UCHAR data_pointer[256];


#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_device_class_storage_request_sense_coverage_test_application_define(void *first_unused_memory)
#endif
{


    /* Inform user.  */
    printf("Running USB Device Class Storage Request Sense Coverage Test ....... ");


    _ux_system_slave = &system_slave;
    cbwcb_data[UX_SLAVE_CLASS_STORAGE_INQUIRY_PAGE_CODE] = UX_SLAVE_CLASS_STORAGE_INQUIRY_PAGE_CODE_SERIAL;
    
    slave_storage.ux_slave_class_storage_host_length = 0;

    endpoint_in.ux_slave_endpoint_transfer_request.ux_slave_transfer_request_data_pointer = data_pointer;
    _ux_device_class_storage_request_sense(&slave_storage, 0, &endpoint_in, NX_NULL, cbwcb_data);


    printf("SUCCESS!\n");

    test_control_return(0);
    return;
}
