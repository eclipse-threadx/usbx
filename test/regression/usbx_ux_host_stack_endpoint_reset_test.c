/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "fx_api.h"

#include "ux_device_class_cdc_acm.h"
#include "ux_device_stack.h"

#include "ux_host_class_cdc_acm.h"
#include "ux_host_class_hid.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

#include "ux_host_stack.h"

/* Define constants.  */
#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024
#define                             UX_DEMO_BUFFER_SIZE (UX_SLAVE_REQUEST_DATA_MAX_LENGTH + 1)
#define                             UX_DEMO_XMIT_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE 512
#define                             UX_DEMO_FILE_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE 64
#define                             UX_DEMO_MEMORY_SIZE     (96*1024)
#define                             UX_DEMO_FILE_SIZE       (128 * 1024)
#define                             UX_RAM_DISK_MEMORY      (256 * 1024)

#define     LSB(x) ( (x) & 0x00ff)
#define     MSB(x) (((x) & 0xff00) >> 8)

/* Configuration descriptor 9 bytes */
#define CFG_DESC(wTotalLength, bNumInterfaces, bmAttributes, bConfigurationValue)\
    /* Configuration 1 descriptor 9 bytes */\
    0x09, 0x02, LSB(wTotalLength), MSB(wTotalLength),\
    (bNumInterfaces), (bConfigurationValue), 0x00,\
    (bmAttributes), 0x00,
#define CFG_DESC_LEN 9

#define IAD_DESC(bIfc) \
    /* Interface association descriptor. 8 bytes.  */\
    0x08, 0x0b, (bIfc), 0x02, 0x02, 0x02, 0x00, 0x00,
#define IAD_DESC_LEN 8

#define CDC_IFC_DESC_ALL(bIfc, bIntIn, bBulkIn, bBulkOut)\
    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */\
    0x09, 0x04, (bIfc), 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,\
    /* Header Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x00, 0x10, 0x01,\
    /* ACM Functional Descriptor 4 bytes */\
    0x04, 0x24, 0x02, 0x0f,\
    /* Union Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x06, (bIfc), (bIfc + 1),\
    /* Call Management Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x01, 0x03, (bIfc + 1),\
    /* Endpoint interrupt in descriptor 7 bytes */\
    0x07, 0x05, (bIntIn), 0x03, 0x40, 0x00, 0x10,\
    /* Data Class Interface Descriptor Requirement 9 bytes */\
    0x09, 0x04, (bIfc + 1), 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,\
    /* Endpoint bulk in descriptor 7 bytes */\
    0x07, 0x05, (bBulkIn), 0x02, 0x40, 0x00, 0x01,\
    /* Endpoint bulk out descriptor 7 bytes */\
    0x07, 0x05, (bBulkOut), 0x02, 0x40, 0x00, 0x01,
#define CDC_IFC_DESC_ALL1(bIfc, bIntIn, bBulkIn, bBulkOut)\
    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */\
    0x09, 0x04, (bIfc), 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,\
    /* Header Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x00, 0x10, 0x01,\
    /* ACM Functional Descriptor 4 bytes */\
    0x04, 0x24, 0x02, 0x0f,\
    /* Union Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x06, (bIfc), (bIfc + 1),\
    /* Call Management Functional Descriptor 5 bytes */\
    0x05, 0x24, 0x01, 0x03, (bIfc + 1),\
    /* Endpoint interrupt in descriptor 7 bytes */\
    0x07, 0x05, (bIntIn), 0x03, 0x40, 0x00, 0x10,\
    /* Data Class Interface Descriptor Requirement 9 bytes */\
    0x09, 0x04, (bIfc + 1), 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,\
    /* Endpoint bulk out descriptor 7 bytes */\
    0x07, 0x05, (bBulkOut), 0x02, 0x40, 0x00, 0x01,\
    /* Endpoint bulk in descriptor 7 bytes */\
    0x07, 0x05, (bBulkIn), 0x02, 0x40, 0x00, 0x01,
#define CDC_IFC_DESC_ALL_LEN (9+5+4+5+5+7+ 9+7+7)

/* Define local/extern function prototypes.  */
static VOID                                test_thread_entry(ULONG);
static TX_THREAD                           tx_test_thread_host_simulation;
static TX_THREAD                           tx_test_thread_slave_simulation;
static VOID                                tx_test_thread_host_simulation_entry(ULONG);
static VOID                                tx_test_thread_slave_simulation_entry(ULONG);

