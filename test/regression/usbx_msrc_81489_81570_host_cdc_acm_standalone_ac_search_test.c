/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_device_class_cdc_acm.h"
#include "ux_device_stack.h"
#include "ux_host_class_cdc_acm.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

/* Define constants.  */
#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024

#define                             UX_DEMO_MEMORY_SIZE     (64*1024)

/* Define local/extern function prototypes.  */
static TX_THREAD   ux_test_thread_host_simulation;
static TX_THREAD   ux_test_thread_device_simulation;
static void        ux_test_thread_host_simulation_entry(ULONG);
static void        ux_test_thread_device_simulation_entry(ULONG);

static VOID        test_cdc_instance_activate(VOID  *cdc_instance);
static VOID        test_cdc_instance_deactivate(VOID *cdc_instance);

static VOID        ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);

/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UX_HOST_CLASS                       *class_driver;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_control;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_data;

static UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER    parameter;
static UCHAR                               cdc_acm_slave_change;

static ULONG                               set_cfg_counter;

static ULONG                               rsc_mem_alloc_cnt_on_set_cfg;
static ULONG                               rsc_sem_on_set_cfg;
static ULONG                               rsc_sem_get_on_set_cfg;
static ULONG                               rsc_mutex_on_set_cfg;

static ULONG                               rsc_enum_sem_usage;
static ULONG                               rsc_enum_sem_get_count;
static ULONG                               rsc_enum_mutex_usage;
static ULONG                               rsc_enum_mem_alloc_count;

static ULONG                               rsc_cdc_sem_usage;
static ULONG                               rsc_cdc_sem_get_count;
static ULONG                               rsc_cdc_mutex_usage;
static ULONG                               rsc_cdc_mem_alloc_count;

static UCHAR                               error_callback_ignore = UX_TRUE;
static ULONG                               error_callback_counter;

/* Define device framework.  */

#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      93
#define             DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED      103
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
    0x01,

    /* Configuration 1 descriptor 9 bytes */
    0x09, 0x02, 0x4b, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,
    0x00,
    0x01,
    0x02, 0x02, 0x01,
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
    0x07, 0x05, 0x83,
    0x03,
    0x08, 0x00,
    0xFF,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02, /* @ 93 - 14 + 2 = 81 */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81, /* @ 93 - 7 + 2 = 88 */
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
    0x01,

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02,
    0x02, 0x00, 0x00,
    0x40,
    0x01,
    0x00,

    /* Configuration 1 descriptor */
    0x09, 0x02, 0x4b, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement */
    0x09, 0x04, 0x00,
    0x00,
    0x01,
    0x02, 0x02, 0x01,
    0x00,

    /* Header Functional Descriptor */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor */
    0x05, 0x24, 0x06,
    0x00,
    0x01,

    /* Call Management Functional Descriptor */
    0x05, 0x24, 0x01,
    0x00,
    0x01,

    /* Endpoint 0x83 descriptor */
    0x07, 0x05, 0x83,
    0x03,
    0x08, 0x00,
    0xFF,

    /* Data Class Interface Descriptor Requirement */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor */
    0x07, 0x05, 0x02, /* @ 103 - 14 + 2 = 91 */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor */
    0x07, 0x05, 0x81, /* @ 103 - 7 + 2 = 98 */
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

/* Setup requests */

static UX_TEST_SETUP _SetConfigure = UX_TEST_SETUP_SetConfigure;

static UX_TEST_HCD_SIM_ACTION log_on_SetCfg[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_SetConfigure,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_SUCCESS, ux_test_hcd_entry_set_cfg,
        UX_TRUE}, /* Invoke callback & continue */
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

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        {
            /* Failed test.  */
            printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
            test_control_return(1);
        }
    }
}

static UINT  sleep_break_on_error(VOID)
{

    if (error_callback_counter >= 3)
        return error_callback_counter;

    return UX_SUCCESS;
}

static UINT  demo_class_cdc_acm_get(void)
{

UINT                                status;
UX_HOST_CLASS                       *class_inst;
UX_HOST_CLASS_CDC_ACM               *cdc_acm_host;
ULONG                               n_loop = 1000 * 1000;

    /* Wait while background task preparing instances.  */
    while(n_loop)
    {
        if (cdc_acm_host_control && cdc_acm_host_data)
            return(UX_SUCCESS);

#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
#endif
        tx_thread_sleep(1);
        n_loop -= 10;
    }

    /* Return ERROR.  */
    return(UX_ERROR);
}

