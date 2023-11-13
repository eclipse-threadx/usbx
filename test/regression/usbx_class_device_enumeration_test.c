/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_host_class_dummy.h"
#include "ux_device_class_dummy.h"
#include "ux_test.h"


/* Define USBX demo constants.  */

#define UX_DEMO_STACK_SIZE      4096
#define UX_DEMO_BUFFER_SIZE     2048
#define UX_DEMO_RUN             1
#define UX_DEMO_MEMORY_SIZE     (64*1024)


/* Define the counters used in the demo application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;


/* Define USBX demo global variables.  */

static UX_HOST_CLASS                   *class_driver;
static UX_HOST_CLASS_DUMMY             *dummy;
static UX_DEVICE_CLASS_DUMMY           *dummy_slave;

static UINT                             expected_error;

#define _W0(w)      ( (w)       & 0xFF)
#define _W1(w)      (((w) >> 8) & 0xFF)

#define _DEVICE_DESCRIPTOR(cls, sub, protocol, pktsize, vid, pid, n_cfg)        \
    0x12, 0x01, 0x10, 0x01,                                                     \
    (cls), (sub), (protocol), (pktsize),                                        \
    _W0(vid), _W1(vid), _W0(pid), _W1(pid),                                     \
    0x00, 0x00, 0x00, 0x00, 0x00, (n_cfg),

#define _QUALIFIER_DESCRIPTOR(cls, sub, protocol, n_cfg)                        \
    0x0a, 0x06, 0x00, 0x02,                                                     \
    (cls), (sub), (protocol), 0x40, (n_cfg), 0x00,

#define _CONFIGURATION_DESCRIPTOR(total_len, n_ifc, cfg_val)                    \
    0x09, 0x02, _W0(total_len), _W1(total_len), (n_ifc), (cfg_val),             \
    0x00, 0xc0, 0x32,

#define _INTERFACE_DESCRIPTOR(ifc_n, alt, n_ep, cls, sub, protocol)             \
    0x09, 0x04, (ifc_n), (alt), (n_ep), (cls), (sub), (protocol), 0x00,

#define _ENDPOINT_DESCRIPTOR(addr, attr, pktsize, interval)                     \
    0x07, 0x05, (addr), (attr), _W0(pktsize), _W1(pktsize), (interval),

#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)
static UCHAR device_framework_full_speed[] = {
    _DEVICE_DESCRIPTOR(0x99, 0x99, 0x99, 8, 0x08EC, 0x0001, 1)
    _CONFIGURATION_DESCRIPTOR(32, 1, 1)
    _INTERFACE_DESCRIPTOR(0, 0, 2, 0x99, 0x99, 0x99)
    _ENDPOINT_DESCRIPTOR(0x01, 0x02, 64, 0x00)
    _ENDPOINT_DESCRIPTOR(0x82, 0x02, 64, 0x00)
};


#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)
static UCHAR device_framework_high_speed[] = {
    _DEVICE_DESCRIPTOR(0x99, 0x99, 0x99, 64, 0x08EC, 0x0001, 1)
    _QUALIFIER_DESCRIPTOR(0, 0, 0, 1)
    _CONFIGURATION_DESCRIPTOR(32, 1, 1)
    _INTERFACE_DESCRIPTOR(0, 0, 2, 0x99, 0x99, 0x99)
    _ENDPOINT_DESCRIPTOR(0x01, 0x02, 512, 0x00)
    _ENDPOINT_DESCRIPTOR(0x82, 0x02, 512, 0x00)
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
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(language_id_framework)
static UCHAR language_id_framework[] = {

    /* English. */
    0x09, 0x04
};


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static VOID                tx_demo_instance_activate(VOID  *dummy_instance);
static VOID                tx_demo_instance_deactivate(VOID *dummy_instance);

#if defined(UX_HOST_STANDALONE)
static UINT                tx_demo_host_change_function(ULONG e, UX_HOST_CLASS *c, VOID *p);
#else
#define                     tx_demo_host_change_function UX_NULL
#endif

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
    if (expected_error == 0 || error_code != expected_error)
    {
        /* Failed test.  */
        printf("Error on line %d, system_level: %d, system_context: %d, error code: %x\n", __LINE__, system_level, system_context, error_code);
        // test_control_return(1);
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_class_device_enumeration_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;
UX_DEVICE_CLASS_DUMMY_PARAMETER  parameter;


    printf("Running Basic Device Class Enumeration Test......................... ");

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX.  */
    status =  ux_host_stack_initialize(tx_demo_host_change_function);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the host class drivers for this USBX implementation.  */
    status =  ux_host_stack_class_register(_ux_host_class_dummy_name, _ux_host_class_dummy_device_entry);

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
    _ux_utility_memory_set((void *)&parameter, 0x00, sizeof(parameter));
    parameter.ux_device_class_dummy_parameter_callbacks.ux_device_class_dummy_instance_activate   =  tx_demo_instance_activate;
    parameter.ux_device_class_dummy_parameter_callbacks.ux_device_class_dummy_instance_deactivate =  tx_demo_instance_deactivate;

    /* Initialize the device dpump class. The class is connected with interface 0 */
    status =  ux_device_stack_class_register(_ux_device_class_dummy_name, _ux_device_class_dummy_entry,
                                              1, 0, &parameter);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main demo thread.  */
    status =  tx_thread_create(&tx_demo_thread_slave_simulation, "tx demo slave simulation", tx_demo_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT status;
ULONG           actual_length;
UCHAR           current_char;
UX_HOST_CLASS   *cls;
UINT            i;

    stepinfo(">>>>> Dummy Class Get\n");
    status = ux_host_stack_class_get(_ux_host_class_dummy_name, &cls);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>> Dummy Class Instance Wait\n");
    do
    {
        status =  ux_host_stack_class_instance_get(cls, 0, (VOID **) &dummy);
#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
#endif
        tx_thread_relinquish();
    } while (status != UX_SUCCESS);

    stepinfo(">>>>> Dummy Class State Wait\n");
    while(dummy -> ux_host_class_dummy_state != UX_HOST_CLASS_INSTANCE_LIVE)
    {
#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
#endif
        tx_thread_relinquish();
    }

    expected_error = 0;

    /* Sleep for a tick to make sure everything is complete.  */
    tx_thread_sleep(1);

    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* DPUMP error.  */
        printf("ERROR #%d: total %ld errors\n", __LINE__, error_counter);
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

UINT    status;
ULONG   actual_length;


    while(1)
    {
#if defined(UX_DEVICE_STANDALONE)

        /* Run device tasks.  */
        ux_system_tasks_run();
#endif
        /* Increment thread counter.  */
        thread_1_counter++;

        /* Relinquish to other thread.  */
        tx_thread_relinquish();
    }
}

static VOID  tx_demo_instance_activate(VOID *inst)
{
    dummy_slave = (UX_DEVICE_CLASS_DUMMY *)inst;
}

static VOID  tx_demo_instance_deactivate(VOID *inst)
{
    dummy_slave = UX_NULL;
}

#if defined(UX_HOST_STANDALONE)
static UINT  tx_demo_host_change_function(ULONG e, UX_HOST_CLASS *c, VOID *p)
{
    if (e == UX_STANDALONE_WAIT_BACKGROUND_TASK)
    {
        tx_thread_relinquish();
    }
}
#endif