static VOID                                test_cdc_instance_activate(VOID  *cdc_instance);
static VOID                                test_cdc_instance_deactivate(VOID *cdc_instance);
static VOID                                test_cdc_instance_parameter_change(VOID *cdc_instance);

/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_control = UX_NULL;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_data = UX_NULL;

static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_control1 = UX_NULL;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_data1 = UX_NULL;

static UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave = UX_NULL;

static UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave1 = UX_NULL;

static UCHAR                               cdc_acm_slave_change;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER    parameter;

static ULONG                               error_counter;

static ULONG                               error_callback_counter;
static UCHAR                               error_callback_ignore;

static ULONG                               call_counter;

static UCHAR                               buffer[UX_DEMO_BUFFER_SIZE];

static UCHAR                               test_slave_code = 0;
static UCHAR                               test_slave_state = 0;

/* Define device framework.  */

static unsigned char device_framework_full_speed[] = {

    /* Device descriptor     18 bytes
       0x02 bDeviceClass:    CDC class code
       0x00 bDeviceSubclass: CDC class sub code
       0x00 bDeviceProtocol: CDC Device protocol

       idVendor & idProduct - http://www.linux-usb.org/usb.ids
    */
    0x12, 0x01, 0x10, 0x01,
    0xEF, 0x02, 0x01,
    0x08,
    0x84, 0x84, 0x00, 0x00,
    0x00, 0x01,
    0x01, 0x02, 03,
    0x01, /* bNumConfigurations */

    /* Configuration 1 descriptor 9 bytes, total 75 bytes */
    CFG_DESC(CFG_DESC_LEN + (IAD_DESC_LEN + CDC_IFC_DESC_ALL_LEN) * 2, 4, 0x40, 1)
    /* IAD 8 bytes */
    IAD_DESC(0)
    /* CDC_ACM interfaces */
    CDC_IFC_DESC_ALL(0, 0x83, 0x81, 0x02)
    /* IAD 8 bytes */
    IAD_DESC(2)
    /* CDC_ACM interfaces */
    CDC_IFC_DESC_ALL1(2, 0x86, 0x85, 0x04)
};
#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      sizeof(device_framework_full_speed)

static unsigned char device_framework_high_speed[] = {

    /* Device descriptor     18 bytes
       0x02 bDeviceClass:    CDC class code
       0x00 bDeviceSubclass: CDC class sub code
       0x00 bDeviceProtocol: CDC Device protocol

       idVendor & idProduct - http://www.linux-usb.org/usb.ids
    */
    0x12, 0x01, 0x00, 0x02,
    0xEF, 0x02, 0x01,
    0x40,
    0x84, 0x84, 0x00, 0x00,
    0x00, 0x01,
    0x01, 0x02, 03,
    0x01, /* bNumConfigurations */

    /* Device qualifier descriptor 10 bytes */
    0x0a, 0x06, 0x00, 0x02,
    0x02, 0x00, 0x00,
    0x40,
    0x01,
    0x00,

    /* Configuration 1 descriptor 9 bytes, total 75 bytes */
    CFG_DESC(CFG_DESC_LEN + IAD_DESC_LEN + CDC_IFC_DESC_ALL_LEN, 2, 0x60, 1)
    /* IAD 8 bytes */
    IAD_DESC(0)
    /* CDC_ACM interfaces */
    CDC_IFC_DESC_ALL1(0, 0x83, 0x81, 0x02)
    /* IAD 8 bytes */
    IAD_DESC(2)
    /* CDC_ACM interfaces */
    CDC_IFC_DESC_ALL(2, 0x86, 0x85, 0x04)
};
#define             DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED      sizeof(device_framework_high_speed)

static unsigned char string_framework[] = {

    /* Manufacturer string descriptor : Index 1 - "Express Logic" */
    0x09, 0x04, 0x01, 0x0c,
    0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
    0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 - "EL Composite device" */
    0x09, 0x04, 0x02, 0x13,
    0x45, 0x4c, 0x20, 0x43, 0x6f, 0x6d, 0x70, 0x6f,
    0x73, 0x69, 0x74, 0x65, 0x20, 0x64, 0x65, 0x76,
    0x69, 0x63, 0x65,

    /* Serial Number string descriptor : Index 3 - "0001" */
    0x09, 0x04, 0x03, 0x04,
    0x30, 0x30, 0x30, 0x31
};
#define             STRING_FRAMEWORK_LENGTH                 sizeof(string_framework)

