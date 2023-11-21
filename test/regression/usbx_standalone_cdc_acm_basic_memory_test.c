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
static TX_THREAD   ux_test_thread_host_simulation;
static TX_THREAD   ux_test_thread_slave_simulation;
static void        ux_test_thread_host_simulation_entry(ULONG);
static void        ux_test_thread_slave_simulation_entry(ULONG);

static VOID        test_cdc_instance_activate(VOID  *cdc_instance);
static VOID        test_cdc_instance_deactivate(VOID *cdc_instance);
static VOID        test_cdc_instance_parameter_change(VOID *cdc_instance);

static VOID        ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);

/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UX_HOST_CLASS                       *class_driver;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_control;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_data;

static UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave;
static UCHAR                               cdc_acm_slave_change;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER    parameter;

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

static ULONG                               interaction_count;

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
UX_HOST_CLASS                       *class;
UX_HOST_CLASS_CDC_ACM               *cdc_acm_host;


    /* Find the main cdc_acm container */
    status =  ux_host_stack_class_get(_ux_system_host_class_cdc_acm_name, &class);
    if (status != UX_SUCCESS)
        return(status);

    /* We get the first instance of the cdc_acm device */
    do
    {

        status =  ux_host_stack_class_instance_get(class, 0, (void **) &cdc_acm_host);
#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
#else
        tx_thread_sleep(10);
#endif
    } while (status != UX_SUCCESS);

    /* We still need to wait for the cdc_acm status to be live */
    while (cdc_acm_host -> ux_host_class_cdc_acm_state != UX_HOST_CLASS_INSTANCE_LIVE)
    {
#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
#else
        tx_thread_sleep(10);
#endif
    }

    /* Isolate both the control and data interfaces.  */
    if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_DATA_CLASS)
    {
        /* This is the data interface.  */
        cdc_acm_host_data = cdc_acm_host;

        /* In that case, the second one should be the control interface.  */
        status =  ux_host_stack_class_instance_get(class, 1, (void **) &cdc_acm_host);

        /* Check error.  */
        if (status != UX_SUCCESS)
            return(status);

        /* Check for the control interfaces.  */
        if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
        {

            /* This is the control interface.  */
            cdc_acm_host_control = cdc_acm_host;

            return(UX_SUCCESS);

        }
    }
    else
    {
        /* Check for the control interfaces.  */
        if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
        {

            /* This is the control interface.  */
            cdc_acm_host_control = cdc_acm_host;

            /* In that case, the second one should be the data interface.  */
            status =  ux_host_stack_class_instance_get(class, 1, (void **) &cdc_acm_host);

            /* Check error.  */
            if (status != UX_SUCCESS)
                return(status);

            /* Check for the data interface.  */
            if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_DATA_CLASS)
            {

                /* This is the data interface.  */
                cdc_acm_host_data = cdc_acm_host;

                return(UX_SUCCESS);

            }
        }
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
                cdc_acm_host_data = UX_NULL;
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

