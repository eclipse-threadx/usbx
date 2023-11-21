/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_class_dpump.h"
#include "ux_device_class_dpump.h"

#include "ux_host_stack.h"

/* Define USBX demo constants.  */

#define UX_DEMO_STACK_SIZE      4096
#define UX_DEMO_BUFFER_SIZE     2048
#define UX_DEMO_RUN             1
#define UX_DEMO_MEMORY_SIZE     (64*1024)


/* Define the counters used in the demo application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           test_sim_entry;
static ULONG                           test_sim_query_usage;
static ULONG                           test_sim_query_count;
static ULONG                           test_sim_activate_count;

/* Define USBX demo global variables.  */

static unsigned char                   host_out_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];
static unsigned char                   host_in_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];
static unsigned char                   slave_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];

static UX_HOST_CLASS                   *class_driver;
static UX_HOST_CLASS_DPUMP             *dpump;
static UX_SLAVE_CLASS_DPUMP            *dpump_slave;


static UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x10, 0x01,
    0x00, 0x00, 0x00, /* bDeviceClass, bDeviceSubClass, bDeviceProtocol */
    0x08, /* bMaxPacketSize0 */
    0xec, 0x08, 0x10, 0x00, /* idVendor, idProduct */
    0x00, 0x00,
    0x00, 0x00, 0x00,
    0x01, /* bNumConfigurations */

    /* Configuration descriptor */
    0x09, 0x02,
    0x20, 0x00, /* wTotalLength */
    0x01, 0x01, /* bNumInterfaces, bConfigurationValue */
    0x00,
    0xc0, 0x32, /* bmAttributes, bMaxPower */

    /* Interface descriptor */
    0x09, 0x04,
    0x00, 0x00, 0x02, /* bInterfaceNumber, bAlternateSetting, bNumEndpoints */
    0x99, 0x99, 0x99, /* bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */
    0x00,

    /* Endpoint descriptor (Bulk Out) */
    0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk In) */
    0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00
};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)


static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x00, 0x02,
    0x00, 0x00, 0x00, /* bDeviceClass, bDeviceSubClass, bDeviceProtocol */
    0x40, /* bMaxPacketSize0 */
    0x0a, 0x07, /* idVendor */
    0x25, 0x40, /* idProduct */
    0x01, 0x00,
    0x01, 0x02, 0x03,
    0x01, /* bNumConfigurations */

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02,
    0x00, 0x00, 0x00, /* bDeviceClass, bDeviceSubClass, bDeviceProtocol */
    0x40, /* bMaxPacketSize0 */
    0x01, /* bNumConfigurations */
    0x00,

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
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)

/* String Device Framework :
    Byte 0 and 1 : Word containing the language ID : 0x0904 for US
    Byte 2       : Byte containing the index of the descriptor
    Byte 3       : Byte containing the length of the descriptor string
*/
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
#define STRING_FRAMEWORK_LENGTH sizeof(string_framework)

/* Multiple languages are supported on the device, to add
    a language besides English, the unicode language code must
    be appended to the language_id_framework array and the length
    adjusted accordingly. */
static UCHAR language_id_framework[] = {

    /* English. */
    0x09, 0x04
};
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(language_id_framework)


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static VOID                tx_demo_instance_activate(VOID  *dpump_instance);
static VOID                tx_demo_instance_deactivate(VOID *dpump_instance);

UINT                       _ux_host_class_dpump_entry(UX_HOST_CLASS_COMMAND *command);
UINT                       ux_hcd_sim_initialize(UX_HCD *hcd);
UINT                       _ux_host_class_dpump_write(UX_HOST_CLASS_DPUMP *dpump, UCHAR * data_pointer,
                                    ULONG requested_length, ULONG *actual_length);
UINT                       _ux_host_class_dpump_read (UX_HOST_CLASS_DPUMP *dpump, UCHAR *data_pointer,
                                    ULONG requested_length, ULONG *actual_length);

static TX_THREAD           tx_demo_thread_host_simulation;
static TX_THREAD           tx_demo_thread_slave_simulation;
static void                tx_demo_thread_host_simulation_entry(ULONG);
static void                tx_demo_thread_slave_simulation_entry(ULONG);

