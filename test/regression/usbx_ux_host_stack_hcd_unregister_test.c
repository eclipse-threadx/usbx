/* This test is designed to test the ux_utility_memory_....  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_device_class_dpump.h"
#include "ux_host_class_dpump.h"

#include "ux_host_stack.h"
#include "ux_hcd_sim_host.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"


/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_MEMORY_SIZE     (64*1024)

#define     LSB(x) ( (x) & 0x00ff)
#define     MSB(x) (((x) & 0xff00) >> 8)

/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           error_callback_ignore = UX_FALSE;
static ULONG                           error_callback_counter;

static UCHAR                           bad_name[UX_MAX_HCD_NAME_LENGTH + 1];


/* Define USBX test global variables.  */


#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED 50
static UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
    0xec, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01,

    /* Configuration descriptor */
    0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xc0,
    0x32,

    /* Interface descriptor */
    0x09, 0x04, 0x00, 0x00, 0x02, 0x99, 0x99, 0x99,
    0x00,

    /* Endpoint descriptor (Bulk Out) */
    0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk In) */
    0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00
};


#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED 60
static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
    0x03, 0x01,

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
    0x07, 0x05, 0x82, 0x02, 0x00, 0x02, 0x00
};

/* String Device Framework :
    Byte 0 and 1 : Word containing the language ID : 0x0904 for US
    Byte 2       : Byte containing the index of the descriptor
    Byte 3       : Byte containing the length of the descriptor string
*/
#define STRING_FRAMEWORK_LENGTH 38
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

/* Actions/callbacks hook.  */

static UX_TEST_SETUP _SetCfgDescriptor = UX_TEST_SETUP_SetConfigure;

static VOID ux_test_set_cfg_descriptor_invoked(UX_TEST_ACTION *action, VOID *params)
{
    /* Unregister HCD.  */
    _ux_host_stack_hcd_unregister(_ux_system_host_hcd_simulator_name, 0, 0);;
}

static UX_TEST_HCD_SIM_ACTION hcd_unregister_while_enum[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_SetCfgDescriptor,
        UX_FALSE, 0,
        UX_TEST_SETUP_MATCH_REQ_V_I, 0, UX_NULL, 0, UX_SUCCESS,
        UX_SUCCESS, ux_test_set_cfg_descriptor_invoked,
        .no_return = UX_TRUE,
        .do_after = UX_FALSE},
{   0   }
};


/* Define prototypes for external Controller's (HCD/DCDs), classes and clients.  */

static TX_THREAD            ux_test_thread_simulation_0;
static void                 ux_test_thread_simulation_0_entry(ULONG);

static TX_THREAD            ux_test_thread_simulation_1;
static void                 ux_test_thread_simulation_1_entry(ULONG);

static UX_SLAVE_CLASS_DPUMP *dpump_device = UX_NULL;
static VOID                 ux_test_dpump_instance_activate(VOID *dpump_instance);
static VOID                 ux_test_dpump_instance_deactivate(VOID *dpump_instance);

/* Prototype for test control return.  */

void  test_control_return(UINT status);

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        {
            /* Failed test.  */
            printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
        }
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_stack_hcd_unregister_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
UCHAR                           *stack_pointer;
UCHAR                           *memory_pointer;
UX_SLAVE_CLASS_DPUMP_PARAMETER  parameter;


    /* Inform user.  */
    printf("Running ux_host_stack_hcd_unregister Test........................... ");

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the device portion of USBX */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH, UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Set the parameters for callback when insertion/extraction of a Data Pump device.  */
    parameter.ux_slave_class_dpump_instance_activate   =  ux_test_dpump_instance_activate;
    parameter.ux_slave_class_dpump_instance_deactivate =  ux_test_dpump_instance_deactivate;

    /* Initialize the device dpump class. The class is connected with interface 0 */
     status =  ux_device_stack_class_register(_ux_system_slave_class_dpump_name, _ux_device_class_dpump_entry,
                                               1, 0, &parameter);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_simulation_0, "test simulation", ux_test_thread_simulation_0_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}

