/* This test is designed to test the ux_device_stack_initialize */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_device_class_cdc_acm.h"
#include "ux_host_class_cdc_acm.h"

#include "ux_host_class_dpump.h"
#include "ux_device_class_dpump.h"

#include "ux_host_class_hid.h"
#include "ux_device_class_hid.h"

#include "ux_host_class_storage.h"
#include "ux_device_class_storage.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"


/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_RUN             1
#define UX_TEST_MEMORY_SIZE     (64*1024)

/* HID mouse related descriptors */

static UCHAR hid_mouse_report[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0x09, 0x38,                    //     USAGE (Mouse Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};
#define HID_MOUSE_REPORT_LENGTH (sizeof(hid_mouse_report)/sizeof(hid_mouse_report[0]))

#define     LSB(x) (x & 0x00ff)
#define     MSB(x) ((x & 0xff00) >> 8)

/* Configuration descriptor 9 bytes */
#define CFG_DESC(wTotalLength, bNumInterfaces, bConfigurationValue)\
    /* Configuration 1 descriptor 9 bytes */\
    0x09, 0x02, LSB(wTotalLength), MSB(wTotalLength),\
    (bNumInterfaces), (bConfigurationValue), 0x00,\
    0x20, 0x00,
#define CFG_DESC_LEN 9

/* DPUMP interface descriptors 9+7+7=23 bytes. */
#define DPUMP_IFC_DESC_ALL(ifc, bulk_in_epa, bulk_out_epa) \
    /* Interface descriptor */\
    0x09, 0x04, (ifc), 0x00, 0x02, 0x99, 0x99, 0x99, 0x00,\
    /* Endpoint descriptor (Bulk Out) */\
    0x07, 0x05, (bulk_out_epa), 0x02, 0x40, 0x00, 0x00,\
    /* Endpoint descriptor (Bulk In) */\
    0x07, 0x05, (bulk_in_epa), 0x02, 0x40, 0x00, 0x00,
#define DPUMP_IFC_DESC_ALL_LEN 23

/* HID Mouse interface descriptors 9+9=18 bytes */
#define HID_MOUSE_IFC0_DESC_ALL(ifc)     \
    /* Interface descriptor */\
    0x09, 0x04, (ifc), 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,\
    /* HID descriptor */\
    0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_MOUSE_REPORT_LENGTH),\
    MSB(HID_MOUSE_REPORT_LENGTH),
#define HID_MOUSE_IFC0_DESC_ALL_LEN 18

/* HID Mouse interface descriptors 9+9+7=25 bytes */
#define HID_MOUSE_IFC1_DESC_ALL(ifc, interrupt_epa)     \
    /* Interface descriptor */\
    0x09, 0x04, (ifc), 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,\
    /* HID descriptor */\
    0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_MOUSE_REPORT_LENGTH),\
    MSB(HID_MOUSE_REPORT_LENGTH),\
    /* Endpoint descriptor (Interrupt) */\
    0x07, 0x05, (interrupt_epa), 0x03, 0x08, 0x00, 0x08,
#define HID_MOUSE_IFC1_DESC_ALL_LEN 25

static UCHAR device_framework_full_speed[] = {

    /* Device descriptor 18 bytes */
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
    0xec, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01,

    /* Configuration descriptor 9 bytes */
    CFG_DESC(CFG_DESC_LEN + DPUMP_IFC_DESC_ALL_LEN + HID_MOUSE_IFC0_DESC_ALL_LEN + HID_MOUSE_IFC1_DESC_ALL_LEN, 2, 1)
    /* DPUMP 9+9+7      =25 */
    DPUMP_IFC_DESC_ALL(0, 0x81, 0x02)
    /* HID   9+9 + 9+9+7=43 */
    HID_MOUSE_IFC0_DESC_ALL(0x11)
    HID_MOUSE_IFC1_DESC_ALL(0x11, 0x83)
};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)

static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
    0x03, 0x01,

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x01, 0x00,

    /* Configuration descriptor */
    CFG_DESC(CFG_DESC_LEN + DPUMP_IFC_DESC_ALL_LEN + HID_MOUSE_IFC0_DESC_ALL_LEN + HID_MOUSE_IFC1_DESC_ALL_LEN, 2, 1)
    /* DPUMP */
    DPUMP_IFC_DESC_ALL(0, 0x81, 0x02)
    /* HID */
    HID_MOUSE_IFC0_DESC_ALL(0x11)
    HID_MOUSE_IFC1_DESC_ALL(0x11, 0x83)
};
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)

/* String Device Framework :
    Byte 0 and 1 : Word containing the language ID : 0x0904 for US
    Byte 2       : Byte containing the index of the descriptor
    Byte 3       : Byte containing the length of the descriptor string
*/

static UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 */
    0x09, 0x04, 0x01, 0x0c,
    0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
    0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
    0x09, 0x04, 0x02, 0x0c,
    0x44, 0x61, 0x74, 0x61, 0x50, 0x75, 0x6d, 0x70,
    0x44, 0x65, 0x6d, 0x6f,

    /* Serial Number string descriptor : Index 3 */
    0x09, 0x04, 0x03, 0x04,
    0x30, 0x30, 0x30, 0x31
};
#define STRING_FRAMEWORK_LENGTH sizeof(string_framework)

    /* Multiple languages are supported on the device, to add
       a language besides English, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
static UCHAR language_id_framework[] = {

/* English. */
    0x09, 0x04
};
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(language_id_framework)

/* Simulation actions. */

static UX_TEST_SIM_ENTRY_ACTION dcd_endpoint_create_fail[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_DCD_CREATE_ENDPOINT, NULL,
        UX_FALSE, 0,
        0         , 0, UX_NULL, 0, 0,
        UX_ERROR , UX_NULL},
{   0   }
};

/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_stack_initialize_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;
UX_SLAVE_CLASS_DPUMP_PARAMETER  parameter;
UX_SLAVE_CLASS_HID_PARAMETER    hid_parameter;


    /* Inform user.  */
    printf("Running ux_device_stack_initialize Test.................. ");

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

#if !defined(UX_DEVICE_INITIALIZE_FRAMEWORK_SCAN_DISABLE) && (UX_MAX_SLAVE_INTERFACE <= 16)
    /* The code below is required for installing the device portion of USBX */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH, UX_NULL);

    /* Check for error.  */
    if (status == UX_MEMORY_INSUFFICIENT) {
        /* Successful test.  */
        printf("SUCCESS!\n");
        test_control_return(0);
    } else {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
#else
    /* Successful test.  */ 
    printf("SUCCESS!\n");
    test_control_return(0);
#endif
}
