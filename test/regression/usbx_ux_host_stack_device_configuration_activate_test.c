/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>

#include "tx_api.h"

#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_class_dpump.h"
#include "ux_device_class_dpump.h"

#include "ux_test.h"


/* Define USBX demo constants.  */

#define UX_DEMO_STACK_SIZE      4096
#define UX_DEMO_BUFFER_SIZE     2048
#define UX_DEMO_RUN             1
#define UX_DEMO_MEMORY_SIZE     (64*1024)


/* Define the counters used in the demo application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static int                             error_counter = 0;


/* Define USBX demo global variables.  */

static unsigned char                   host_out_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];
static unsigned char                   host_in_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];
static unsigned char                   slave_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];

static UX_HOST_CLASS                   *class_driver;
static UX_HOST_CLASS_DPUMP             *dpump;
static UX_SLAVE_CLASS_DPUMP            *dpump_slave;

static UINT                            ignore_error = UX_FALSE;

#define BYTE0(x)                        (((x)     ) & 0xFF)
#define BYTE1(x)                        (((x) >> 8) & 0xFF)

#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)
static UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
    0xec, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,
    0x02, /* bNumConfigurations  */

    /* Configuration descriptor */
    0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xc0,
    0x32,

    /* Interface descriptor */
    0x09, 0x04, 0x00, 0x00, 0x02, 0x99, 0x99, 0x99,
    0x00,

    /* Endpoint descriptor (Bulk Out) */
    0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk In) */
    0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00,

    /* Configuration descriptor */
    0x09, 0x02,
    0x09, 0x00, /* wTotalLength  */
    0x00,       /* bNumInterfaces  */
    0x02,       /* bConfigurationValue  */
    0x00, 0xc0, 0x32,
};

#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)
static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
    0x03,
    0x02, /* bNumConfigurations  */

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x01, 0x00,

    /* Configuration descriptor */
    0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xc0,
    0x32,

    /* Interface descriptor */
    0x09, 0x04, 0x00, 0x00, 0x02, 0x99, 0x99, 0x99,
    0x00,

    /* Endpoint descriptor (Bulk Out) */
    0x07, 0x05, 0x01, 0x02, 0x00, 0x02, 0x00,

    /* Endpoint descriptor (Bulk In) */
    0x07, 0x05, 0x82, 0x02, 0x00, 0x02, 0x00,

    /* Configuration descriptor */
    0x09, 0x02,
    0x09, 0x00, /* wTotalLength  */
    0x00,       /* bNumInterfaces  */
    0x02,       /* bConfigurationValue  */
    0x00, 0xc0, 0x32,
};

/* String Device Framework :
     Byte 0 and 1 : Word containing the language ID : 0x0904 for US
     Byte 2       : Byte containing the index of the descriptor
     Byte 3       : Byte containing the length of the descriptor string
    */

#define STRING_FRAMEWORK_LENGTH sizeof(string_framework)
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

/* Multiple languages are supported on the device, to add
       a language besides English, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
#define LANGUAGE_ID_FRAMEWORK_LENGTH 2
static UCHAR language_id_framework[] = {

    /* English. */
        0x09, 0x04
    };


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static VOID                tx_demo_instance_activate(VOID  *dpump_instance);
static VOID                tx_demo_instance_deactivate(VOID *dpump_instance);

UINT                       _ux_host_class_dpump_entry(UX_HOST_CLASS_COMMAND *command);
UINT                       ux_hcd_sim_initialize(UX_HCD *hcd);

static TX_THREAD           tx_demo_thread_host_simulation;
static TX_THREAD           tx_demo_thread_slave_simulation;
static void                tx_demo_thread_host_simulation_entry(ULONG);
static void                tx_demo_thread_slave_simulation_entry(ULONG);


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
    if (!ignore_error)
    {
        /* Failed test.  */
        printf("Error #%d, system_level: %d, system_context: %d, code: %x(%d)\n",
			__LINE__, system_level, system_context, error_code, error_code);
        test_control_return(1);
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_stack_device_configuration_activate_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;
UX_SLAVE_CLASS_DPUMP_PARAMETER  parameter;


    printf("Running ux_host_stack_device_configuration_(de)activate Test........ ");
    stepinfo("\n");

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX.  */
    status =  ux_host_stack_initialize(UX_NULL);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register all the host class drivers for this USBX implementation.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_dpump_name, ux_host_class_dpump_entry);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
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
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Set the parameters for callback when insertion/extraction of a Data Pump device.  */
    parameter.ux_slave_class_dpump_instance_activate   =  tx_demo_instance_activate;
    parameter.ux_slave_class_dpump_instance_deactivate =  tx_demo_instance_deactivate;

    /* Initialize the device dpump class. The class is connected with interface 0 */
     status =  ux_device_stack_class_register(_ux_system_slave_class_dpump_name, _ux_device_class_dpump_entry,
                                               1, 0, &parameter);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main demo thread.  */
    status =  tx_thread_create(&tx_demo_thread_slave_simulation, "tx demo slave simulation", tx_demo_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}

