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
static UX_HOST_CLASS_CDC_ACM_RECEPTION     cdc_acm_host_reception;
static UCHAR                               cdc_acm_host_reception_buffer[UX_DEMO_RECEPTION_BUFFER_SIZE];
static UINT                                cdc_acm_host_reception_status = 0;
static ULONG                               cdc_acm_host_reception_count = 0;
static UCHAR                               cdc_acm_host_read_buffer[UX_SLAVE_REQUEST_DATA_MAX_LENGTH * 2];
static ULONG                               cdc_acm_host_read_buffer_length;

static UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER    parameter;
static UCHAR                               cdc_acm_slave_change;
static UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_PARAMETER cdc_acm_slave_line_coding;
static UX_SLAVE_CLASS_CDC_ACM_LINE_STATE_PARAMETER  cdc_acm_slave_line_state;
static ULONG                               device_read_length = UX_SLAVE_REQUEST_DATA_MAX_LENGTH;
static UCHAR                               device_buffer[UX_SLAVE_REQUEST_DATA_MAX_LENGTH * 2];

static UCHAR                               host_buffer[UX_SLAVE_REQUEST_DATA_MAX_LENGTH * 2];

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

    /* Get new paramster.  */
    ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING,
                                (VOID*)&cdc_acm_slave_line_coding);
    ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING,
                                (VOID*)&cdc_acm_slave_line_state);
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
void    usbx_standalone_cdc_acm_basic_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;

    /* Inform user.  */
    printf("Running STANDALONE CDC ACM Basic Test............................... ");

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

#if defined(UX_HOST_STANDALONE)
static void test_host_delay(ULONG ticks)
{
ULONG t_start = tx_time_get();
ULONG t_now, t_elapsed;
    while(1)
    {
        ux_system_tasks_run();
        tx_thread_relinquish();
        if (ticks != TX_WAIT_FOREVER)
        {
            if (ticks == 0)
                break;
            t_now = tx_time_get();
            t_elapsed = _ux_utility_time_elapsed(t_start, t_now);
            if (t_elapsed > ticks)
                break;
        }
    }
}
#else
#define test_host_delay         tx_thread_sleep
#endif

