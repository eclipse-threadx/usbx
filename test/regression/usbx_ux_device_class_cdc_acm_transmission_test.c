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
#define                             UX_DEMO_MEMORY_SIZE     (64*1024)
#define                             UX_DEMO_FILE_SIZE       (128 * 1024)
#define                             UX_RAM_DISK_MEMORY      (256 * 1024)

#define     LSB(x) ( (x) & 0x00ff)
#define     MSB(x) (((x) & 0xff00) >> 8)

/* Configuration descriptor 9 bytes */
#define CFG_DESC(wTotalLength, bNumInterfaces, bConfigurationValue)\
    /* Configuration 1 descriptor 9 bytes */\
    0x09, 0x02, LSB(wTotalLength), MSB(wTotalLength),\
    (bNumInterfaces), (bConfigurationValue), 0x00,\
    0x40, 0x00,
#define CFG_DESC_LEN 9

#define CDC_IFC_DESC_ALL(bIfc, bIntIn, bBulkIn, bBulkOut)\
    /* Interface association descriptor. 8 bytes.  */\
    0x08, 0x0b, (bIfc), 0x02, 0x02, 0x02, 0x00, 0x00,\
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
    0x07, 0x05, (bIntIn), 0x02, 0x40, 0x00, 0x00,\
    /* Data Class Interface Descriptor Requirement 9 bytes */\
    0x09, 0x04, (bIfc + 1), 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,\
    /* Endpoint bulk out descriptor 7 bytes */\
    0x07, 0x05, (bBulkOut), 0x02, 0x40, 0x00, 0x00,\
    /* Endpoint bulk in descriptor 7 bytes */\
    0x07, 0x05, (bBulkIn), 0x02, 0x40, 0x00, 0x00,
#define CDC_IFC_DESC_ALL_LEN (8+ 9+5+4+5+5+7+ 9+7+7)

/* Define local/extern function prototypes.  */
static VOID                                test_thread_entry(ULONG);
static TX_THREAD                           tx_test_thread_host_simulation;
static TX_THREAD                           tx_test_thread_slave_simulation;
static VOID                                tx_test_thread_host_simulation_entry(ULONG);
static VOID                                tx_test_thread_slave_simulation_entry(ULONG);
       void                                tx_test_thread_dummy_entry(ULONG arg);
static VOID                                test_cdc_instance_activate(VOID  *cdc_instance);
static VOID                                test_cdc_instance_deactivate(VOID *cdc_instance);
static VOID                                test_cdc_instance_parameter_change(VOID *cdc_instance);

/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UX_HOST_CLASS                       *class_driver;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_control;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_data;

static UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave;
static UCHAR                               cdc_acm_slave_change;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER    parameter;

static ULONG                               error_counter;

static ULONG                               error_callback_counter;
static UCHAR                               error_callback_ignore;

static TX_SEMAPHORE                        slave_read_semaphore;
static UINT                                slave_read_status;
static ULONG                               slave_read_length;
static ULONG                               slave_read_counter;
static struct {
    UCHAR *buffer;
    UCHAR *save2;
    ULONG length;
    UINT  status;
} slave_read_log[32];
static TX_SEMAPHORE                        slave_write_semaphore;
static UINT                                slave_write_status;
static ULONG                               slave_write_length;

static UCHAR                               buffer[1024];
static UCHAR                               host_buffer[1024];

static ULONG                               actions_2_set_trigger = ~0;
static UX_TEST_HCD_SIM_ACTION              *actions_2_set = UX_NULL;

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
    CFG_DESC(CFG_DESC_LEN + CDC_IFC_DESC_ALL_LEN, 2, 1)
    CDC_IFC_DESC_ALL(0, 0x83, 0x81, 0x02)
};
#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      sizeof(device_framework_full_speed)

static unsigned char device_framework_high_speed[] = {

    /* Device descriptor
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

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02,
    0x02, 0x00, 0x00,
    0x40,
    0x01,
    0x00,

    /* Configuration 1 descriptor 9 bytes, total 75 bytes */
    CFG_DESC(CFG_DESC_LEN + CDC_IFC_DESC_ALL_LEN, 2, 1)
    CDC_IFC_DESC_ALL(0, 0x83, 0x81, 0x02)
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

static UX_TEST_HCD_SIM_ACTION error_on_transfer0[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_DCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER, 0, UX_NULL, 0, UX_ERROR,
        UX_ERROR}, /* Error */
{   0   }
};

