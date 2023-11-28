/* This test is designed to test the ux_device_stack_class_register.  */

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

/* CDC-ACM interfaces descriptors 8 +9+5+4+5+5+7 +9+7+7=66 bytes */
#define CDC_ACM_IFCES_DESC_ALL(ifc, interrupt_epa, bulk_in_epa, bulk_out_epa) \
    /* Interface association descriptor. 8 bytes.  */\
    0x08, 0x0b, (ifc), 0x02, 0x02, 0x02, 0x00, 0x00,\
    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */\
    0x09, 0x04, (ifc), 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,\
    /* Header Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x00, 0x10, 0x01,\
    /* ACM Functional Descriptor 4 bytes */\
    0x04, 0x24, 0x02, 0x0f,\
    /* Union Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x06,\
    (ifc),                        /* Master interface */\
    (ifc+1),                      /* Slave interface  */\
    /* Call Management Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x01, 0x03,\
    (ifc+1),                      /* Data interface   */\
    /* Interrupt endpoint descriptor 7 bytes */\
    0x07, 0x05, (interrupt_epa), 0x03, 0x08, 0x00, 15,\
    /* Data Class Interface Descriptor Requirement 9 bytes */\
    0x09, 0x04, (ifc+1), 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,\
    /* Endpoint descriptor 7 bytes */\
    0x07, 0x05, (bulk_out_epa), 0x02, 0x40, 0x00, 0x00,\
    /* Endpoint descriptor 7 bytes */\
    0x07, 0x05, (bulk_in_epa), 0x02, 0x40, 0x00, 0x00,
#define CDC_ACM_IFCES_DESC_ALL_LEN 66

/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           expect_errors = UX_FALSE;


/* Define USBX test global variables.  */

static UX_HOST_CLASS                   *class_driver;
static UX_HOST_CLASS_DPUMP             *dpump;
static UX_SLAVE_CLASS_DPUMP            *dpump_slave = UX_NULL;


static UCHAR device_framework_full_speed[] = {

    /* Device descriptor 18 bytes */
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
    0xec, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01,

    /* Configuration descriptor 9 bytes */
    CFG_DESC(CFG_DESC_LEN + DPUMP_IFC_DESC_ALL_LEN + HID_MOUSE_IFC0_DESC_ALL_LEN + HID_MOUSE_IFC1_DESC_ALL_LEN + CDC_ACM_IFCES_DESC_ALL_LEN, 2, 1)
    /* DPUMP 9+9+7      =25 */
    DPUMP_IFC_DESC_ALL(0, 0x81, 0x02)
    /* HID   9+9 + 9+9+7=43 */
    HID_MOUSE_IFC0_DESC_ALL(1)
    HID_MOUSE_IFC1_DESC_ALL(1, 0x83)
    /* CDC-ACM */
    CDC_ACM_IFCES_DESC_ALL(2, 0x86, 0x85, 0x04)
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

    /* Configuration descriptor 9 bytes */
    CFG_DESC(CFG_DESC_LEN + DPUMP_IFC_DESC_ALL_LEN + HID_MOUSE_IFC0_DESC_ALL_LEN + HID_MOUSE_IFC1_DESC_ALL_LEN + CDC_ACM_IFCES_DESC_ALL_LEN, 2, 1)
    /* DPUMP 9+9+7      =25 */
    DPUMP_IFC_DESC_ALL(0, 0x81, 0x02)
    /* HID   9+9 + 9+9+7=43 */
    HID_MOUSE_IFC0_DESC_ALL(1)
    HID_MOUSE_IFC1_DESC_ALL(1, 0x83)
    /* CDC-ACM */
    CDC_ACM_IFCES_DESC_ALL(2, 0x86, 0x85, 0x04)
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


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static VOID                ux_test_instance_activate(VOID  *dpump_instance);
static VOID                ux_test_instance_deactivate(VOID *dpump_instance);

UINT                       _ux_host_class_dpump_entry(UX_HOST_CLASS_COMMAND *command);
UINT                       ux_hcd_sim_initialize(UX_HCD *hcd);
UINT                       _ux_host_class_dpump_write(UX_HOST_CLASS_DPUMP *dpump, UCHAR * data_pointer,
                                    ULONG requested_length, ULONG *actual_length);
UINT                       _ux_host_class_dpump_read (UX_HOST_CLASS_DPUMP *dpump, UCHAR *data_pointer,
                                    ULONG requested_length, ULONG *actual_length);

static TX_THREAD           ux_test_thread_simulation_0;
static TX_THREAD           ux_test_thread_simulation_1;
static void                ux_test_thread_simulation_0_entry(ULONG);
static void                ux_test_thread_simulation_1_entry(ULONG);


/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}