static void test_cdc_acm_device_ioctl_parameters(void)
{
UINT                status;
    if (cdc_acm_slave == UX_NULL)
    {
        printf("ERROR #%d, device instance not ready\n", __LINE__);
        test_control_return(1);
    }

    /* Get and check default line coding.  */
    ux_utility_memory_set(&cdc_acm_slave_line_coding, 0, sizeof(cdc_acm_slave_line_coding));
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING,
                                (VOID*)&cdc_acm_slave_line_coding);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_baudrate ==
                    UX_HOST_CLASS_CDC_ACM_LINE_CODING_DEFAULT_RATE);
    UX_TEST_ASSERT(cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_stop_bit ==
                    UX_HOST_CLASS_CDC_ACM_LINE_CODING_DEFAULT_STOP_BIT);
    UX_TEST_ASSERT(cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_parity ==
                    UX_HOST_CLASS_CDC_ACM_LINE_CODING_DEFAULT_PARITY);
    UX_TEST_ASSERT(cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_data_bit ==
                    UX_HOST_CLASS_CDC_ACM_LINE_CODING_DEFAULT_DATA_BIT);

    /* Set new line coding, read back to test.  */
    cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_baudrate ++;
    cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_stop_bit ++;
    cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_parity ++;
    cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_data_bit ++;
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING,
                                (VOID*)&cdc_acm_slave_line_coding);
    UX_TEST_ASSERT(status == UX_SUCCESS);

    ux_utility_memory_set(&cdc_acm_slave_line_coding, 0, sizeof(cdc_acm_slave_line_coding));
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING,
                                (VOID*)&cdc_acm_slave_line_coding);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_baudrate ==
                    UX_HOST_CLASS_CDC_ACM_LINE_CODING_DEFAULT_RATE + 1);
    UX_TEST_ASSERT(cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_stop_bit ==
                    UX_HOST_CLASS_CDC_ACM_LINE_CODING_DEFAULT_STOP_BIT + 1);
    UX_TEST_ASSERT(cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_parity ==
                    UX_HOST_CLASS_CDC_ACM_LINE_CODING_DEFAULT_PARITY + 1);
    UX_TEST_ASSERT(cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_data_bit ==
                    UX_HOST_CLASS_CDC_ACM_LINE_CODING_DEFAULT_DATA_BIT + 1);

    /* Set line coding back.  */
    cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_baudrate --;
    cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_stop_bit --;
    cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_parity --;
    cdc_acm_slave_line_coding.ux_slave_class_cdc_acm_parameter_data_bit --;
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING,
                                (VOID*)&cdc_acm_slave_line_coding);
    UX_TEST_ASSERT(status == UX_SUCCESS);

    /* Get and check line state.  */
    ux_utility_memory_set(&cdc_acm_slave_line_state, 0, sizeof(cdc_acm_slave_line_state));
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE,
                                (VOID*)&cdc_acm_slave_line_state);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(cdc_acm_slave_line_state.ux_slave_class_cdc_acm_parameter_dtr == 1);
    UX_TEST_ASSERT(cdc_acm_slave_line_state.ux_slave_class_cdc_acm_parameter_rts == 1);

    /* Set new line state, read back to test.  */
    cdc_acm_slave_line_state.ux_slave_class_cdc_acm_parameter_dtr ++;
    cdc_acm_slave_line_state.ux_slave_class_cdc_acm_parameter_rts ++;
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_LINE_STATE,
                                (VOID*)&cdc_acm_slave_line_state);
    UX_TEST_ASSERT(status == UX_SUCCESS);

    ux_utility_memory_set(&cdc_acm_slave_line_state, 0, sizeof(cdc_acm_slave_line_state));
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE,
                                (VOID*)&cdc_acm_slave_line_state);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(cdc_acm_slave_line_state.ux_slave_class_cdc_acm_parameter_dtr == 2);
    UX_TEST_ASSERT(cdc_acm_slave_line_state.ux_slave_class_cdc_acm_parameter_rts == 2);

    /* Set line state back.  */
    cdc_acm_slave_line_state.ux_slave_class_cdc_acm_parameter_dtr --;
    cdc_acm_slave_line_state.ux_slave_class_cdc_acm_parameter_rts --;
    status = ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                                UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_LINE_STATE,
                                (VOID*)&cdc_acm_slave_line_state);
    UX_TEST_ASSERT(status == UX_SUCCESS);
}


static void test_thread_host_reception_callback(UX_HOST_CLASS_CDC_ACM *cdc_acm, UINT status, UCHAR *reception_buffer, ULONG reception_size)
{
ULONG       i;

    /* And move to the next reception buffer.  Check if we are at the end of the application buffer.  */
    if (cdc_acm_host_reception.ux_host_class_cdc_acm_reception_data_tail +
            cdc_acm_host_reception.ux_host_class_cdc_acm_reception_block_size >=
        cdc_acm_host_reception.ux_host_class_cdc_acm_reception_data_buffer +
            cdc_acm_host_reception.ux_host_class_cdc_acm_reception_data_buffer_size)

        /* We are at the end of the buffer. Move back to the beginning.  */
        cdc_acm_host_reception.ux_host_class_cdc_acm_reception_data_tail =
            cdc_acm_host_reception.ux_host_class_cdc_acm_reception_data_buffer;

    else

        /* Program the tail to be after the current buffer.  */
        cdc_acm_host_reception.ux_host_class_cdc_acm_reception_data_tail +=
            cdc_acm_host_reception.ux_host_class_cdc_acm_reception_block_size;

    cdc_acm_host_reception_status = status;
    cdc_acm_host_reception_count ++;

    /* Save buffer.  */
    for (i = 0;
        (i < reception_size) &&
        (cdc_acm_host_read_buffer_length < sizeof(cdc_acm_host_read_buffer));
        i ++, cdc_acm_host_read_buffer_length ++)
    {
        cdc_acm_host_read_buffer[cdc_acm_host_read_buffer_length] = reception_buffer[i];
    }

    return;
}


static void test_cdc_acm_device_read_length_set(ULONG new_length)
{
    if (device_read_length == new_length)
        return;
    tx_thread_sleep(1);
    if (new_length < 64)
        device_read_length = 64;
    else
    {
        /* Align with 64.  */
        device_read_length = (new_length & 63u) ? ((new_length & ~63u) + 64) : new_length;
    }
#if !defined(UX_DEVICE_STANDALONE)
    /* Cancel the pending read to apply new length.  */
    ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
            UX_SLAVE_CLASS_CDC_ACM_IOCTL_ABORT_PIPE,
            (VOID*)UX_SLAVE_CLASS_CDC_ACM_ENDPOINT_RCV);