static UX_TEST_HCD_SIM_ACTION error_on_transfer1[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_DCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER, 0, UX_NULL, 0, UX_ERROR,
        UX_SUCCESS}, /* Completion code error */
{   UX_DCD_TRANSFER_REQUEST, UX_NULL,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER, 0, UX_NULL, 0, UX_ERROR,
        UX_ERROR}, /* Error */
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
UINT                                 status;
UX_HOST_CLASS                       *class;

    /* Find the main cdc_acm container */
    status = ux_host_stack_class_get(_ux_system_host_class_cdc_acm_name, &class);
    if (status != UX_SUCCESS)
        /* Do not break. */
        return 0;

    /* Find class instance. */
    status = ux_host_stack_class_instance_get(class, 0, (void **) &cdc_acm_host_control);
    if (status != UX_SUCCESS)
        /* Do not break. */
        return 0;

    if (cdc_acm_host_control->ux_host_class_cdc_acm_state != UX_HOST_CLASS_INSTANCE_LIVE)
        /* Do not break. */
        return 0;

    /* Find class instance. */
    status = ux_host_stack_class_instance_get(class, 1, (void **) &cdc_acm_host_data);
    if (status != UX_SUCCESS)
        /* Do not break. */
        return 0;

    if (cdc_acm_host_control->ux_host_class_cdc_acm_state != UX_HOST_CLASS_INSTANCE_LIVE)
        /* Do not break. */
        return 0;

    if (cdc_acm_slave == UX_NULL)
        /* Do not break. */
        return 0;

    /* All found, break. */
    return 1;
}