/* Multiple languages are supported on the device, to add
    a language besides english, the unicode language code must
    be appended to the language_id_framework array and the length
    adjusted accordingly. */
static unsigned char language_id_framework[] = {

    /* English. */
    0x09, 0x04
};
#define             LANGUAGE_ID_FRAMEWORK_LENGTH            sizeof(language_id_framework)

/* Simulation actions. */

static UX_TEST_SIM_ENTRY_ACTION hcd_transfer_request_fail[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, NULL,
        UX_FALSE, 0,
        0         , 0, UX_NULL, 0, 0,
        UX_ERROR , UX_NULL},
{   0   }
};

/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}

static UINT break_on_cdc_acm_all_ready(VOID)
{

    if (cdc_acm_host_control == UX_NULL || cdc_acm_host_data == UX_NULL)
        /* Do not break. */
        return 0;

    if (cdc_acm_host_control1 == UX_NULL || cdc_acm_host_data1 == UX_NULL)
        /* Do not break. */
        return 0;

    if (cdc_acm_host_control->ux_host_class_cdc_acm_state != UX_HOST_CLASS_INSTANCE_LIVE)
        /* Do not break. */
        return 0;

    if (cdc_acm_host_data->ux_host_class_cdc_acm_state != UX_HOST_CLASS_INSTANCE_LIVE)
        /* Do not break. */
        return 0;

    if (cdc_acm_host_control1->ux_host_class_cdc_acm_state != UX_HOST_CLASS_INSTANCE_LIVE)
        /* Do not break. */
        return 0;

    if (cdc_acm_host_data1->ux_host_class_cdc_acm_state != UX_HOST_CLASS_INSTANCE_LIVE)
        /* Do not break. */
        return 0;

    if (cdc_acm_slave == UX_NULL || cdc_acm_slave1 == UX_NULL)
        /* Do not break. */
        return 0;

    /* All found, break. */
    return 1;
}