#else
    /* Wait a while to let device background task do action.  */
    tx_thread_sleep(2);
#endif
}


static void test_cdc_acm_device_read_write_blocking(void)
{
UINT        status;
ULONG       actual_length;
UCHAR       test_chr;
ULONG       test_length;
ULONG       i, test;
#undef N_TEST
#define N_TEST 5
struct {
    UCHAR chr;
    ULONG len;
} tests[N_TEST] = {
    {'$', 1},
    {'A', 64},
    {'N', 65},
    {'3', UX_SLAVE_REQUEST_DATA_MAX_LENGTH - 64},
    {'G', UX_SLAVE_REQUEST_DATA_MAX_LENGTH},
};

    for (test = 0; test < N_TEST; test ++)
    {
        test_chr    = tests[test].chr;
        test_length = tests[test].len;
        test_cdc_acm_device_read_length_set(test_length);

        for (i = 0; i < test_length; i ++)
        {
            host_buffer[i] = test_chr;
        }

        /* Blocking write.  */
        status = ux_host_class_cdc_acm_write(cdc_acm_host_data, host_buffer,
                                             test_length, &actual_length);
        UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "test_length %ld\n", test_length);
        if (((test_length & 63) == 0) && actual_length != device_read_length)
        {
            status = ux_host_class_cdc_acm_write(cdc_acm_host_data, host_buffer,
                                                 0, &actual_length);
            UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "test_length %ld\n", test_length);
        }

        /* Blocking read.  */
        _ux_utility_memory_set(host_buffer, ~test_chr, test_length);
        status = ux_host_class_cdc_acm_read(cdc_acm_host_data, host_buffer,
                                            test_length, &actual_length);
        UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "test_length %ld\n", test_length);
        if (actual_length != test_length)
        {
            printf("ERROR #%d, length not match %ld <> %ld\n", __LINE__, test_length, actual_length);
            test_control_return(1);
        }
        for (i = 0; i < test_length; i ++)
        {
            UX_TEST_ASSERT_MESSAGE(host_buffer[i] == test_chr, "test_length %ld\n", test_length);
        }
    }
}


#if defined(UX_HOST_STANDALONE)

static ULONG test_cdc_acm_host_write_callback_count = 0;
static UINT  test_cdc_acm_host_write_callback_status;
static ULONG test_cdc_acm_host_write_callback_actual_length;
static VOID test_cdc_acm_host_write_callback(UX_HOST_CLASS_CDC_ACM *cdc_acm,
                                             UINT status, ULONG actual_length)
{
    test_cdc_acm_host_write_callback_count ++;
    test_cdc_acm_host_write_callback_status = status;
    test_cdc_acm_host_write_callback_actual_length = actual_length;
}

/* Uses _write_with_callback.  */

static UINT test_cdc_acm_host_write(UX_HOST_CLASS_CDC_ACM *cdc_acm,
                                    UCHAR *data_pointer, 
                                    ULONG requested_length,
                                    ULONG *actual_length)
{
UINT  status;
ULONG i;

    status = ux_host_class_cdc_acm_ioctl(cdc_acm,
                                UX_HOST_CLASS_CDC_ACM_IOCTL_WRITE_CALLBACK,
                                (VOID*)test_cdc_acm_host_write_callback);
    if (status != UX_SUCCESS)
        return(status);

    test_cdc_acm_host_write_callback_count = 0;
    status = ux_host_class_cdc_acm_write_with_callback(cdc_acm, data_pointer, requested_length);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d : write exec error 0x%x\n", __LINE__, status);
        return(status);
    }
    test_host_delay(requested_length/64 + 1);
    if (test_cdc_acm_host_write_callback_count == 0)
    {
        printf("ERROR #%d : write timeout\n", __LINE__);
        return(UX_TRANSFER_TIMEOUT);
    }
    status = ux_host_class_cdc_acm_ioctl(cdc_acm,
                                UX_HOST_CLASS_CDC_ACM_IOCTL_GET_WRITE_STATUS,
                                (VOID*)actual_length);
    if (status != test_cdc_acm_host_write_callback_status ||
        *actual_length != test_cdc_acm_host_write_callback_actual_length)
    {
        printf("ERROR #%d : write status conflict\n", __LINE__);
        return(UX_ERROR);
    }
    return(UX_SUCCESS);
}
#else
#define test_cdc_acm_host_write                 ux_host_class_cdc_acm_write
#endif


