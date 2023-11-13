/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "fx_api.h"

#include "ux_device_class_printer.h"
#include "ux_device_stack.h"
#include "ux_host_class_printer.h"

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
static VOID                                test_thread_entry(ULONG);
static TX_THREAD                           tx_test_thread_host_simulation;
static TX_THREAD                           tx_test_thread_slave_simulation;
static VOID                                tx_test_thread_host_simulation_entry(ULONG);
static VOID                                tx_test_thread_slave_simulation_entry(ULONG);

static TX_THREAD                           tx_test_thread_printer_read;
static TX_THREAD                           tx_test_thread_printer_write;
static TX_SEMAPHORE                        tx_test_semaphore_printer_trigger;
static VOID                                tx_test_printer_read_entry(ULONG);
static VOID                                tx_test_printer_write_entry(ULONG);

static VOID                                ux_test_hcd_entry_should_not_be_called(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_disconnect(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);

/* Define global data structures.  */
static UCHAR                        usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 4)];

static UX_DEVICE                    *device = UX_NULL;
static UX_HOST_CLASS_PRINTER        *host_printer = UX_NULL;
static UCHAR                        host_buffer[UX_DEMO_BUFFER_SIZE * 8];

static ULONG                        enum_counter;

static ULONG                        error_counter;
static ULONG                        error_callback_counter;

static ULONG                        set_cfg_counter;

static ULONG                        rsc_mem_alloc_cnt_on_set_cfg;
static ULONG                        rsc_mem_free_on_set_cfg;
static ULONG                        rsc_sem_on_set_cfg;
static ULONG                        rsc_sem_get_on_set_cfg;
static ULONG                        rsc_mutex_on_set_cfg;

static ULONG                        rsc_enum_sem_usage;
static ULONG                        rsc_enum_sem_get_count;
static ULONG                        rsc_enum_mutex_usage;
static ULONG                        rsc_enum_mem_usage;
static ULONG                        rsc_enum_mem_alloc_count;

static ULONG                        rsc_test_sem_usage;
static ULONG                        rsc_test_sem_get_count;
static ULONG                        rsc_test_mutex_usage;
static ULONG                        rsc_test_mem_alloc_count;

static UX_DEVICE_CLASS_PRINTER              *device_printer = UX_NULL;
static UX_DEVICE_CLASS_PRINTER_PARAMETER    device_printer_parameter;
static UCHAR                                device_buffer[UX_DEMO_BUFFER_SIZE * 8];
static ULONG                                device_buffer_length = 0;
UCHAR _ux_device_class_printer_name[] = "_ux_device_class_printer";

/* Device printer device ID.  */
static UCHAR printer_device_id[] =
 {
    "  "                                //   Length (changed later)
    "MFG:Generic;"                      //   manufacturer (case sensitive)
    "MDL:Generic_/_Text_Only;"          //   model (case sensitive)
    "CMD:1284.4;"                       //   PDL command set
    "CLS:PRINTER;"                      //   class
    "DES:Generic text only printer;"    //   description
 };

/* Define device framework.  */

#define _W0(w)      ( (w)       & 0xFF)
#define _W1(w)      (((w) >> 8) & 0xFF)

#define _CONFIGURATION_DESCRIPTOR(total_len, n_ifc, cfg_val)                    \
    0x09, 0x02, _W0(total_len), _W1(total_len), (n_ifc), (cfg_val),             \
    0x00, 0xc0, 0x32,

#define _INTERFACE_DESCRIPTOR(ifc_n, alt, n_ep, cls, sub, protocol)             \
    0x09, 0x04, (ifc_n), (alt), (n_ep), (cls), (sub), (protocol), 0x00,

#define _ENDPOINT_DESCRIPTOR(addr, attr, pktsize, interval)                     \
    0x07, 0x05, (addr), (attr), _W0(pktsize), _W1(pktsize), (interval),

#define _CFG_TOTAL_LEN (9+9+7+7)

#define             STRING_FRAMEWORK_LENGTH                 47
#define             LANGUAGE_ID_FRAMEWORK_LENGTH            2