static UINT break_on_removal(VOID)
{

    if (cdc_acm_host_control != UX_NULL || cdc_acm_host_data != UX_NULL)
        /* Do not break. */
        return 0;

    if (cdc_acm_host_control1 != UX_NULL || cdc_acm_host_data1 != UX_NULL)
        /* Do not break. */
        return 0;

    if (cdc_acm_slave != UX_NULL || cdc_acm_slave1 != UX_NULL)
        /* Do not break. */
        return 0;

    return 1;
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_CDC_ACM *cdc_acm = (UX_HOST_CLASS_CDC_ACM *) inst;

    switch(event)
    {

        case UX_DEVICE_INSERTION:

            if (cdc_acm -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
            {
                if (cdc_acm_host_control == UX_NULL)
                    cdc_acm_host_control = cdc_acm;
                else
                    if (cdc_acm_host_control1 == UX_NULL)
                        cdc_acm_host_control1 = cdc_acm;
            }
            else
            {
                if (cdc_acm_host_data == UX_NULL)
                    cdc_acm_host_data = cdc_acm;
                else
                    if (cdc_acm_host_data1 == UX_NULL)
                        cdc_acm_host_data1 = cdc_acm;
            }
            break;

        case UX_DEVICE_REMOVAL:

            if (cdc_acm -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
            {
                if (cdc_acm_host_control == cdc_acm)
                    cdc_acm_host_control = UX_NULL;
                if (cdc_acm_host_control1 == cdc_acm)
                    cdc_acm_host_control1 = UX_NULL;
            }
            else
            {
                if (cdc_acm_host_data == cdc_acm)
                    cdc_acm_host_data = UX_NULL;
                if (cdc_acm_host_data1 == cdc_acm)
                    cdc_acm_host_data1 = UX_NULL;
            }
            break;

        default:
            break;
    }
    return 0;
}

static VOID    test_cdc_instance_activate(VOID *cdc_instance)
{

    /* Save the CDC instance.  */
    if (cdc_acm_slave == UX_NULL)
        cdc_acm_slave = (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;
    else
    if (cdc_acm_slave1 == UX_NULL)
        cdc_acm_slave1 =  (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;
}
static VOID    test_cdc_instance_deactivate(VOID *cdc_instance)
{

    /* Reset the CDC instance.  */
    if ((VOID *)cdc_acm_slave == cdc_instance)
        cdc_acm_slave = UX_NULL;
    if ((VOID *)cdc_acm_slave1 == cdc_instance)
        cdc_acm_slave1 = UX_NULL;
}

static VOID test_cdc_instance_parameter_change(VOID *cdc_instance)
{

    /* Set CDC parameter change flag. */
    cdc_acm_slave_change = UX_TRUE;
}

static VOID test_ux_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        /* Ignore UX_DEVICE_HANDLE_UNKNOWN.  */
        if (UX_DEVICE_HANDLE_UNKNOWN == error_code)
            return;
        {
            /* Failed test.  */
            printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
            // test_control_return(1);
        }
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_host_stack_endpoint_reset_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;

    printf("Running ux_host_stack_endpoint_reset Test........................... ");

    stepinfo("\n");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    stepinfo(">>>>>>>>>>>>>>>> ux_system_initialize\n");

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL, 0);
    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(test_ux_error_callback);

    stepinfo(">>>>>>>>>>>>>>>> ux_host_stack_initialize\n");

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(test_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> ux_device_stack_initialize\n");

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> ux_device_stack_class_register\n");

    /* Set the parameters for callback when insertion/extraction of a CDC device.  */
    parameter.ux_slave_class_cdc_acm_instance_activate   =  test_cdc_instance_activate;
    parameter.ux_slave_class_cdc_acm_instance_deactivate =  test_cdc_instance_deactivate;
    parameter.ux_slave_class_cdc_acm_parameter_change    =  test_cdc_instance_parameter_change;

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status  =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1,0,  &parameter);
    status |=  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1,2,  &parameter);
    status |=  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1,4,  &parameter);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
#else
    error_callback_ignore = UX_TRUE;
#endif
    stepinfo(">>>>>>>>>>>>>>>> _ux_test_dcd_sim_slave_initialize\n");

    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> ux_host_stack_class_register\n");

#if UX_MAX_CLASS_DRIVER > 1
    /* Register HID class */
    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
#endif

    /* Register CDC ACM class */
    status =  ux_host_stack_class_register(_ux_system_host_class_cdc_acm_name, ux_host_class_cdc_acm_entry);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> ux_host_stack_hcd_register\n");

#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    error_callback_ignore = UX_TRUE; /* One of interface no driver.  */
#endif

    /* Register HCD for test */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Create main test thread\n");

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_test_thread_host_simulation, "tx test host simulation", tx_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Create test thread\n");

    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&tx_test_thread_slave_simulation, "tx test slave simulation", tx_test_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_DONT_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}

void  tx_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;

    /* Test connect. */
    stepinfo(">>>>>>>>>>>>>>>> Test connect (FS)\n");
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    ux_test_breakable_sleep(500, break_on_cdc_acm_all_ready);
    if (!(cdc_acm_host_control
            && cdc_acm_host_data
            && cdc_acm_slave
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
            && cdc_acm_host_control1
            && cdc_acm_host_data1
            && cdc_acm_slave1
#endif
            ))
    {

        printf("ERROR #%d: connect fail\n", __LINE__);
        test_control_return(1);
    }

    error_callback_ignore = UX_TRUE;

    stepinfo(">>>>>>>>>>>>>>>> Test _ux_host_stack_endpoint_reset - request fail\n");

    ux_test_hcd_sim_host_set_actions(hcd_transfer_request_fail);
    status = _ux_host_stack_endpoint_reset(cdc_acm_host_data->ux_host_class_cdc_acm_bulk_in_endpoint);
    if (status == UX_SUCCESS)
    {
        printf("ERROR #%d: expect error\n", __LINE__);
        error_counter ++;
    }


    stepinfo(">>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();

    ux_test_breakable_sleep(100, break_on_removal);

    if (cdc_acm_host_control || cdc_acm_host_data || cdc_acm_slave)
    {

        printf("ERROR #%d: disconnect fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Deinitialize\n");

    /* Deinitialize the class.  */
    ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);
    ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);
    ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    stepinfo(">>>>>>>>>>>>>>>> Dump results\n");

    if (error_counter > 0)
    {

        /* Test error.  */
        printf("ERROR #%d: total %ld errors\n", __LINE__, error_counter);
        test_control_return(1);
    }

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}

void  tx_test_thread_slave_simulation_entry(ULONG arg)
{

    while(1)
    {
        switch (test_slave_code)
        {
        default:
            break;
        }
        test_slave_state = 0;
        tx_thread_suspend(&tx_test_thread_slave_simulation);
    }
}