#define UX_TEST_CONN_PASS  0
#define UX_TEST_CONN_WAIT  1
#define UX_TEST_CONN_CHECK 2
static void ux_test_hcd_register(char *s, int line, int connection_wait)
{
UINT        status;
INT         i;
UX_DEVICE   *device;
char        *nothing = "";
    if (s == UX_NULL)
        s = nothing;
    status = _ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%s%d.%d: 0x%x\n", s, line, __LINE__, status);
        test_control_return(1);
    }
    if (!connection_wait)
        return;
    /* Wait for connection.  */
    for (i = 0; i < 100; i ++)
    {
        tx_thread_sleep(UX_MS_TO_TICK_NON_ZERO(10));
        status = _ux_host_stack_device_get(0, &device);
        if (status == UX_SUCCESS && dpump_device != UX_NULL)
            break;
    }
    if (connection_wait > UX_TEST_CONN_WAIT)
    {
        if (dpump_device == UX_NULL)
        {
            printf("ERROR #%s%d.%d\n", s, line, __LINE__);
            test_control_return(1);
        }
        if (status != UX_SUCCESS || device == UX_NULL)
        {
            printf("ERROR #%s%d.%d\n", s, line, __LINE__);
            test_control_return(1);
        }
    }
}

static UINT entry_function(UX_HCD* hcd, UINT function, VOID* parameter)
{
    return(UX_SUCCESS);


}