static UINT  tx_demo_dpump_get(void)
{
UINT                status;
UX_HOST_CLASS       *class_inst;
    status =  ux_host_stack_class_get(_ux_system_host_class_dpump_name, &class_inst);
    if (status != UX_SUCCESS)
        return(UX_ERROR);
    status =  ux_host_stack_class_instance_get(class_inst, 0, (VOID **) &dpump);
    if (status != UX_SUCCESS)
        return(UX_ERROR);
    if (dpump -> ux_host_class_dpump_state != UX_HOST_CLASS_INSTANCE_LIVE)
        return(UX_ERROR);
    return(UX_SUCCESS);
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                status;
INT                 i;
UX_DEVICE           *device;
UX_CONFIGURATION    *configuration;
ULONG               temp;

    /* Wait until dpump is ready.  */
    for (i = 0; i < 100; i ++)
    {
        if (tx_demo_dpump_get() == UX_SUCCESS && dpump_slave != UX_NULL)
            break;
        tx_thread_sleep(UX_MS_TO_TICK_NON_ZERO(10));
    }
    if (dpump == UX_NULL || dpump_slave == UX_NULL)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    device = dpump -> ux_host_class_dpump_device;
    configuration = device -> ux_device_first_configuration;

    /* >>>>> Test configuration deactivate.  */

    ignore_error = UX_TRUE;

    /* >>>>> ux_host_stack_device_configuration_deactivate: invalid device.  */
    temp = device -> ux_device_handle;
    device -> ux_device_handle = 0;
    status = ux_host_stack_device_configuration_deactivate(device);
    if (status != UX_DEVICE_HANDLE_UNKNOWN)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    device -> ux_device_handle = temp;

    /* >>>>> ux_host_stack_device_configuration_deactivate: invalid device semaphore.  */
    _ux_utility_semaphore_delete(&device -> ux_device_protection_semaphore);
    status = ux_host_stack_device_configuration_deactivate(device);
    if (status != UX_SEMAPHORE_ERROR)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_utility_semaphore_create(&device -> ux_device_protection_semaphore, "ux_device_protection_semaphore", 1);

    ignore_error = UX_FALSE;

    /* >>>>> ux_host_stack_device_configuration_deactivate: OK.  */
    status = ux_host_stack_device_configuration_deactivate(device);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    UX_TEST_ASSERT(dpump_slave == UX_NULL);

    /* >>>>> ux_host_stack_device_configuration_deactivate: Already done, OK.  */
    status = ux_host_stack_device_configuration_deactivate(device);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    ignore_error = UX_TRUE;

    /* >>>>> ux_host_stack_device_configuration_activate: invalid configuration handle.  */
    temp = configuration -> ux_configuration_handle;
    configuration -> ux_configuration_handle = 0;
    status = ux_host_stack_device_configuration_activate(configuration);
    if (status != UX_CONFIGURATION_HANDLE_UNKNOWN)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    configuration -> ux_configuration_handle = temp;

    /* >>>>> ux_host_stack_device_configuration_activate: invalid device semaphore.  */
    _ux_utility_semaphore_delete(&device -> ux_device_protection_semaphore);
    status = ux_host_stack_device_configuration_activate(configuration);
    if (status != UX_SEMAPHORE_ERROR)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    _ux_utility_semaphore_create(&device -> ux_device_protection_semaphore, "ux_device_protection_semaphore", 1);

    ignore_error = UX_FALSE;

    /* >>>>> ux_host_stack_device_configuration_activate: OK.  */
    status = ux_host_stack_device_configuration_activate(configuration);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    UX_TEST_ASSERT(dpump_slave != UX_NULL);

    /* >>>>> ux_host_stack_device_configuration_activate: Already done, OK.  */
    status = ux_host_stack_device_configuration_activate(configuration);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* >>>>> ux_host_stack_device_configuration_activate: Activate another, FAIL.  */
    status = ux_host_stack_device_configuration_activate(configuration -> ux_configuration_next_configuration);
    if (status != UX_ALREADY_ACTIVATED)
    {
        printf("ERROR #%d:0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Sleep for a tick to make sure everything is complete.  */
    tx_thread_sleep(1);

    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* DPUMP error.  */
        printf("ERROR %d\n", error_counter);
        test_control_return(1);
    }
    else
    {

        /* Successful test.  */
        printf("SUCCESS!\n");
        test_control_return(0);
    }
}


static void  tx_demo_thread_slave_simulation_entry(ULONG arg)
{
}

static VOID  tx_demo_instance_activate(VOID *dpump_instance)
{

    /* Save the DPUMP instance.  */
    dpump_slave = (UX_SLAVE_CLASS_DPUMP *) dpump_instance;
}

static VOID  tx_demo_instance_deactivate(VOID *dpump_instance)
{

    /* Reset the DPUMP instance.  */
    dpump_slave = UX_NULL;
}