static void test_cdc_acm_device_read_write(void)
{
UINT        status;
ULONG       actual_length;
UCHAR       test_chr;
ULONG       test_length;
ULONG       i, test;
#undef N_TEST
#define N_TEST 6
struct {
    UCHAR chr;
    ULONG len;
} tests[N_TEST] = {
    {'$', 1},
    {'A', 64},
    {'N', 65},
    {'3', UX_SLAVE_REQUEST_DATA_MAX_LENGTH - 64},
    {'G', UX_SLAVE_REQUEST_DATA_MAX_LENGTH},
    {'H', UX_SLAVE_REQUEST_DATA_MAX_LENGTH + 1},
};

    /* Read packet by packet on device side.  */
    test_cdc_acm_device_read_length_set(64);

    /* Reception parameter */
    cdc_acm_host_reception.ux_host_class_cdc_acm_reception_block_size = UX_DEMO_RECEPTION_BLOCK_SIZE;
    cdc_acm_host_reception.ux_host_class_cdc_acm_reception_data_buffer = cdc_acm_host_reception_buffer;
    cdc_acm_host_reception.ux_host_class_cdc_acm_reception_data_buffer_size = UX_DEMO_RECEPTION_BUFFER_SIZE;
    cdc_acm_host_reception.ux_host_class_cdc_acm_reception_callback = test_thread_host_reception_callback;

    /* Start reception.  */
    status = ux_host_class_cdc_acm_reception_start(cdc_acm_host_data, &cdc_acm_host_reception);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: reception start fail %x\n", __LINE__, status);
        test_control_return(1);
    }

    for (test = 0; test < N_TEST; test ++)
    {
        test_chr    = tests[test].chr;
        test_length = tests[test].len;

        for (i = 0; i < test_length; i ++)
        {
            host_buffer[i] = test_chr;
            cdc_acm_host_read_buffer[i] = ~test_chr;
        }
        cdc_acm_host_read_buffer_length = 0;

        status = test_cdc_acm_host_write(cdc_acm_host_data, host_buffer,
                                                test_length, &actual_length);
        UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "test_length %ld\n", test_length);
        if (((test_length & 63) == 0) && actual_length != device_read_length)
        {
            status = test_cdc_acm_host_write(cdc_acm_host_data, host_buffer,
                                                0, &actual_length);
            UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "test_length %ld\n", test_length);
        }

        /* Wait a while for background reception.  */
        test_host_delay(test_length/64 + 1);

        UX_TEST_ASSERT_MESSAGE(cdc_acm_host_reception_status == UX_SUCCESS, "test_length %ld\n", test_length);
        if (cdc_acm_host_read_buffer_length != test_length)
        {
            printf("ERROR #%d, length not match %ld <> %ld\n", __LINE__,
                test_length, cdc_acm_host_read_buffer_length);
            test_control_return(1);
        }
        for (i = 0; i < test_length; i ++)
        {
            UX_TEST_ASSERT_MESSAGE(cdc_acm_host_read_buffer[i] == test_chr, "test_length %ld\n", test_length);
        }
    }

    /* Stop reception.  */
    ux_host_class_cdc_acm_reception_stop(cdc_acm_host_data, &cdc_acm_host_reception);
}

void  ux_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;

    stepinfo("\n");

    /* Find the cdc_acm class and wait for the link to be up.  */
    status =  demo_class_cdc_acm_get();
    if (status != UX_SUCCESS)
    {

        /* CDC ACM basic test error.  */
        printf("ERROR #10\n");
        test_control_return(1);
    }

    test_cdc_acm_device_ioctl_parameters();
    test_cdc_acm_device_read_write_blocking();
    test_cdc_acm_device_read_write();

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