static void  ux_test_thread_simulation_0_entry(ULONG arg)
{

UINT                status;
ULONG               rfree;
INT                 i, try;
char                hdr[64];
UX_DEVICE           *device;


    /* Initialize host stack.  */
    status = ux_host_stack_initialize(UX_NULL);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register all the host class drivers for this USBX implementation.  */
    status = ux_host_stack_class_register(_ux_system_host_class_dpump_name, ux_host_class_dpump_entry);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /************************** Register & unregister (no device connected).  */

    /* Log memory level to check.  */
    rfree = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    /* Provent HCD & ENUM thread to run.  */
    _ux_utility_thread_suspend(&_ux_system_host -> ux_system_host_hcd_thread);
    _ux_utility_thread_suspend(&_ux_system_host -> ux_system_host_enum_thread);

    /* Register.  */
    ux_test_hcd_register(UX_NULL, __LINE__, UX_TEST_CONN_PASS);

    /* Unregister.  */
    status = _ux_host_stack_hcd_unregister(_ux_system_host_hcd_simulator_name, 0, 0);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    if (rfree != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Clear semaphore by init.  */
    _ux_utility_semaphore_get(&_ux_system_host -> ux_system_host_enum_semaphore, 0);

    /* Resume threads.  */
    _ux_utility_thread_resume(&_ux_system_host -> ux_system_host_hcd_thread);
    _ux_utility_thread_resume(&_ux_system_host -> ux_system_host_enum_thread);

    /************************** Register & unregister (device connected).  */

    /* Register HCD.  */
    ux_test_hcd_register(UX_NULL, __LINE__, UX_TEST_CONN_CHECK);

    /* Error ignore, there should be UX_CONTROLLER_UNKNOWN.  */
    error_callback_ignore = UX_TRUE;

    /* Unregister HCD.  */
    status = _ux_host_stack_hcd_unregister(_ux_system_host_hcd_simulator_name, 0, 0);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Get host device.  */
    status = _ux_host_stack_device_get(0, &device);
    if (status != UX_DEVICE_HANDLE_UNKNOWN)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /************************** Unregister HCD not found.  */

    /* Unregister HCD.  */
    status = _ux_host_stack_hcd_unregister(_ux_system_host_hcd_ehci_name, 0, 0);
    if (status == UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /************************** Unregister HCD bad name.  */

    /* Bad string (error callback reported).  */
    _ux_utility_memory_set(bad_name, '0', UX_MAX_HCD_NAME_LENGTH + 1);

    /* Unregister HCD.  */
    status = _ux_host_stack_hcd_unregister(bad_name, 0, 0);
    if (status == UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /************************** Unregister HCD other cases not tested.  */
    /* E.g., HCD name match but parameters not, HCD parameters match but name not. */
    /* E.g., All connected devices are not rooted from the HCD to unregister.  */

    /* No error ignore.  */
    error_callback_ignore = UX_FALSE;

    /************************** Register & unregister several times (not corrupted).  */

    // ux_test_dcd_sim_slave_disconnect();
    // ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);

    for (try = 0; try < 4; try ++)
    {
        // printf("#%d.%d\n", try, __LINE__);
        sprintf(hdr, "%i", try);

        /* Log memory level to check.  */
        rfree = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

        /* Register HCD.  */
        ux_test_hcd_register(hdr, __LINE__, UX_TEST_CONN_CHECK);

        error_callback_ignore = UX_TRUE;

        if(try < 3)
            /* Unregister HCD.  */
            status = _ux_host_stack_hcd_unregister(_ux_system_host_hcd_simulator_name, 0, 0);
        else
        {
            /* Test line 127 */
            _ux_host_stack_hcd_unregister(_ux_system_host_hcd_simulator_name, 1, 0);

            /* Test line 128 */
            _ux_host_stack_hcd_unregister(_ux_system_host_hcd_simulator_name, 0, 1);

            /* Test line 138 */
            _ux_host_stack_hcd_unregister("noname", 0, 0);

            /* ALl previous 3 calls would fail. Call the unregister again to properly
               unregister this HCD. */
            status = _ux_host_stack_hcd_unregister(_ux_system_host_hcd_simulator_name, 0, 0);

        }

        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d.%d: 0x%x\n", try, __LINE__, status);
            test_control_return(1);
        }


        /* Get host device.  */
        status = _ux_host_stack_device_get(0, &device);
        if (status != UX_DEVICE_HANDLE_UNKNOWN)
        {
            printf("ERROR #%d.%d: 0x%x\n", try, __LINE__, status);
            test_control_return(1);
        }

        /* Check memory level.  */
        if (rfree != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {
            printf("ERROR #%d.%d\n", try, __LINE__);
            test_control_return(1);
        }

        error_callback_ignore = UX_FALSE;

        // ux_test_dcd_sim_slave_disconnect();
        // ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);

    } /* for (try = 0; try < 3; try ++) */

    /************************** Register & unregister when enumeration in progress.  */

    /* Log memory level to check.  */
    rfree = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    /* Capture GetDeviceDescriptor and unregister HCD at that time. */
    ux_test_hcd_sim_host_set_actions(hcd_unregister_while_enum);

    /* Register HCD.  */
    error_callback_ignore = UX_TRUE;
    ux_test_hcd_register(UX_NULL, __LINE__, UX_TEST_CONN_WAIT);

    /* Get host device.  */
    status = _ux_host_stack_device_get(0, &device);
    if (status != UX_DEVICE_HANDLE_UNKNOWN)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    error_callback_ignore = UX_FALSE;

    /* Check memory level.  */
    if (rfree != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

#if UX_MAX_HCD > 1 && UX_MAX_DEVICES > 1

    /************************** Unregister all devices on HCD RH.  */

    /* Skip first slot, to use later device slots.  */
    _ux_system_host->ux_system_host_device_array[0].ux_device_handle = UX_USED;

    /* Register several HCD.  */
    ux_test_hcd_register("", __LINE__, UX_TEST_CONN_CHECK);
    status = _ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,1,0);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (_ux_system_host->ux_system_host_device_array[1].ux_device_handle == UX_UNUSED)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Mark slot free.  */
    _ux_system_host->ux_system_host_device_array[0].ux_device_handle = UX_UNUSED;

    /* Unregister HCD, device on slot 1 is removed.  */
    error_callback_ignore = UX_TRUE;
    status = _ux_host_stack_hcd_unregister(_ux_system_host_hcd_simulator_name, 0, 0);
    error_callback_ignore = UX_FALSE;
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Device should be removed.  */
    if (_ux_system_host->ux_system_host_device_array[1].ux_device_handle != UX_UNUSED)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
#endif

    {
        UX_HCD* hcd;
        UX_DEVICE* device;

        hcd = _ux_system_host->ux_system_host_hcd_array;
        hcd->ux_hcd_status = UX_USED;
        hcd->ux_hcd_io = 0;
        hcd->ux_hcd_irq = 0;
        hcd->ux_hcd_entry_function = entry_function;
        strcpy(hcd->ux_hcd_name, _ux_system_host_hcd_simulator_name);
        device = _ux_system_host->ux_system_host_device_array;

#if UX_MAX_DEVICES>1
        device->ux_device_hcd = hcd;
#endif
#if UX_MAX_DEVICES>1
        if (_ux_system_host->ux_system_host_max_devices == 0)
        {
            printf("ERROR #%d: ux_system_host_max_devices= %ld errors\n", __LINE__, _ux_system_host->ux_system_host_max_devices);
            test_control_return(1);

        }
        device->ux_device_parent = device;

#endif
        device->ux_device_handle = 1;

        status = _ux_host_stack_hcd_unregister(_ux_system_host_hcd_simulator_name, 0, 0);
        if(status)
            error_counter++;
    }
    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* Test error.  */
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


static VOID  ux_test_dpump_instance_activate(VOID *dpump_instance)
{

    /* Save the DPUMP instance.  */
    dpump_device = (UX_SLAVE_CLASS_DPUMP *) dpump_instance;
}

static VOID  ux_test_dpump_instance_deactivate(VOID *dpump_instance)
{

    /* Reset the DPUMP instance.  */
    dpump_device = UX_NULL;
}