static UINT test_ux_host_class_dpump_entry(UX_HOST_CLASS_COMMAND *command)
{
    if (test_sim_entry)
    {

        switch (command -> ux_host_class_command_request)
        {
        case UX_HOST_CLASS_COMMAND_QUERY:
            test_sim_query_count ++;
            if (command -> ux_host_class_command_usage != test_sim_query_usage)
                return UX_NO_CLASS_MATCH;
            break;

        case UX_HOST_CLASS_COMMAND_ACTIVATE:
            test_sim_activate_count ++;
            break;

        default:
            break;
        }
        return UX_SUCCESS;
    }
    return ux_host_class_dpump_entry(command);
}


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

    /* Failed test.  */
    printf("Error on line %d, system_level: %d, system_context: %d, error code: %d\n", __LINE__, system_level, system_context, error_code);
    test_control_return(1);
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_stack_class_device_scan_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;
UX_SLAVE_CLASS_DPUMP_PARAMETER  parameter;


    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_class_device_scan Test........................ ERROR #1\n");
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX.  */
    status =  ux_host_stack_initialize(UX_NULL);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_class_device_scan Test........................ ERROR #2\n");
        test_control_return(1);
    }

    /* Register all the host class drivers for this USBX implementation.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_dpump_name, test_ux_host_class_dpump_entry);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_class_device_scan Test........................ ERROR #3\n");
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

        printf("Running ux_host_stack_class_device_scan Test........................ ERROR #5\n");
        test_control_return(1);
    }

    /* Set the parameters for callback when insertion/extraction of a Data Pump device.  */
    parameter.ux_slave_class_dpump_instance_activate   =  tx_demo_instance_activate;
    parameter.ux_slave_class_dpump_instance_deactivate =  tx_demo_instance_deactivate;

    /* Initialize the device dpump class. The class is connected with interface 0 */
    status  = ux_device_stack_class_register(_ux_system_slave_class_dpump_name, _ux_device_class_dpump_entry,
                                              1, 0, &parameter);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_class_device_scan Test........................ ERROR #6\n");
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_class_device_scan Test........................ ERROR #7\n");
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_class_device_scan Test........................ ERROR #4\n");
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Running ux_host_stack_class_device_scan Test........................ ERROR #8\n");
        test_control_return(1);
    }

    /* Create the main demo thread.  */
    status =  tx_thread_create(&tx_demo_thread_slave_simulation, "tx demo slave simulation", tx_demo_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Running ux_host_stack_class_device_scan Test........................ ERROR #9\n");
        test_control_return(1);
    }
}


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT status;
UX_HOST_CLASS   *class;
UX_DEVICE       *device;


    /* Inform user.  */
    printf("Running ux_host_stack_class_device_scan Test........................ ");

    /* Find the main data pump container.  */
    status =  ux_host_stack_class_get(_ux_system_host_class_dpump_name, &class);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        /* DPUMP basic test error.  */
        printf("ERROR #10\n");
        test_control_return(1);
    }

    /* We get the first instance of the data pump device.  */
    do
    {

        status =  ux_host_stack_class_instance_get(class, 0, (VOID **) &dpump);
        tx_thread_relinquish();
    } while (status != UX_SUCCESS);

    /* We still need to wait for the data pump status to be live.  */
    while (dpump -> ux_host_class_dpump_state != UX_HOST_CLASS_INSTANCE_LIVE)
    {

        tx_thread_relinquish();
    }

    status = ux_host_stack_device_get(0, &device);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: fail to get device instance\n", __LINE__);
        test_control_return(1);
    }

#if !defined(UX_HOST_STACK_DEVICE_DRIVER_SCAN_DISABLE)
    /* Simulate class device scan, query and activate should happen. */
    test_sim_entry = UX_TRUE;

    /* Query fail case */
    test_sim_query_usage = 0xFF; /* UX_HOST_CLASS_COMMAND_USAGE_PIDVID, UX_HOST_CLASS_COMMAND_USAGE_CSP */
    test_sim_activate_count = 0;
    test_sim_query_count = 0;
    status = _ux_host_stack_class_device_scan(device);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: Queries should fail\n", __LINE__);
        error_counter ++;
    }
    if (test_sim_query_count != 2)
    {

        printf("ERROR #%d: Queries tried must be 2\n", __LINE__);
        error_counter ++;
    }
    if (test_sim_activate_count > 0)
    {

        printf("ERROR #%d: No activate should be done\n", __LINE__);
        error_counter ++;
    }

#if !defined(UX_HOST_STACK_DEVICE_DRIVER_SCAN_VIDPID_DISABLE)
    /* VIDPID success case */
    test_sim_query_usage = UX_HOST_CLASS_COMMAND_USAGE_PIDVID; /* UX_HOST_CLASS_COMMAND_USAGE_PIDVID, UX_HOST_CLASS_COMMAND_USAGE_DCSP */
    test_sim_activate_count = 0;
    test_sim_query_count = 0;
    status = _ux_host_stack_class_device_scan(device);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: Queries should pass\n", __LINE__);
        error_counter ++;
    }
    if (test_sim_query_count == 0)
    {

        printf("ERROR #%d: There should be at least 1 query\n", __LINE__);
        error_counter ++;
    }
    if (test_sim_activate_count != 1)
    {

        printf("ERROR #%d: There must be 1 activate\n", __LINE__);
        error_counter ++;
    }
#endif /* !defined(UX_HOST_STACK_DEVICE_DRIVER_SCAN_DCSP_DISABLE) */

#if !defined(UX_HOST_STACK_DEVICE_DRIVER_SCAN_DCSP_DISABLE)
    /* Class/SubClass/Protocol success case */
    test_sim_query_usage = UX_HOST_CLASS_COMMAND_USAGE_DCSP; /* UX_HOST_CLASS_COMMAND_USAGE_PIDVID, UX_HOST_CLASS_COMMAND_USAGE_DCSP */
    test_sim_activate_count = 0;
    test_sim_query_count = 0;
    status = _ux_host_stack_class_device_scan(device);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: Queries should pass\n", __LINE__);
        error_counter ++;
    }
    if (test_sim_query_count == 0)
    {

        printf("ERROR #%d: There should be at least 1 query\n", __LINE__);
        error_counter ++;
    }
    if (test_sim_activate_count != 1)
    {

        printf("ERROR #%d: There must be 1 activate\n", __LINE__);
        error_counter ++;
    }
#endif /* !defined(UX_HOST_STACK_DEVICE_DRIVER_SCAN_DCSP_DISABLE) */

#endif /* !defined(UX_HOST_STACK_DEVICE_DRIVER_SCAN_DISABLE) */

    /* Sleep for a tick to make sure everything is complete.  */
    tx_thread_sleep(1);

    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* DPUMP error.  */
        printf("ERROR #14\n");
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

    while(1)
    {

        /* Ensure the dpump class on the device is still alive.  */
        if (dpump_slave != UX_NULL)
        {

            /* Increment thread counter.  */
            thread_1_counter++;
        }

        /* Let other thread run.  */
        tx_thread_sleep(10);
    }
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

