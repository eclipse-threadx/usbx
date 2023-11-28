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
UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
UX_HOST_CLASS                       *class_driver;
UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_control;
UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_data;

UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave;
UCHAR                               cdc_acm_slave_change;
UX_SLAVE_CLASS_CDC_ACM_PARAMETER    parameter;

ULONG                               error_counter;

ULONG                               set_cfg_counter;

ULONG                               rsc_mem_free_on_set_cfg;
ULONG                               rsc_sem_on_set_cfg;
ULONG                               rsc_sem_get_on_set_cfg;
ULONG                               rsc_mutex_on_set_cfg;

ULONG                               rsc_enum_sem_usage;
ULONG                               rsc_enum_sem_get_count;
ULONG                               rsc_enum_mutex_usage;
ULONG                               rsc_enum_mem_usage;

/* Define device framework.  */

#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      161
#define             DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED      171
#define             STRING_FRAMEWORK_LENGTH                 47
#define             LANGUAGE_ID_FRAMEWORK_LENGTH            2

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
    0x02, /* bNumConfigurations */

    /* Configuration 1 descriptor 9 bytes, total 68 bytes */
    0x09, 0x02, 0x44, 0x00, /* ,,wTotalLength */
    0x02, 0x01, 0x00,       /* ,bConfigurationValue, */
    0x40, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,       /* ,,bInterfaceNumber */
    0x00,                   /* bAlternateSetting */
    0x00,                   /* bNumEndpoints */
    0x02, 0x02, 0x01,       /* bInterfaceClass,SubClass,Protocol */
    0x00,

    /* Header Functional Descriptor 5 bytes */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor 4 bytes */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor 5 bytes */
    0x05, 0x24, 0x06,
    0x00,                          /* Master interface */
    0x01,                          /* Slave interface  */

    /* Call Management Functional Descriptor 5 bytes */
    0x05, 0x24, 0x01,
    0x03,
    0x01,                          /* Data interface   */

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02,
    0x02,
    0x40, 0x00,
    0x00,

    /* Configuration 2 descriptor 9 bytes, total 75 bytes */
    0x09, 0x02, 0x4b, 0x00, /* ,,wTotalLength */
    0x02, 0x02, 0x00,       /* ,bConfigurationValue, */
    0x40, 0xFA,             /* bmAttributes,bMaxPower () */

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,       /* ,,bInterfaceNumber */
    0x00,                   /* bAlternateSetting */
    0x01,                   /* bNumEndpoints */
    0x02, 0x02, 0x01,       /* bInterfaceClass,SubClass,Protocol */
    0x00,

    /* Header Functional Descriptor 5 bytes */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor 4 bytes */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor 5 bytes */
    0x05, 0x24, 0x06,
    0x00,                          /* Master interface */
    0x01,                          /* Slave interface  */

    /* Call Management Functional Descriptor 5 bytes */
    0x05, 0x24, 0x01,
    0x03,
    0x01,                          /* Data interface   */

    /* Endpoint 0x83 descriptor 7 bytes */
    0x07, 0x05, 0x83,          /* ,,bEndpointAddress */
    0x03,
    0x08, 0x00,
    0xFF,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02,
    0x02,
    0x40, 0x00,
    0x00,
};

#define DEVICE_FRAMEWORK_EPA_POS_1_FS (DEVICE_FRAMEWORK_LENGTH_FULL_SPEED - 14 + 2)
#define DEVICE_FRAMEWORK_EPA_POS_2_FS (DEVICE_FRAMEWORK_LENGTH_FULL_SPEED - 7 + 2)

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
    0x02, /* bNumConfigurations */

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02,
    0x02, 0x00, 0x00,
    0x40,
    0x01,
    0x00,

    /* Configuration 1 descriptor 9 bytes, total 68 bytes */
    0x09, 0x02, 0x44, 0x00, /* ,,wTotalLength */
    0x02, 0x01, 0x00,       /* ,bConfigurationValue, */
    0x40, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,       /* ,,bInterfaceNumber */
    0x00,                   /* bAlternateSetting */
    0x00,                   /* bNumEndpoints */
    0x02, 0x02, 0x01,       /* bInterfaceClass,SubClass,Protocol */
    0x00,

    /* Header Functional Descriptor 5 bytes */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor 4 bytes */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor 5 bytes */
    0x05, 0x24, 0x06,
    0x00,                          /* Master interface */
    0x01,                          /* Slave interface  */

    /* Call Management Functional Descriptor 5 bytes */
    0x05, 0x24, 0x01,
    0x03,
    0x01,                          /* Data interface   */

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02,
    0x02,
    0x40, 0x00,
    0x00,

    /* Configuration 2 descriptor 9 bytes, total 75 bytes */
    0x09, 0x02, 0x4b, 0x00, /* ,,wTotalLength */
    0x02, 0x02, 0x00,       /* ,bConfigurationValue, */
    0x40, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,       /* ,,bInterfaceNumber */
    0x00,                   /* bAlternateSetting */
    0x01,                   /* bNumEndpoints */
    0x02, 0x02, 0x01,       /* bInterfaceClass,SubClass,Protocol */
    0x00,

    /* Header Functional Descriptor 5 bytes */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor 4 bytes */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor 5 bytes */
    0x05, 0x24, 0x06,
    0x00,                          /* Master interface */
    0x01,                          /* Slave interface  */

    /* Call Management Functional Descriptor 5 bytes */
    0x05, 0x24, 0x01,
    0x03,
    0x01,                          /* Data interface   */

    /* Endpoint 0x83 descriptor 7 bytes */
    0x07, 0x05, 0x83,          /* ,,bEndpointAddress */
    0x03,
    0x08, 0x00,
    0xFF,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81,
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02,
    0x02,
    0x40, 0x00,
    0x00,
};