static UINT demo_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
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
            {
                /* Confirm link to control is cleared on control removal.  */
                UX_TEST_ASSERT(cdc_acm_host_data -> ux_host_class_cdc_acm_control == cdc_acm_host_control);
                cdc_acm_host_data = UX_NULL;
            }
            break;

#if defined(UX_HOST_STANDALONE)
        case UX_STANDALONE_WAIT_BACKGROUND_TASK:
            tx_thread_relinquish();
            break;
#endif

        default:
            break;
    }
    return 0;
}

static VOID test_cdc_instance_activate(VOID *cdc_instance)
{

    /* Save the CDC instance.  */
    cdc_acm_slave = (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;
}
static VOID test_cdc_instance_deactivate(VOID *cdc_instance)
{

    /* Reset the CDC instance.  */
    cdc_acm_slave = UX_NULL;
}

static VOID ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *_params)
{

    set_cfg_counter ++;

    rsc_mem_alloc_cnt_on_set_cfg = ux_test_utility_sim_mem_alloc_count();

    rsc_sem_on_set_cfg = ux_test_utility_sim_sem_create_count();
    rsc_sem_get_on_set_cfg = ux_test_utility_sim_sem_get_count();
    rsc_mutex_on_set_cfg = ux_test_utility_sim_mutex_create_count();
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_msrc_81489_81570_standalone_cdc_acm_ac_search_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;

    /* Inform user.  */
    printf("Running MSRC 81489/570 - STANDALONE CDC ACM AC Search Test.......... ");
#if !defined(UX_HOST_STANDALONE)
    printf("SKIP\n");
    test_control_return(1);
    return;
#endif

    /* Reset testing counts. */
    ux_test_utility_sim_mem_alloc_log_enable(UX_TRUE);
    ux_test_utility_sim_mem_alloc_count_reset();
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
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);
    UX_TEST_CHECK_SUCCESS(status);

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(demo_system_host_change_function);
    UX_TEST_CHECK_SUCCESS(status);

    /* Register CDC-ACM class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_cdc_acm_name, ux_host_class_cdc_acm_entry);
    UX_TEST_CHECK_SUCCESS(status);

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    UX_TEST_CHECK_SUCCESS(status);

    /* Set the parameters for callback when insertion/extraction of a CDC device.  */
    ux_utility_memory_set(&parameter, 0, sizeof(parameter));
    parameter.ux_slave_class_cdc_acm_instance_activate   =  test_cdc_instance_activate;
    parameter.ux_slave_class_cdc_acm_instance_deactivate =  test_cdc_instance_deactivate;

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1,0,  &parameter);
    UX_TEST_CHECK_SUCCESS(status);

    /* Initialize the simulated device controller.  */
    status = _ux_dcd_sim_slave_initialize();
    UX_TEST_CHECK_SUCCESS(status);

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    UX_TEST_CHECK_SUCCESS(status);

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_host_simulation, "tx demo host simulation", ux_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    UX_TEST_ASSERT(status == TX_SUCCESS);

#if 0
    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&ux_test_thread_device_simulation, "tx demo slave simulation", ux_test_thread_device_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    UX_TEST_ASSERT(status == TX_SUCCESS);
#endif
}

static UX_HOST_CLASS_COMMAND  class_command;
static UX_DEVICE              fake_device;
static UX_CONFIGURATION       fake_configuration;
static UX_INTERFACE           fake_interface[2];
static UX_HOST_CLASS_CDC_ACM  fake_cdc[2];
static UX_HOST_CLASS    *good_cls;
static UX_CONFIGURATION *good_cfg;