static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    if (!expect_errors)
    {
        if (error_code != UX_DEVICE_HANDLE_UNKNOWN)
        {
            /* Failed test.  */
            printf("Error on line %d, system_level: %d, system_context: %d, error code: %d\n", __LINE__, system_level, system_context, error_code);
            test_control_return(1);
        }
    }
}

static UINT test_ux_device_class_hid_entry(UX_SLAVE_CLASS_COMMAND *command)
{
    return UX_SUCCESS;
    // return _ux_device_class_hid_entry(command);
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_stack_class_register_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;
UX_SLAVE_CLASS_DPUMP_PARAMETER  parameter;
UX_SLAVE_CLASS_HID_PARAMETER    hid_parameter;

    /* Inform user.  */
    printf("Running ux_device_stack_class_register Test......................... ");
    stepinfo("\n");

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

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX.  */
    status =  ux_host_stack_initialize(UX_NULL);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the host class drivers for this USBX implementation.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_dpump_name, ux_host_class_dpump_entry);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* The code below is required for installing the device portion of USBX */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH, UX_NULL);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Set the parameters for callback when insertion/extraction of a Data Pump device.  */
    parameter.ux_slave_class_dpump_instance_activate   =  ux_test_instance_activate;
    parameter.ux_slave_class_dpump_instance_deactivate =  ux_test_instance_deactivate;

    /* Initialize the device dpump class. The class is connected with interface 0 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_dpump_name, _ux_device_class_dpump_entry,
                                              1, 0, &parameter);

    /* Initialize the hid class parameters.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_mouse_report;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_MOUSE_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = UX_NULL;
    hid_parameter.ux_slave_class_hid_instance_activate         = UX_NULL;

    status |= ux_device_stack_class_register(_ux_system_slave_class_hid_name, test_ux_device_class_hid_entry,
                                              1, 1, &parameter);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
#endif
    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_simulation_0, "test host simulation", ux_test_thread_simulation_0_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}


static void  ux_test_thread_simulation_0_entry(ULONG arg)
{

UINT                            status;
UX_SLAVE_CLASS_DPUMP_PARAMETER  parameter;
UCHAR                           invalid_class_name[UX_MAX_CLASS_NAME_LENGTH+2];

    /* Inform user.  */
    stepinfo(">>>>>>>>> ENUM check\n");

    ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);

    /* Enumeration check. */
    if (dpump_slave == UX_NULL)
    {

        printf("ERROR #%d: Enum fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>> Disconnect\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();

    /* Expect errors so skip them in error handler. */
    expect_errors = UX_TRUE;

    stepinfo(">>>>>>>>> ux_device_stack_class_register - error if class_name exceed UX_MAX_CLASS_NAME_LENGTH\n");
    _ux_utility_memory_set(invalid_class_name, 'a', sizeof(invalid_class_name));
    _ux_utility_memory_copy(invalid_class_name, _ux_system_slave_class_dpump_name, 20);
    status =  ux_device_stack_class_register(invalid_class_name, _ux_device_class_dpump_entry,
                                              2, 0, &parameter);
    if (status != UX_ERROR)
    {
        printf("ERROR %d: expect error %d\n", __LINE__, UX_ERROR);
        error_counter ++;
    }

    /* Break on errors. */
    expect_errors = UX_TRUE;

    /* Sleep for a tick to make sure everything is complete.  */
    tx_thread_sleep(1);

    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* Test error.  */
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    else
    {

        /* Successful test.  */
        printf("SUCCESS!\n");
        test_control_return(0);
    }
}

static VOID  ux_test_instance_activate(VOID *dpump_instance)
{

    /* Save the DPUMP instance.  */
    dpump_slave = (UX_SLAVE_CLASS_DPUMP *) dpump_instance;
}

static VOID  ux_test_instance_deactivate(VOID *dpump_instance)
{

    /* Reset the DPUMP instance.  */
    dpump_slave = UX_NULL;
}