#define DEVICE_FRAMEWORK_EPA_POS_1_HS (DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED - 14 + 2)
#define DEVICE_FRAMEWORK_EPA_POS_2_HS (DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED - 7 + 2)

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


    /* Multiple languages are supported on the device, to add
       a language besides english, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
static unsigned char language_id_framework[] = {

    /* English. */
        0x09, 0x04
};

static UX_TEST_SETUP _SetConfigure = UX_TEST_SETUP_SetConfigure;

static UX_TEST_HCD_SIM_ACTION error_on_SetCfg[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_SetConfigure,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_ERROR, UX_NULL}, /* Error */
{   0   }
};

static UX_CONFIGURATION *cfg_2_modify;
static VOID ux_test_hcd_entry_interaction_set_cfg(UX_TEST_ACTION *action, VOID *_params)
{

    if (cfg_2_modify)
        cfg_2_modify -> ux_configuration_handle = 0;
}

static UX_TEST_HCD_SIM_ACTION corrupt_configuration_on_SetCfg[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_SetConfigure,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_SUCCESS, ux_test_hcd_entry_interaction_set_cfg,
        UX_TRUE}, /* Go on */
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
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_cdc_acm_configure_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;

    printf("Running CDC ACM Configure Test...................................... ");

    /* Reset testing counts. */
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    /* Reset error generations */
    ux_test_utility_sim_sem_error_generation_stop();
    ux_test_utility_sim_mutex_error_generation_stop();
    ux_test_utility_sim_sem_get_error_generation_stop();

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
    status =  ux_host_stack_initialize(test_host_change_function);
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

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1,0,  &parameter);

    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
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
ULONG                                               test_n;
ULONG                                               mem_free;
UX_HOST_CLASS_COMMAND                               class_command;
UX_CONFIGURATION                                   *configuration;
UX_HOST_CLASS_CDC_ACM                              *cdc_control;
UX_HOST_CLASS_CDC_ACM                              *cdc_data;
UX_DEVICE                                          *device;
UX_DEVICE                                          *parent_device;
UX_INTERFACE                                       *interface;

    stepinfo("\n");

    /* Test connect. */
    stepinfo(">>>>>>>>>>>>>>>> Test connect\n");
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    tx_thread_sleep(100);
    if (cdc_acm_host_control == UX_NULL || cdc_acm_host_data == UX_NULL || cdc_acm_slave == UX_NULL)
    {

        printf("ERROR #%d: connection not detected\n", __LINE__);
        test_control_return(1);
    }
    mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    cdc_control = cdc_acm_host_control;
    cdc_data = cdc_acm_host_data;

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    test_n = 10;
    while((cdc_acm_host_control || cdc_acm_host_data || cdc_acm_slave) && test_n --)
        tx_thread_sleep(10);

    if (cdc_acm_host_control || cdc_acm_host_data || cdc_acm_slave)
    {

        printf("ERROR #%d: instance not removed when disconnect, %p %p %p\n", __LINE__, cdc_acm_host_control, cdc_acm_host_data, cdc_acm_slave);
        test_control_return(1);
    }
    if (_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available <= mem_free)
    {

        printf("ERROR #%d: memory not freed when disconnect\n", __LINE__);
        test_control_return(1);
    }

    /* Test configure function */
    stepinfo(">>>>>>>>>>>>>>>> Test Configure\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    /* Now connect, configuration stopped because power issue */
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    tx_thread_sleep(100);
    /* Find device */
    status = _ux_host_stack_device_get(0, &device);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    /* Reset configuration */
    status = ux_host_stack_device_configuration_reset(device);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    /* Find configuration */
    status = _ux_host_stack_device_configuration_get(device, 0, &configuration);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    /* Force power source */
    device -> ux_device_power_source =  UX_DEVICE_BUS_POWERED;
    /* Activate interfaces */
    class_command.ux_host_class_command_request   = UX_HOST_CLASS_COMMAND_ACTIVATE;
    /* Control interface */
    interface = configuration -> ux_configuration_first_interface;
    class_command.ux_host_class_command_container = (VOID *)interface;
    class_command.ux_host_class_command_class_ptr = interface->ux_interface_class;
    status = interface->ux_interface_class->ux_host_class_entry_function(&class_command);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    /* Data interface */
    interface = configuration -> ux_configuration_first_interface->ux_interface_next_interface;
    class_command.ux_host_class_command_container = (VOID *)interface;
    class_command.ux_host_class_command_class_ptr = interface->ux_interface_class;
    status = interface->ux_interface_class->ux_host_class_entry_function(&class_command);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test Configure ERROR\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    /* Now connect, configuration stopped because power issue */
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    tx_thread_sleep(100);
    /* Find device */
    status = _ux_host_stack_device_get(0, &device);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    /* Reset configuration */
    status = ux_host_stack_device_configuration_reset(device);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    /* Find configuration */
    status = _ux_host_stack_device_configuration_get(device, 0, &configuration);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
#if UX_MAX_DEVICES > 1
    stepinfo(">>>>>>>>>>>>>>>> Test Configure ERROR - Parent power fail\n");
    /* Get a device */
    parent_device = _ux_host_stack_new_device_get();
    /* Use this device as parent */
    parent_device -> ux_device_power_source = UX_DEVICE_BUS_POWERED;
    device -> ux_device_parent = parent_device;
    /* Force power source */
    device -> ux_device_power_source =  UX_DEVICE_BUS_POWERED;
    /* Activate interfaces */
    class_command.ux_host_class_command_request   = UX_HOST_CLASS_COMMAND_ACTIVATE;
    /* Control interface */
    interface = configuration -> ux_configuration_first_interface;
    class_command.ux_host_class_command_container = (VOID *)interface;
    class_command.ux_host_class_command_class_ptr = interface->ux_interface_class;
    status = interface->ux_interface_class->ux_host_class_entry_function(&class_command);
    if (status != UX_CONNECTION_INCOMPATIBLE)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test Configure ERROR - Configuration handler\n");
    device -> ux_device_handle = 0;
    status = interface->ux_interface_class->ux_host_class_entry_function(&class_command);
    if (status != UX_CONFIGURATION_HANDLE_UNKNOWN)
    {

        printf("ERROR %d: expect UX_CONFIGURATION_HANDLE_UNKNOWN but got 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    device -> ux_device_handle = (ULONG) (ALIGN_TYPE) device;

    stepinfo(">>>>>>>>>>>>>>>> Test Configure ERROR - Parent power OK but SetConfigure error\n");
    parent_device -> ux_device_power_source = UX_DEVICE_SELF_POWERED;
    ux_test_hcd_sim_host_set_actions(error_on_SetCfg);
    status = interface->ux_interface_class->ux_host_class_entry_function(&class_command);
    if (status == UX_SUCCESS)
    {

        printf("ERROR %d: expect fail\n", __LINE__);
        test_control_return(1);
    }
#endif
    stepinfo(">>>>>>>>>>>>>>>> Test Configure ERROR - Device power OK but SetConfigure error\n");
    device -> ux_device_power_source = UX_DEVICE_SELF_POWERED;
    ux_test_hcd_sim_host_set_actions(error_on_SetCfg);
    status = interface->ux_interface_class->ux_host_class_entry_function(&class_command);
    if (status == UX_SUCCESS)
    {

        printf("ERROR %d: expect fail\n", __LINE__);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>>>>>> Test Configure ERROR - SetConfigure\n");
    UX_DEVICE_PARENT_SET(device, UX_NULL);
    ux_test_hcd_sim_host_set_actions(error_on_SetCfg);
    status = interface->ux_interface_class->ux_host_class_entry_function(&class_command);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    stepinfo(">>>>>>>>>>>>>>>> Test Configure ERROR - Interface get\n");
    /* Break the configuration */
    cfg_2_modify = configuration;
    ux_test_hcd_sim_host_set_actions(corrupt_configuration_on_SetCfg);
    status = interface->ux_interface_class->ux_host_class_entry_function(&class_command);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: %x\n", __LINE__, status);
        test_control_return(1);
    }
    /* Restore */
    cfg_2_modify = UX_NULL;
    configuration->ux_configuration_handle = (ULONG)(ALIGN_TYPE)configuration;

    /* Deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);

}

void  tx_test_thread_slave_simulation_entry(ULONG arg)
{

    while(1)
    {

        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
    }
}