static unsigned char device_framework_full_speed[] = {

    /* Device descriptor     18 bytes
       0xEF bDeviceClass:    Composite class code
       0x02 bDeviceSubclass: class sub code
       0x00 bDeviceProtocol: Device protocol
       idVendor & idProduct - http://www.linux-usb.org/usb.ids
    */
    0x12, 0x01, 0x10, 0x01,
    0x00, 0x00, 0x00,
    0x08,
    0x84, 0x84, 0x00, 0x00,
    0x00, 0x01,
    0x01, 0x02, 0x03,
    0x01,

    _CONFIGURATION_DESCRIPTOR(_CFG_TOTAL_LEN, 1, 1)
    _INTERFACE_DESCRIPTOR(0, 0, 2, 0x07, 0x01, 0x02)
    _ENDPOINT_DESCRIPTOR(0x01, 0x02, 64, 0x00)
    _ENDPOINT_DESCRIPTOR(0x82, 0x02, 64, 0x00)
};

#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      sizeof(device_framework_full_speed)
#define             DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED      sizeof(device_framework_full_speed)
#define             device_framework_high_speed             device_framework_full_speed

static unsigned char string_framework[] = {

    /* Manufacturer string descriptor : Index 1 - "AzureRTOS" */
    0x09, 0x04, 0x01, 9,
        'A','z','u','r','e','R','T','O','S',

    /* Product string descriptor : Index 2 - "Printer device" */
    0x09, 0x04, 0x02, 14,
        'P','r','i','n','t','e','r',' ','d','e','v','i','c','e',

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
static UX_TEST_SETUP _PrinterGetDeviceID = {0xA1,0x00,0x0000,0x0000};

/* Test interactions */

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


static UINT _test_simulate_disconnect = 0;
static VOID ux_test_on_printer_get_device_id(UX_TEST_ACTION *action, VOID *params)
{
/* Test is printer instance allocated.  */
UX_DEVICE               *dev = &_ux_system_host->ux_system_host_device_array[0];
UX_INTERFACE            *ifc = dev->ux_device_first_configuration->ux_configuration_first_interface;
UX_HOST_CLASS_PRINTER   *printer;
    UX_TEST_ASSERT(ifc != UX_NULL);
    printer = (UX_HOST_CLASS_PRINTER *)ifc->ux_interface_class_instance;
    UX_TEST_ASSERT(printer != UX_NULL);
    UX_TEST_ASSERT(printer->ux_host_class_printer_allocated != UX_NULL);
    /* Memory allocated, disconnect device to see if memory is double freed.  */
    _test_simulate_disconnect = 1;
}
static UX_TEST_HCD_SIM_ACTION call_on_PrinterGetDeviceID[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_PrinterGetDeviceID,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_SUCCESS, ux_test_on_printer_get_device_id,
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

static UINT test_slave_change_function(ULONG change)
{
    return 0;
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_PRINTER *printer_inst = (UX_HOST_CLASS_PRINTER *) inst;

    switch(event)
    {

    case UX_DEVICE_INSERTION:
        host_printer = printer_inst;
        break;

    case UX_DEVICE_REMOVAL:
        if (host_printer == printer_inst)
            host_printer = UX_NULL;
        break;

    case UX_DEVICE_CONNECTION:
        device = (UX_DEVICE *)inst;
        break;

    case UX_DEVICE_DISCONNECTION:
        if ((VOID *)device == inst)
            device = UX_NULL;
        break;

#if defined(UX_HOST_STANDALONE)
    case UX_STANDALONE_WAIT_BACKGROUND_TASK:
        tx_thread_relinquish();
        if(_test_simulate_disconnect)
        {
            _test_simulate_disconnect = 0;
            ux_test_dcd_sim_slave_disconnect();
            ux_test_hcd_sim_host_disconnect();
        }
        break;
#endif

    default:
        break;
    }
    return 0;
}

static VOID    test_printer_instance_activate(VOID *dummy_instance)
{
    if (device_printer == UX_NULL)
        device_printer = (UX_DEVICE_CLASS_PRINTER *)dummy_instance;
}
static VOID    test_printer_instance_deactivate(VOID *dummy_instance)
{
    if ((VOID*)device_printer == dummy_instance)
        device_printer = UX_NULL;
}

static VOID test_ux_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    printf("ERROR #%d: 0x%x, 0x%x, 0x%x\n", __LINE__, system_level, system_context, error_code);
    UX_TEST_ASSERT(!(
         (system_context == UX_SYSTEM_CONTEXT_UTILITY) &&
         (error_code == UX_MEMORY_CORRUPTED))); /* fail on UX_MEMORY_CORRUPTED */
    error_callback_counter ++;
}

static VOID ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params)
{

    set_cfg_counter ++;

    rsc_mem_free_on_set_cfg = ux_test_regular_memory_free();

    rsc_mem_alloc_cnt_on_set_cfg = ux_test_utility_sim_mem_alloc_count();

    rsc_sem_on_set_cfg = ux_test_utility_sim_sem_create_count();
    rsc_enum_sem_get_count = ux_test_utility_sim_sem_get_count();
    rsc_mutex_on_set_cfg = ux_test_utility_sim_mutex_create_count();
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_msrc_81572_standalone_host_printer_allocated_enum_free_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;


    printf("Running MSRC 81572 - Standalone Host Printer Alloc Enum Free Test... ");
#if !defined(UX_HOST_STANDALONE)
    printf("Skip\n");
    test_control_return(0);
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
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 4);

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);
    UX_TEST_CHECK_SUCCESS(status);

    /* Register the error callback. */
    _ux_utility_error_callback_register(test_ux_error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(test_host_change_function);
    UX_TEST_CHECK_SUCCESS(status);

    /* Register PRINTER class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_printer_name, ux_host_class_printer_entry);
    UX_TEST_CHECK_SUCCESS(status);

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,
                                       test_slave_change_function);
    UX_TEST_CHECK_SUCCESS(status);

    /* Set the parameters for callback when insertion/extraction of a printer device.  */
    _ux_utility_memory_set(&device_printer_parameter, 0, sizeof(device_printer_parameter));
    device_printer_parameter.ux_device_class_printer_device_id = printer_device_id;
    ux_utility_short_put_big_endian(printer_device_id, sizeof(printer_device_id));
    device_printer_parameter.ux_device_class_printer_instance_activate   = test_printer_instance_activate;
    device_printer_parameter.ux_device_class_printer_instance_deactivate = test_printer_instance_deactivate;
    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status  = ux_device_stack_class_register(_ux_device_class_printer_name,
                                             _ux_device_class_printer_entry,
                                             1, 0, &device_printer_parameter);
    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();
    UX_TEST_CHECK_SUCCESS(status);

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    UX_TEST_CHECK_SUCCESS(status);

    /* Setup a hook for GET_DEVICE_ID request, to test disconnect (memory allocated by class at that time)  */
    ux_test_hcd_sim_host_set_actions(call_on_PrinterGetDeviceID);

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_test_thread_host_simulation, "tx test host simulation", tx_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    UX_TEST_ASSERT(status == TX_SUCCESS);

    /* Create the main slave simulation  thread.  */
    stack_pointer += UX_DEMO_STACK_SIZE;
    status =  tx_thread_create(&tx_test_thread_slave_simulation, "tx test slave simulation", tx_test_thread_slave_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    UX_TEST_ASSERT(status == TX_SUCCESS);
}

static UINT _test_check_host_connection_error(VOID)
{
    if (device_printer && host_printer)
        return(UX_SUCCESS);
    if (error_callback_counter >= 3)
        return(UX_SUCCESS);
    return(UX_ERROR);
}

static UINT _test_check_host_connection_success(VOID)
{
    if (device_printer && host_printer)
        return(UX_SUCCESS);
    return(UX_ERROR);
}

static UINT _test_check_host_disconnection_success(VOID)
{
    if (device_printer == UX_NULL && host_printer == UX_NULL)
        return(UX_SUCCESS);
    return(UX_ERROR);
}

void  tx_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
ULONG                                               test_n;
ULONG                                               mem_free;
ULONG                                               loop;
ULONG                                               parameter_u32[64/4];
USHORT                                              *parameter_u16 = (USHORT*)parameter_u32;
UCHAR                                               *parameter_u8 = (UCHAR*)parameter_u32;


    stepinfo("\n");
    stepinfo(">>>>>>>>>>>>>>>> Test connect\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    status = ux_test_sleep_break_on_error(100, _test_check_host_connection_success);
    UX_TEST_ASSERT(status == UX_SUCCESS);

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    if (host_printer != UX_NULL)
    {

        printf("ERROR #13: instance not removed when disconnect");
        test_control_return(1);
    }

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status  = ux_device_stack_class_unregister(_ux_device_class_printer_name, _ux_device_class_printer_entry);

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
#if defined(UX_DEVICE_STANDALONE)
        ux_system_tasks_run();
        tx_thread_relinquish();
#else
        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
#endif
    }
}