static VOID    test_cdc_instance_activate(VOID *cdc_instance)
{

    /* Save the CDC instance.  */
    cdc_acm_slave = (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;
}
static VOID test_cdc_instance_deactivate(VOID *cdc_instance)
{

    /* Reset the CDC instance.  */
    cdc_acm_slave = UX_NULL;
}

static VOID test_cdc_instance_parameter_change(VOID *cdc_instance)
{

    /* Set CDC parameter change flag. */
    cdc_acm_slave_change = UX_TRUE;
}

static VOID test_swap_framework_bulk_ep_descriptors(VOID)
{
UCHAR tmp;

    tmp = device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_1_FS];
    device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_1_FS] = device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_2_FS];
    device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_2_FS] = tmp;

    tmp = device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_1_HS];
    device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_1_HS] = device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_2_HS];
    device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_2_HS] = tmp;
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
void    usbx_standalone_cdc_acm_basic_memory_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;

    /* Inform user.  */
    printf("Running STANDALONE CDC ACM Basic Memory Test........................ ");

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

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #1\n");
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(demo_system_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #2\n");
        test_control_return(1);
    }

    /* Register CDC-ACM class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_cdc_acm_name, ux_host_class_cdc_acm_entry);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #3\n");
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

        printf("ERROR #5\n");
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

        printf("ERROR #6\n");
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #7\n");
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #4\n");
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_host_simulation, "tx demo host simulation", ux_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #8\n");
        test_control_return(1);
    }

    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&ux_test_thread_slave_simulation, "tx demo slave simulation", ux_test_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #9\n");
        test_control_return(1);
    }
}

void  ux_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
UX_SLAVE_CLASS_CDC_ACM *                            cdc_acm_slave_bak;
UX_HOST_CLASS_CDC_ACM *                             cdc_acm_host_ctrl_bak;
UX_HOST_CLASS_CDC_ACM *                             cdc_acm_host_data_bak;
ULONG                                               test_n;
ULONG                                               mem_free;
ULONG                                               retry;

    stepinfo("\n");

    /* Find the cdc_acm class and wait for the link to be up.  */
    status =  demo_class_cdc_acm_get();
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #10\n");
        test_control_return(1);
    }

    /* Save slave instance for later tests. */
    cdc_acm_slave_bak = cdc_acm_slave;
    /* Save host instances for later tests. */
    cdc_acm_host_ctrl_bak = cdc_acm_host_control;
    cdc_acm_host_data_bak = cdc_acm_host_data;

    /* Test disconnect. */
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();

    /* Reset testing counts. */
    ux_test_utility_sim_mem_alloc_count_reset();
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    ux_test_hcd_sim_host_set_actions(log_on_SetCfg);

    /* Save free memory usage. */
    mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    for (retry = 0; (retry < 10) && (cdc_acm_host_control == UX_NULL || cdc_acm_host_data == UX_NULL); retry ++)
        tx_thread_sleep(10);

    /* Log create counts for further tests. */
    rsc_enum_mutex_usage = rsc_mutex_on_set_cfg;
    rsc_enum_sem_usage = rsc_sem_on_set_cfg;
    rsc_enum_mem_alloc_count = rsc_mem_alloc_cnt_on_set_cfg;
    /* Log create counts when instances active for further tests. */
    rsc_cdc_mutex_usage = ux_test_utility_sim_mutex_create_count() - rsc_enum_mutex_usage;
    rsc_cdc_sem_usage = ux_test_utility_sim_sem_create_count() - rsc_enum_sem_usage;
    rsc_cdc_mem_alloc_count = ux_test_utility_sim_mem_alloc_count() - rsc_enum_mem_alloc_count;

    /* Lock log base for tests. */
    ux_test_utility_sim_mem_alloc_log_lock();

    stepinfo("enum mem: %ld\n", rsc_enum_mem_alloc_count);
    stepinfo("cdc mem : %ld\n", rsc_cdc_mem_alloc_count);
    stepinfo("mem free: %ld, %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available);

    /* Simulate detach and attach for FS enumeration,
       and check if there is memory error in normal enumeration.
     */
    stepinfo(">>>>>>>>>>>> Enumeration test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < 3; test_n++)
    {
        stepinfo("%4ld / 2\n", test_n);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #11.%ld: Memory level different after re-enumerations %ld <> %ld\n", test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            test_control_return(1);
        }

        /* Connect. */
        error_callback_counter = 0;
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        /* Wait and break on error. */
        ux_test_breakable_sleep(100, sleep_break_on_error);

        /* Check */
        if (!cdc_acm_host_control || !cdc_acm_host_data)
        {

            printf("ERROR #12.%ld: Enumeration fail\n", test_n);
            test_control_return(1);
        }
    }

    /* Simulate detach and attach for FS enumeration,
       and test possible memory allocation error handlings.
     */
    if (rsc_cdc_mem_alloc_count) stepinfo(">>>>>>>>>>>> Memory errors enumeration test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < rsc_cdc_mem_alloc_count; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_cdc_mem_alloc_count - 1);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #11.%ld: Memory level different after re-enumerations %ld <> %ld\n", test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            test_control_return(1);
        }

        /* Set memory error generation */
        ux_test_utility_sim_mem_alloc_error_generation_start(test_n + rsc_enum_mem_alloc_count);

        /* Connect. */
        error_callback_counter = 0;
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        /* Wait and break on errors. */
        ux_test_breakable_sleep(100, sleep_break_on_error);

        /* Check error */
        if (cdc_acm_host_control && cdc_acm_host_data)
        {

            printf("ERROR #12.%ld: device detected when there is memory error\n", test_n);
            test_control_return(1);
        }
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_mem_alloc_error_generation_stop();
    if (rsc_cdc_mem_alloc_count) stepinfo("\n");

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

void  ux_test_thread_slave_simulation_entry(ULONG arg)
{

    while(1)
    {

        /* Keep running device stack tasks.  */
        ux_system_tasks_run();
        tx_thread_relinquish();
    }
}