UINT    status;
ULONG   actual_length;
ULONG   read_length = device_read_length;
ULONG   write_length, write_zlp = UX_FALSE;
#define CDC_ACM_DEVICE_STATE_READ     UX_STATE_STEP
#define CDC_ACM_DEVICE_STATE_WRITE    UX_STATE_STEP + 1
#define CDC_ACM_DEVICE_STATE_ZLP      UX_STATE_STEP + 2
UINT    cdc_acm_device_state = UX_STATE_RESET;

    while(1)
    {

#if defined(UX_DEVICE_STANDALONE)

        /* Keep running device stack tasks.  */
        ux_system_tasks_run();

        /* Reset state if read length changed.  */
        if (read_length != device_read_length)
        {
            cdc_acm_device_state = UX_STATE_RESET;
            read_length = device_read_length;
        }

        /* CDC ACM echo state machine.  */
        switch(cdc_acm_device_state)
        {
        case UX_STATE_RESET:
            if (cdc_acm_slave == UX_NULL)
                break;

            {
                ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                        UX_SLAVE_CLASS_CDC_ACM_IOCTL_ABORT_PIPE,
                        (VOID*)UX_SLAVE_CLASS_CDC_ACM_ENDPOINT_XMIT);
                ux_device_class_cdc_acm_ioctl(cdc_acm_slave,
                        UX_SLAVE_CLASS_CDC_ACM_IOCTL_ABORT_PIPE,
                        (VOID*)UX_SLAVE_CLASS_CDC_ACM_ENDPOINT_RCV);
                cdc_acm_device_state = CDC_ACM_DEVICE_STATE_READ;
            }
            /* Fall through.  */
        case CDC_ACM_DEVICE_STATE_READ:
            if (cdc_acm_slave == UX_NULL)
            {
                cdc_acm_device_state = UX_STATE_RESET;
                break;
            }
            status = ux_device_class_cdc_acm_read_run(cdc_acm_slave,
                            device_buffer, device_read_length, &actual_length);
            if (status < UX_STATE_NEXT)
            {
                printf("ERROR #%d: read status 0x%x\n", __LINE__, status);
                return;
            }
            if (status == UX_STATE_NEXT)
            {
                write_length = actual_length;
                if ((actual_length < device_read_length) &&
                    ((actual_length & 63) == 0))
                {
                    write_zlp = UX_TRUE;
                }
                cdc_acm_device_state = CDC_ACM_DEVICE_STATE_WRITE;
            }
            break;
        case CDC_ACM_DEVICE_STATE_WRITE:
            if (cdc_acm_slave == UX_NULL)
            {
                cdc_acm_device_state = UX_STATE_RESET;
                break;
            }
            status = ux_device_class_cdc_acm_write_run(cdc_acm_slave,
                            device_buffer, write_length, &actual_length);
            if (status < UX_STATE_NEXT)
            {
                printf("ERROR #%d: write status 0x%x\n", __LINE__, status);
                return;
            }
            if (status == UX_STATE_NEXT)
            {
                if (write_zlp && ((write_length % 64) == 0))
                {
                    cdc_acm_device_state = CDC_ACM_DEVICE_STATE_ZLP;
                    break;
                }
                cdc_acm_device_state = CDC_ACM_DEVICE_STATE_READ;
                break;
            }
            break;
        case CDC_ACM_DEVICE_STATE_ZLP:
            if (cdc_acm_slave == UX_NULL)
            {
                cdc_acm_device_state = UX_STATE_RESET;
                break;
            }
            status = ux_device_class_cdc_acm_write_run(cdc_acm_slave,
                            device_buffer, 0, &actual_length);
            if (status < UX_STATE_NEXT)
            {
                printf("ERROR #%d: ZLP status 0x%x\n", __LINE__, status);
                return;
            }
            if (status == UX_STATE_NEXT)
                cdc_acm_device_state = CDC_ACM_DEVICE_STATE_READ;
            break;
        default:
            cdc_acm_device_state = UX_STATE_RESET;
        }

        /* Let other threads run.  */
        tx_thread_relinquish();
#else

    if (cdc_acm_slave == UX_NULL)
    {
        tx_thread_sleep(1);
        continue;
    }
    /* Force reading packet by packet.  */
    status = ux_device_class_cdc_acm_read(cdc_acm_slave, device_buffer,
                                        device_read_length, &actual_length);
    if (status == UX_SUCCESS)
    {
        write_length = actual_length;
        status = ux_device_class_cdc_acm_write(cdc_acm_slave, device_buffer,
                                            write_length, &actual_length);
    }
#endif
    }
}