static UINT break_on_removal(VOID)
{

UINT                     status;
UX_DEVICE               *device;

    status = ux_host_stack_device_get(0, &device);
    if (status == UX_SUCCESS)
        /* Do not break. */
        return UX_SUCCESS;

    return 1;
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_CDC_ACM *cdc_acm = (UX_HOST_CLASS_CDC_ACM *) inst;

    switch(event)
    {

        case UX_DEVICE_INSERTION:

            if (cdc_acm -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
                cdc_acm_host_control = cdc_acm;
            else
                cdc_acm_host_data = cdc_acm;
            break;

        case UX_DEVICE_REMOVAL:

            if (cdc_acm -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
                cdc_acm_host_control = UX_NULL;
            else
                cdc_acm_host_data = UX_NULL;
            break;

        default:
            break;
    }
    return 0;
}

static VOID    test_cdc_instance_activate(VOID *cdc_instance)
{

    /* Save the CDC instance.  */
    cdc_acm_slave = (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;
}
static VOID    test_cdc_instance_deactivate(VOID *cdc_instance)
{

    /* Reset the CDC instance.  */
    cdc_acm_slave = UX_NULL;
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
void usbx_ux_device_class_cdc_acm_transmission_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;
ULONG                   rmem_free;


    printf("Running ux_device_class_cdc_acm_ transmission Test.................. ");
#ifdef UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE
    printf("Skipped\n");
    test_control_return(0);
    return;
#endif
    stepinfo("\n");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

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

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register CDC ACM class */
    status =  ux_host_stack_class_register(_ux_system_host_class_cdc_acm_name, ux_host_class_cdc_acm_entry);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

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

    /* Set the parameters for callback when insertion/extraction of a CDC device.  */
    parameter.ux_slave_class_cdc_acm_instance_activate   =  test_cdc_instance_activate;
    parameter.ux_slave_class_cdc_acm_instance_deactivate =  test_cdc_instance_deactivate;
    parameter.ux_slave_class_cdc_acm_parameter_change    =  test_cdc_instance_parameter_change;

#if 1 /* Resources management moved to initialize/uninitialize  */

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1, 0,  &parameter);
    if(status!=UX_SUCCESS)
    {
        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);
    if(status!=UX_SUCCESS)
    {
        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    error_callback_ignore = UX_TRUE;
    rmem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    stepinfo(">>>>>>>>>>>>>>>> Test device init TRANSMISSION stack fail\n");
    ux_test_utility_sim_mem_allocate_until_flagged(sizeof(UX_SLAVE_CLASS_CDC_ACM) + UX_THREAD_STACK_SIZE, UX_REGULAR_MEMORY);
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1, 0, &parameter);
    UX_TEST_ASSERT(status == UX_MEMORY_INSUFFICIENT);
    ux_test_utility_sim_mem_free_all();

    stepinfo(">>>>>>>>>>>>>>>> Test device init TRANSMISSION flag fail\n");
    ux_test_utility_sim_event_create_count_reset();
    ux_test_utility_sim_event_error_generation_start(0);
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1, 0, &parameter);
    if (status != UX_EVENT_ERROR)
    {
        printf("ERROR #%d: stack alloc fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    ux_test_utility_sim_event_error_generation_stop();

    stepinfo(">>>>>>>>>>>>>>>> Test device init TRANSMISSION bulkin thread fail\n");
    ux_test_utility_sim_thread_create_count_reset();
    ux_test_utility_sim_thread_error_generation_start(0);
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1, 0, &parameter);
    if (status != UX_THREAD_ERROR)
    {
        printf("ERROR #%d: thread create fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device init TRANSMISSION bulkout thread fail\n");
    ux_test_utility_sim_thread_create_count_reset();
    ux_test_utility_sim_thread_error_generation_start(1);
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1, 0, &parameter);
    if (status != UX_THREAD_ERROR)
    {
        printf("ERROR #%d: thread create fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    ux_test_utility_sim_thread_error_generation_stop();

    /* Check memory leak.  */
    if (rmem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
    {
        printf("ERROR #%d: memory level changed %ld <> %ld\n", __LINE__, rmem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
        test_control_return(1);
    }
    error_callback_ignore = UX_TRUE;
#endif

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1, 0,  &parameter);

    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d, code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register HCD for test */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

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

    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&tx_test_thread_slave_simulation, "tx test slave simulation", tx_test_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

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
UX_HOST_CLASS_CDC_ACM_LINE_STATE                   line_state;
ULONG                                              actual_length;


    stepinfo(">>>>>>>>>>>>>>>> Test host wait\n");
    _ux_utility_thread_suspend(&tx_test_thread_host_simulation);

    /* Test connect. */
    stepinfo(">>>>>>>>>>>>>>>> Test host connect\n");
    ux_test_breakable_sleep(100, break_on_cdc_acm_all_ready);
    if (!(cdc_acm_host_control && cdc_acm_host_data && cdc_acm_slave))
    {

        printf("ERROR #%d: connect fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test host set DTR\n");
    line_state.ux_host_class_cdc_acm_line_state_dtr = 1;
    line_state.ux_host_class_cdc_acm_line_state_rts = 1;
    status = ux_host_class_cdc_acm_ioctl(cdc_acm_host_control, UX_HOST_CLASS_CDC_ACM_IOCTL_SET_LINE_STATE, &line_state);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_utility_thread_suspend(&tx_test_thread_host_simulation);

    stepinfo(">>>>>>>>>>>>>>>> Test host write 1, to move slave bulk OUT thread\n");
    status = ux_host_class_cdc_acm_write(cdc_acm_host_data, host_buffer, 1, &actual_length);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_utility_thread_suspend(&tx_test_thread_host_simulation);

    stepinfo(">>>>>>>>>>>>>>>> Test host read 64, expect 1\n");
    status = ux_host_class_cdc_acm_read(cdc_acm_host_data, host_buffer, 64, &actual_length);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (actual_length != 1)
    {
        printf("ERROR #%d: length %ld\n", __LINE__, actual_length);
        test_control_return(1);
    }
    if (_ux_utility_memory_compare(buffer, host_buffer, 1) != UX_SUCCESS)
    {
        printf("ERROR #%d: data error\n", __LINE__);
        test_control_return(1);
    }
    _ux_utility_thread_suspend(&tx_test_thread_host_simulation);

    stepinfo(">>>>>>>>>>>>>>>> Test host read 512, expect 128 (ZLP)\n");
    status = ux_host_class_cdc_acm_read(cdc_acm_host_data, host_buffer, 512, &actual_length);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (actual_length != 128)
    {
        printf("ERROR #%d: length %ld\n", __LINE__, actual_length);
        test_control_return(1);
    }
    if (_ux_utility_memory_compare(buffer, host_buffer, 128) != UX_SUCCESS)
    {
        printf("ERROR #%d: data error\n", __LINE__);
        test_control_return(1);
    }
    _ux_utility_thread_suspend(&tx_test_thread_host_simulation);

    stepinfo(">>>>>>>>>>>>>>>> Test host write 64+1\n");
    _ux_utility_memory_set(host_buffer, 64+1, 64+1);
    _ux_utility_memory_set(buffer, 0, 64+1);
    status = ux_host_class_cdc_acm_write(cdc_acm_host_data, host_buffer, 64+1, &actual_length);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (actual_length != 64+1)
    {
        printf("ERROR #%d: length %ld\n", __LINE__, actual_length);
        test_control_return(1);
    }
    _ux_utility_thread_suspend(&tx_test_thread_host_simulation);

    stepinfo(">>>>>>>>>>>>>>>> Test host write 1\n");
    ux_host_class_cdc_acm_write(cdc_acm_host_data, host_buffer, 1, &actual_length);
    _ux_utility_thread_suspend(&tx_test_thread_host_simulation);

    stepinfo(">>>>>>>>>>>>>>>> Test host write 1\n");
    ux_host_class_cdc_acm_write(cdc_acm_host_data, host_buffer, 1, &actual_length);
    _ux_utility_thread_suspend(&tx_test_thread_host_simulation);

    stepinfo(">>>>>>>>>>>>>>>> Deinitialize\n");

    /* Deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);

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

UINT tx_demo_thread_slave_read_callback(UX_SLAVE_CLASS_CDC_ACM *cdc_acm_slave, UINT status, UCHAR *data_pointer, ULONG length)
{
    if (slave_read_counter < 32)
    {
        slave_read_log[slave_read_counter].buffer = data_pointer;
        slave_read_log[slave_read_counter].length = length;
        slave_read_log[slave_read_counter].status = status;
        if (slave_read_log[slave_read_counter].save2)
            ux_utility_memory_copy(slave_read_log[slave_read_counter].save2, data_pointer,length);
    }
    slave_read_counter ++;

    if (actions_2_set_trigger != (~0))
    {
        if (slave_read_counter == actions_2_set_trigger)
        {
            ux_test_dcd_sim_slave_set_actions(actions_2_set);
            slave_read_counter = 0;
            actions_2_set = UX_NULL;
            actions_2_set_trigger = ~0;
        }

    }

    /* Save the status.  */
    slave_read_status = status;
    slave_read_length = length;

    /* Copy the data in local buffer.  */
    ux_utility_memory_copy(buffer, data_pointer,length);

    /* Put a semaphore. This will wake up the host.  */
    _ux_utility_semaphore_put(&slave_read_semaphore);

    /* Done here.  */
    return(UX_SUCCESS);
}


UINT tx_demo_thread_slave_write_callback(UX_SLAVE_CLASS_CDC_ACM *cdc_acm_slave, UINT status, ULONG length)
{

    /* Save the status.  */
    slave_write_status = status;
    slave_write_length = length;

    /* Confirmation of the end of transmission. */
    _ux_utility_semaphore_put(&slave_write_semaphore);

    /* Done here.  */
    return(UX_SUCCESS);
}

void  tx_test_thread_dummy_entry(ULONG arg)
{
    _ux_utility_thread_suspend(_ux_utility_thread_identify());
}

void  tx_test_thread_slave_simulation_entry(ULONG arg)
{

UINT                                                status;
ULONG                                               actual_length;
UX_SLAVE_CLASS_CDC_ACM_LINE_STATE_PARAMETER         line_state;
UX_SLAVE_CLASS_CDC_ACM_CALLBACK_PARAMETER           callback;
UINT                                                i;
ULONG                                               rmem_free;
ALIGN_TYPE                                          tmp;
UX_SLAVE_INTERFACE                                  *interface;


    /* Test device. */
    stepinfo(">>>>>>>>>>>>>>>> Test device connect\n");
    ux_test_breakable_sleep(200, break_on_cdc_acm_all_ready);
    if (!(cdc_acm_host_control && cdc_acm_host_data && cdc_acm_slave))
    {
        printf("ERROR #%d: connect fail\n", __LINE__);
        test_control_return(1);
    }

#ifndef UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE

    stepinfo(">>>>>>>>>>>>>>>> Test device DTR state to start transmission\n");
    line_state.ux_slave_class_cdc_acm_parameter_dtr = 0;
    line_state.ux_slave_class_cdc_acm_parameter_rts = 0;
    ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_LINE_STATE, &line_state);
    _ux_utility_thread_resume(&tx_test_thread_host_simulation);
    for (i = 0; i < 20; i ++)
    {
        _ux_utility_delay_ms(25);
        status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE, &line_state);
        if (status != UX_SUCCESS)
            break;
        if (line_state.ux_slave_class_cdc_acm_parameter_dtr)
            break;
    }
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (line_state.ux_slave_class_cdc_acm_parameter_dtr != UX_TRUE)
    {

        printf("ERROR #%d: DTR not set\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device ioctl UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START\n");

    /* Set the callback parameter. */
    callback.ux_device_class_cdc_acm_parameter_write_callback = tx_demo_thread_slave_write_callback;
    callback.ux_device_class_cdc_acm_parameter_read_callback = tx_demo_thread_slave_read_callback;

    /* Create the semaphore to synchronize with reception.  */
    status =  _ux_utility_semaphore_create(&slave_read_semaphore, "slave_read_semaphore", 0);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: slave_read_semaphore fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the semaphore to synchronize with transmission.  */
    status =  _ux_utility_semaphore_create(&slave_write_semaphore, "slave_write_semaphore", 0);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: slave_write_semaphore fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    error_callback_ignore = UX_TRUE;
    rmem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    stepinfo(">>>>>>>>>>>>>>>> Test device ioctl UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START status fail\n");
    cdc_acm_slave->ux_slave_class_cdc_acm_transmission_status = UX_TRUE;
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, &callback);
    if (status != UX_ERROR)
    {
        printf("ERROR #%d: stack alloc fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    cdc_acm_slave->ux_slave_class_cdc_acm_transmission_status = UX_FALSE;

#if 0 /* Resources management moved to initialize/uninitialize  */
    stepinfo(">>>>>>>>>>>>>>>> Test device ioctl UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START bulkout stack fail\n");
    ux_test_utility_sim_mem_allocate_until_flagged(UX_THREAD_STACK_SIZE, UX_REGULAR_MEMORY);
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, &callback);
    if (status != UX_MEMORY_INSUFFICIENT)
    {
        printf("ERROR #%d: stack alloc fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    ux_test_utility_sim_mem_free_all();

    stepinfo(">>>>>>>>>>>>>>>> Test device ioctl UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START bulkin stack fail\n");
    ux_test_utility_sim_mem_allocate_until_flagged(2 * UX_THREAD_STACK_SIZE, UX_REGULAR_MEMORY);
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, &callback);
    if (status != UX_MEMORY_INSUFFICIENT)
    {
        printf("ERROR #%d: stack alloc fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    ux_test_utility_sim_mem_free_all();

    stepinfo(">>>>>>>>>>>>>>>> Test device ioctl UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START flag fail\n");
    _ux_utility_event_flags_create(&cdc_acm_slave->ux_slave_class_cdc_acm_event_flags_group, "test");
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, &callback);
    if (status != UX_EVENT_ERROR)
    {
        printf("ERROR #%d: stack alloc fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_utility_event_flags_delete(&cdc_acm_slave->ux_slave_class_cdc_acm_event_flags_group);

    stepinfo(">>>>>>>>>>>>>>>> Test device ioctl UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START bulkin thread fail\n");
    _ux_utility_thread_create(&cdc_acm_slave->ux_slave_class_cdc_acm_bulkin_thread, "test",
                        tx_test_thread_dummy_entry,
                        (ULONG)(ALIGN_TYPE)cdc_acm_slave, (VOID *)buffer,
                        UX_THREAD_STACK_SIZE, UX_THREAD_PRIORITY_CLASS,
                        UX_THREAD_PRIORITY_CLASS, UX_NO_TIME_SLICE, TX_DONT_START);
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, &callback);
    if (status != UX_THREAD_ERROR)
    {
        printf("ERROR #%d: thread create fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_utility_thread_delete(&cdc_acm_slave->ux_slave_class_cdc_acm_bulkin_thread);

    stepinfo(">>>>>>>>>>>>>>>> Test device ioctl UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START bulkout thread fail\n");
    _ux_utility_thread_create(&cdc_acm_slave->ux_slave_class_cdc_acm_bulkout_thread, "test",
                        tx_test_thread_dummy_entry,
                        (ULONG)(ALIGN_TYPE)cdc_acm_slave, (VOID *)buffer,
                        UX_THREAD_STACK_SIZE, UX_THREAD_PRIORITY_CLASS,
                        UX_THREAD_PRIORITY_CLASS, UX_NO_TIME_SLICE, TX_DONT_START);
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, &callback);
    if (status != UX_THREAD_ERROR)
    {
        printf("ERROR #%d: thread create fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_utility_thread_delete(&cdc_acm_slave->ux_slave_class_cdc_acm_bulkout_thread);
#endif

    /* Check memory leak.  */
    if (rmem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
    {
        printf("ERROR #%d: memory level changed %ld <> %ld\n", __LINE__, rmem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
        test_control_return(1);
    }
    error_callback_ignore = UX_TRUE;

    stepinfo(">>>>>>>>>>>>>>>> Test device ioctl UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START good\n");
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, &callback);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device read/write on transmission mode\n");

    status = ux_device_class_cdc_acm_read(cdc_acm_slave, buffer, 64, &actual_length);
    if (status != UX_ERROR)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    status = ux_device_class_cdc_acm_write(cdc_acm_slave, buffer, 64, &actual_length);
    if (status != UX_ERROR)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device write_with_callback\n");
    status = ux_device_class_cdc_acm_write_with_callback(cdc_acm_slave, buffer, 64);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device write_with_callback busy\n");
    status = ux_device_class_cdc_acm_write_with_callback(cdc_acm_slave, buffer, 64);
    if (status != UX_ERROR)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device write_with_callback state fail\n");
    error_callback_ignore = UX_TRUE;
    tmp = _ux_system_slave->ux_system_slave_device.ux_slave_device_state;
    _ux_system_slave->ux_system_slave_device.ux_slave_device_state = UX_DEVICE_ADDRESSED;
    status = ux_device_class_cdc_acm_write_with_callback(cdc_acm_slave, buffer, 64);
    if (status != UX_CONFIGURATION_HANDLE_UNKNOWN)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Wait a while to let threads run.  */
    _ux_utility_delay_ms(20);

    /* Push the bulkout thread so it runs to check status.  */
    _ux_utility_thread_resume(&tx_test_thread_host_simulation);
    _ux_utility_thread_resume(&cdc_acm_slave->ux_slave_class_cdc_acm_bulkout_thread);
    _ux_utility_delay_ms(20);
    _ux_utility_thread_resume(&cdc_acm_slave->ux_slave_class_cdc_acm_bulkout_thread);

    _ux_system_slave->ux_system_slave_device.ux_slave_device_state = (ULONG)tmp;
    error_callback_ignore = UX_FALSE;

    /* Wait a while to let threads run.  */
    _ux_utility_delay_ms(20);

    stepinfo(">>>>>>>>>>>>>>>> Test device ioctl UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP\n");
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP, UX_NULL);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device ioctl UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP when stopped\n");
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP, UX_NULL);
    if (status != UX_ERROR)
    {
        printf("ERROR #%d: UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device write_with_callback wrong mode\n");
    status = ux_device_class_cdc_acm_write_with_callback(cdc_acm_slave, buffer, 64);
    if (status != UX_ERROR)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device transmission\n");

    /* Swap endpoints.  */
    interface = cdc_acm_slave->ux_slave_class_cdc_acm_interface;
    tmp = (ALIGN_TYPE)interface->ux_slave_interface_first_endpoint;
    interface->ux_slave_interface_first_endpoint = interface->ux_slave_interface_first_endpoint->ux_slave_endpoint_next_endpoint;
    interface->ux_slave_interface_first_endpoint->ux_slave_endpoint_next_endpoint = (UX_SLAVE_ENDPOINT*)tmp;
    interface->ux_slave_interface_first_endpoint->ux_slave_endpoint_next_endpoint->ux_slave_endpoint_next_endpoint = UX_NULL;

    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, &callback);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device transmission unconfigued\n");
    tmp = _ux_system_slave->ux_system_slave_device.ux_slave_device_state;
    _ux_system_slave->ux_system_slave_device.ux_slave_device_state = UX_DEVICE_ADDRESSED;
    _ux_utility_thread_resume(&cdc_acm_slave->ux_slave_class_cdc_acm_bulkin_thread);
    _ux_utility_thread_resume(&cdc_acm_slave->ux_slave_class_cdc_acm_bulkout_thread);
    _ux_utility_delay_ms(10);
    _ux_system_slave->ux_system_slave_device.ux_slave_device_state = (ULONG)tmp;

    stepinfo(">>>>>>>>>>>>>>>> Test device transmission event flags error\n");
    _ux_utility_event_flags_delete(&cdc_acm_slave->ux_slave_class_cdc_acm_event_flags_group);
    _ux_utility_thread_resume(&cdc_acm_slave->ux_slave_class_cdc_acm_bulkin_thread);
    _ux_utility_delay_ms(10);
    _ux_utility_event_flags_create(&cdc_acm_slave->ux_slave_class_cdc_acm_event_flags_group, "ux_device_class_cdc_acm_event_flag");

    stepinfo(">>>>>>>>>>>>>>>> Test device transmission event flags not expected\n");
    _ux_utility_thread_resume(&cdc_acm_slave->ux_slave_class_cdc_acm_bulkin_thread);
    _ux_utility_event_flags_set(&cdc_acm_slave->ux_slave_class_cdc_acm_event_flags_group, UX_DEVICE_CLASS_CDC_ACM_WRITE_EVENT << 1, TX_OR);
    _ux_utility_event_flags_set(&cdc_acm_slave->ux_slave_class_cdc_acm_event_flags_group, UX_DEVICE_CLASS_CDC_ACM_WRITE_EVENT, TX_OR);
    _ux_utility_delay_ms(10);

    stepinfo(">>>>>>>>>>>>>>>> Test device ioctl UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION _STOP & _START\n");
    status  = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP, UX_NULL);
    status |= ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, &callback);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION _STOP or _START fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_utility_delay_ms(10);
    status  = ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP, UX_NULL);
    status |= ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_START, &callback);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION _STOP or _START fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    while(slave_write_semaphore.tx_semaphore_count)
        _ux_utility_semaphore_get(&slave_write_semaphore, 0);

    stepinfo(">>>>>>>>>>>>>>>> Test device _ux_device_class_cdc_acm_write_with_callback 1\n");
    host_buffer[0] = 0;
    buffer[0] = '1';
    _ux_utility_thread_resume(&tx_test_thread_host_simulation);
    status  = ux_device_class_cdc_acm_write_with_callback(cdc_acm_slave, buffer, 1);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _ux_utility_semaphore_get(&slave_write_semaphore, 100);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (slave_write_status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, slave_write_status);
        test_control_return(1);
    }
    if (slave_write_length != 1)
    {
        printf("ERROR #%d: length %ld\n", __LINE__, slave_write_length);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device _ux_device_class_cdc_acm_write_with_callback 128\n");
    _ux_utility_memory_set(buffer, 128, 128);
    _ux_utility_memory_set(host_buffer, 0, 512);
    _ux_utility_memory_set(&buffer[128], '1', 512-128);
    _ux_utility_thread_resume(&tx_test_thread_host_simulation);
    status = ux_device_class_cdc_acm_write_with_callback(cdc_acm_slave, buffer, 128);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _ux_utility_semaphore_get(&slave_write_semaphore, 100);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (slave_write_status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, slave_write_status);
        test_control_return(1);
    }
    if (slave_write_length != 128)
    {
        printf("ERROR #%d: length %ld\n", __LINE__, slave_write_length);
        test_control_return(1);
    }

#if !defined(UX_DEVICE_CLASS_CDC_ACM_WRITE_AUTO_ZLP) /* ZLP auto appended in autoZLP case.  */
    stepinfo(">>>>>>>>>>>>>>>> Test device _ux_device_class_cdc_acm_write_with_callback ZLP\n");
    status = ux_device_class_cdc_acm_write_with_callback(cdc_acm_slave, UX_NULL, 0);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _ux_utility_semaphore_get(&slave_write_semaphore, 100);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (slave_write_status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, slave_write_status);
        test_control_return(1);
    }
    if (slave_write_length != 0)
    {
        printf("ERROR #%d: length %ld\n", __LINE__, slave_write_length);
        test_control_return(1);
    }
#endif

    stepinfo(">>>>>>>>>>>>>>>> Test device _ux_device_class_cdc_acm_write_with_callback error\n");
    ux_test_dcd_sim_slave_set_actions(error_on_transfer0);
    status = ux_device_class_cdc_acm_write_with_callback(cdc_acm_slave, buffer, 12);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _ux_utility_semaphore_get(&slave_write_semaphore, 100);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (slave_write_status != UX_ERROR)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, slave_write_status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device _ux_device_class_cdc_acm_write_with_callback no callback\n");
    cdc_acm_slave->ux_device_class_cdc_acm_write_callback = UX_NULL;
    ux_test_dcd_sim_slave_set_actions(error_on_transfer0);
    status = ux_device_class_cdc_acm_write_with_callback(cdc_acm_slave, buffer, 12);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = _ux_utility_semaphore_get(&slave_write_semaphore, 100);
    if (status == UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    cdc_acm_slave->ux_device_class_cdc_acm_write_callback = callback.ux_device_class_cdc_acm_parameter_write_callback;

    /* Flush read semaphore.  */
    while(UX_SUCCESS == _ux_utility_semaphore_get(&slave_read_semaphore, 0));

    stepinfo(">>>>>>>>>>>>>>>> Test device _read_callback, expect 64+1+error >>>\n");
    actions_2_set_trigger = 2;
    actions_2_set = error_on_transfer1;
    slave_read_counter = 0;
    slave_read_log[0].save2 = &buffer[512];
    slave_read_log[1].save2 = &buffer[512+64];
    slave_read_log[2].save2 = UX_NULL;
    slave_read_log[3].save2 = UX_NULL;
    slave_read_log[4].save2 = UX_NULL;
    _ux_utility_thread_resume(&tx_test_thread_host_simulation);

    stepinfo(">>>>>>>>>>>>>>>> Test device _read_callback, expect 64\n");
    status = _ux_utility_semaphore_get(&slave_read_semaphore, 100);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (slave_read_log[0].status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (slave_read_log[0].length != 64)
    {
        printf("ERROR #%d: length %ld\n", __LINE__, slave_read_log[0].length);
        test_control_return(1);
    }
    if (_ux_utility_memory_compare(host_buffer, slave_read_log[0].save2, 64) != UX_SUCCESS)
    {
        printf("ERROR #%d: data error\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device _read_callback, expect 1\n");
    status = _ux_utility_semaphore_get(&slave_read_semaphore, 100);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (slave_read_log[1].status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (slave_read_log[1].length != 1)
    {
        printf("ERROR #%d: length %ld\n", __LINE__, slave_read_log[1].length);
        test_control_return(1);
    }
    if (_ux_utility_memory_compare(host_buffer, slave_read_log[1].save2, 1) != UX_SUCCESS)
    {
        printf("ERROR #%d: data error\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device _read_callback, expect ZL\n");
    status = _ux_utility_semaphore_get(&slave_read_semaphore, 100);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (slave_read_log[2].status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (slave_read_log[2].length != 0)
    {
        printf("ERROR #%d: length %ld\n", __LINE__, slave_read_log[1].length);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device _read_callback, expect timeout\n");
    status = _ux_utility_semaphore_get(&slave_read_semaphore, 100);
    if (status == UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device _read_ no callback, expect timeout >>>\n");
    cdc_acm_slave->ux_device_class_cdc_acm_read_callback = UX_NULL;
    _ux_utility_thread_resume(&tx_test_thread_host_simulation);

    stepinfo(">>>>>>>>>>>>>>>> Test device _read_ no callback, expect timeout\n");
    status = _ux_utility_semaphore_get(&slave_read_semaphore, 100);
    if (status == UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test device _read_ request error no callback, expect timeout\n");
    ux_test_dcd_sim_slave_set_actions(error_on_transfer1);
    _ux_utility_thread_resume(&tx_test_thread_host_simulation);
    status = _ux_utility_semaphore_get(&slave_read_semaphore, 500);
    if (status == UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    cdc_acm_slave->ux_device_class_cdc_acm_read_callback = callback.ux_device_class_cdc_acm_parameter_read_callback;

#endif

    /* End */
    stepinfo(">>>>>>>>>>>>>>>> Test device done\n");
    _ux_utility_thread_resume(&tx_test_thread_host_simulation);
    while(1)
    {

        /* Sleep so ThreadX on Win32 will delete this thread. */
        ux_utility_delay_ms(100);
    }
}