/* Control must not be linked if not expected.  */
static void _msrc_81489_81570_cdc_activate_data_control_search_link_tests(void)
{
UINT        status;

    stepinfo(">>>>>>>>>>>> ACM Search Test: disconnect - clear class instances\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    UX_TEST_ASSERT(cdc_acm_host_control == UX_NULL);
    UX_TEST_ASSERT(cdc_acm_host_data == UX_NULL);

    stepinfo(">>>>>>>>>>>> ACM Search Test: insert fake class instance in head (DEV/CFG different)\n");
    fake_interface[0].ux_interface_configuration = &fake_configuration;
    fake_interface[0].ux_interface_next_interface = &fake_interface[0];
    fake_interface[1].ux_interface_configuration = &fake_configuration;
    fake_configuration.ux_configuration_first_interface = &fake_interface[0];
    fake_configuration.ux_configuration_device = &fake_device;

    fake_cdc[0].ux_host_class_cdc_acm_interface = &fake_interface[0];
    fake_cdc[0].ux_host_class_cdc_acm_interfaces_bitmap = 0x3u;
    fake_cdc[0].ux_host_class_cdc_acm_control = UX_NULL;
    fake_interface[0].ux_interface_descriptor.bInterfaceNumber = 0;
    fake_interface[0].ux_interface_descriptor.bAlternateSetting = 0;
    fake_interface[0].ux_interface_descriptor.bInterfaceClass = UX_HOST_CLASS_CDC_CONTROL_CLASS;
    fake_interface[0].ux_interface_descriptor.bInterfaceSubClass = UX_HOST_CLASS_CDC_ACM_SUBCLASS;

    fake_cdc[1].ux_host_class_cdc_acm_interface = &fake_interface[1];
    fake_cdc[1].ux_host_class_cdc_acm_control = UX_NULL;
    fake_interface[1].ux_interface_descriptor.bInterfaceNumber = 0;
    fake_interface[1].ux_interface_descriptor.bAlternateSetting = 0;
    fake_interface[1].ux_interface_descriptor.bInterfaceClass = UX_HOST_CLASS_CDC_DATA_CLASS;
    fake_interface[1].ux_interface_descriptor.bInterfaceSubClass = 0;
    _ux_host_stack_class_instance_create(good_cls, (VOID *)&fake_cdc[0]);
    _ux_host_stack_class_instance_create(good_cls, (VOID *)&fake_cdc[1]);

    stepinfo(">>>>>>>>>>>> ACM Search Test: connect - create class instances\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    status =  demo_class_cdc_acm_get();
    UX_TEST_CHECK_SUCCESS(status);

    stepinfo(">>>>>>>>>>>> ACM Search Test: Validate connections\n");
    UX_TEST_ASSERT(cdc_acm_host_control != UX_NULL);
    UX_TEST_ASSERT(cdc_acm_host_data != UX_NULL);
    /* Fakes must not be linked (in normal).  */
    UX_TEST_ASSERT(cdc_acm_host_control -> ux_host_class_cdc_acm_control == cdc_acm_host_control);
    UX_TEST_ASSERT(cdc_acm_host_data -> ux_host_class_cdc_acm_control == cdc_acm_host_control);
    /* Fakes must not be linked (update of fake).  */
    UX_TEST_ASSERT(fake_cdc[0].ux_host_class_cdc_acm_control == UX_NULL);
    UX_TEST_ASSERT(fake_cdc[1].ux_host_class_cdc_acm_control == UX_NULL);

    stepinfo(">>>>>>>>>>>> ACM Search Test: destroy fake class instance\n");
    _ux_host_stack_class_instance_destroy(good_cls, (VOID *)&fake_cdc[0]);
    _ux_host_stack_class_instance_destroy(good_cls, (VOID *)&fake_cdc[1]);

    stepinfo(">>>>>>>>>>>> ACM Search Test: clear class instances\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    UX_TEST_ASSERT(cdc_acm_host_control == UX_NULL);
    UX_TEST_ASSERT(cdc_acm_host_data == UX_NULL);
}

static void  ux_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;

    stepinfo("\n");

    /* Find the cdc_acm class and wait for the link to be up.  */
    status =  demo_class_cdc_acm_get();
    UX_TEST_CHECK_SUCCESS(status);

    good_cls = cdc_acm_host_control -> ux_host_class_cdc_acm_class;
    good_cfg = cdc_acm_host_control -> ux_host_class_cdc_acm_device -> ux_device_current_configuration;

    _msrc_81489_81570_cdc_activate_data_control_search_link_tests();

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);

}
