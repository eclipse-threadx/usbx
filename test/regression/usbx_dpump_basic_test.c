/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_host_class_dpump.h"
#include "ux_device_class_dpump.h"


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

static unsigned char                   host_out_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];
static unsigned char                   host_in_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];
static unsigned char                   slave_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];

static UX_HOST_CLASS                   *class_driver;
static UX_HOST_CLASS_DPUMP             *dpump;
static UX_SLAVE_CLASS_DPUMP            *dpump_slave;

static UINT                             expected_error;

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

#ifdef UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT
    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00
#else
    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00
#endif
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

#ifdef UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT
    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00
#else
    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x82, 0x02, 0x00, 0x02, 0x00
#endif
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


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static VOID                tx_demo_instance_activate(VOID  *dpump_instance);
static VOID                tx_demo_instance_deactivate(VOID *dpump_instance);

#if defined(UX_HOST_STANDALONE)
static UINT                tx_demo_host_change_function(ULONG e, UX_HOST_CLASS *c, VOID *p);
#else
#define                     tx_demo_host_change_function UX_NULL
#endif

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
void    usbx_dpump_basic_test_application_define(void *first_unused_memory)
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

        printf("Running DPUMP Basic Functionality Test.............................. ERROR #1\n");
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX.  */
    status =  ux_host_stack_initialize(tx_demo_host_change_function);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running DPUMP Basic Functionality Test.............................. ERROR #2\n");
        test_control_return(1);
    }

    /* Register all the host class drivers for this USBX implementation.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_dpump_name, ux_host_class_dpump_entry);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running DPUMP Basic Functionality Test.............................. ERROR #3\n");
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

        printf("Running DPUMP Basic Functionality Test.............................. ERROR #5\n");
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

        printf("Running DPUMP Basic Functionality Test.............................. ERROR #6\n");
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running DPUMP Basic Functionality Test.............................. ERROR #7\n");
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running DPUMP Basic Functionality Test.............................. ERROR #4\n");
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Running DPUMP Basic Functionality Test.............................. ERROR #8\n");
        test_control_return(1);
    }

    /* Create the main demo thread.  */
    status =  tx_thread_create(&tx_demo_thread_slave_simulation, "tx demo slave simulation", tx_demo_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Running DPUMP Basic Functionality Test.............................. ERROR #9\n");
        test_control_return(1);
    }
}


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT status;
ULONG           actual_length;
UCHAR           current_char;
UX_HOST_CLASS   *class;
UINT            i;
#if 0
UX_ENDPOINT     *endpoint;
#endif


    /* Inform user.  */
    printf("Running DPUMP Basic Functionality Test.............................. ");

    /* Find the class container with unregistered name.  */
    status =  ux_host_stack_class_get(_ux_system_host_class_hid_name, &class);

    /* Should return error.  */
    if (status != UX_HOST_CLASS_UNKNOWN)
    {

        /* DPUMP basic test error.  */
        printf("ERROR #10\n");
        test_control_return(1);
    }

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

#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
#endif
        tx_thread_relinquish();

    } while (status != UX_SUCCESS);

    /* We still need to wait for the data pump status to be live.  */
    while (dpump -> ux_host_class_dpump_state != UX_HOST_CLASS_INSTANCE_LIVE)
    {

#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
#endif
        tx_thread_relinquish();

    }

    /* At this point, the data pump class has been found.  Now use the
       data pump to send and receive data between the host and device.  */

    /* We start with a 'A' in buffer.  */
    current_char = 'A';

    /* Perform this test sequence 100 times.  */
    for (i = 0; i < 100; i++)
    {

        /* Increment thread counter.  */
        thread_0_counter++;

        /* Initialize the write buffer. */
        _ux_utility_memory_set(host_out_buffer, current_char, UX_HOST_CLASS_DPUMP_PACKET_SIZE);

        /* Increment the character in buffer.  */
        current_char++;

        /* Check for upper alphabet limit.  */
        if (current_char > 'Z')
            current_char =  'A';

        /* Write to the host Data Pump Bulk out endpoint.  */
        status =  _ux_host_class_dpump_write (dpump, host_out_buffer, UX_HOST_CLASS_DPUMP_PACKET_SIZE, &actual_length);

        /* Check for error.  */
        if (status != UX_SUCCESS)
        {

            /* DPUMP basic test error.  */
            printf("ERROR #%d: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        /* Verify that the status and the amount of data is correct.  */
        if ((status != UX_SUCCESS) || actual_length != UX_HOST_CLASS_DPUMP_PACKET_SIZE)
        {

            /* DPUMP basic test error.  */
            printf("ERROR #12\n");
            test_control_return(1);
        }

#if defined(UX_HOST_STANDALONE)
        /* Relinquish to other thread.  */
        tx_thread_relinquish();
#endif

        /* Read to the Data Pump Bulk out endpoint.  */
        status =  _ux_host_class_dpump_read (dpump, host_in_buffer, UX_HOST_CLASS_DPUMP_PACKET_SIZE, &actual_length);

        /* Verify that the status and the amount of data is correct.  */
        if ((status != UX_SUCCESS) || actual_length != UX_HOST_CLASS_DPUMP_PACKET_SIZE)
        {

            /* DPUMP basic test error.  */
            printf("ERROR #13\n");
            test_control_return(1);
        }

        /* Relinquish to other thread.  */
        tx_thread_relinquish();
    }

#if 0
    /* Test ux_host_stack_endpoint_reset with invalid endpoint number.  */
    endpoint = dpump -> ux_host_class_dpump_interface -> ux_interface_first_endpoint;
    endpoint -> ux_endpoint_descriptor.bEndpointAddress = 0xf;
    expected_error = UX_TRANSFER_STALLED;
    status = _ux_host_stack_endpoint_reset(endpoint);

    /* Check for error.  */
    if (status == UX_SUCCESS)
    {

        /* DPUMP basic test error.  */
        printf("ERROR #14\n");
        test_control_return(1);
    }
#endif

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
#if defined(UX_DEVICE_STANDALONE)
#define DPUMP_DEVICE_STATE_READ     UX_STATE_STEP
#define DPUMP_DEVICE_STATE_WRITE    UX_STATE_STEP + 1
UINT    dpump_device_state = UX_STATE_RESET;
#endif


    while(1)
    {
#if defined(UX_DEVICE_STANDALONE)

        /* Run device tasks.  */
        ux_system_tasks_run();

        /* DPUMP echo state machine.  */
        switch(dpump_device_state)
        {
        case UX_STATE_RESET:
            if (dpump_slave != UX_NULL)

                /* Start reading.  */
                dpump_device_state = DPUMP_DEVICE_STATE_READ;
            break;

        case DPUMP_DEVICE_STATE_READ:

            /* Read from the device data pump.  */
            if (dpump_slave == UX_NULL)
            {
                dpump_device_state = UX_STATE_RESET;
                break;
            }
            status = ux_device_class_dpump_read_run(dpump_slave, slave_buffer, UX_HOST_CLASS_DPUMP_PACKET_SIZE, &actual_length);

            if (status < UX_STATE_NEXT)
            {
                printf("ERROR #%d: read status 0x%x\n", __LINE__, status);
                error_counter ++;
                return;
            }

            if (status == UX_STATE_NEXT)
            {
                if (actual_length != UX_HOST_CLASS_DPUMP_PACKET_SIZE)
                {
                    printf("ERROR #%d: read length %ld\n", __LINE__, actual_length);
                    error_counter ++;
                    return;
                }

                dpump_device_state = DPUMP_DEVICE_STATE_WRITE;
            }
            break;

        case DPUMP_DEVICE_STATE_WRITE:

            /* Now write to the device data pump.  */
            if (dpump_slave == UX_NULL)
            {
                dpump_device_state = UX_STATE_RESET;
                break;
            }
            status = ux_device_class_dpump_write_run(dpump_slave, slave_buffer, UX_HOST_CLASS_DPUMP_PACKET_SIZE, &actual_length);

            if (status < UX_STATE_NEXT)
            {
                printf("ERROR #%d: write status 0x%x\n", __LINE__, status);
                error_counter ++;
                return;
            }

            if (status == UX_STATE_NEXT)
                dpump_device_state = DPUMP_DEVICE_STATE_READ;
            break;
        
        default:
            dpump_device_state = UX_STATE_RESET;
        }

        /* Increment thread counter.  */
        thread_1_counter++;

        /* Relinquish to other thread.  */
        tx_thread_relinquish();

#else
        /* Ensure the dpump class on the device is still alive.  */
        while (dpump_slave != UX_NULL)
        {

            /* Increment thread counter.  */
            thread_1_counter++;

            /* Read from the device data pump.  */
            status =  _ux_device_class_dpump_read(dpump_slave, slave_buffer, UX_HOST_CLASS_DPUMP_PACKET_SIZE, &actual_length);

            /* Verify that the status and the amount of data is correct.  */
            if ((status != UX_SUCCESS) || actual_length != UX_HOST_CLASS_DPUMP_PACKET_SIZE)
            {
                printf("ERROR #%d.%ld: read status 0x%x, length %ld\n", __LINE__, thread_1_counter, status, actual_length);

                /* Increment error counter.  */
                error_counter++;

                /* Return from thread.  */
                return;
            }

            /* Now write to the device data pump.  */
            status =  _ux_device_class_dpump_write(dpump_slave, slave_buffer, UX_HOST_CLASS_DPUMP_PACKET_SIZE, &actual_length);

            /* Verify that the status and the amount of data is correct.  */
            if ((status != UX_SUCCESS) || actual_length != UX_HOST_CLASS_DPUMP_PACKET_SIZE)
            {
                printf("ERROR #%d.%ld: write status 0x%x, length %ld\n", __LINE__, thread_1_counter, status, actual_length);

                /* Increment error counter.  */
                error_counter++;

                /* Return from thread.  */
                return;
            }
        }

        /* Relinquish to other thread.  */
        tx_thread_relinquish();
#endif
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

#if defined(UX_HOST_STANDALONE)
static UINT  tx_demo_host_change_function(ULONG e, UX_HOST_CLASS *c, VOID *p)
{
    if (e == UX_STANDALONE_WAIT_BACKGROUND_TASK)
    {
        tx_thread_relinquish();
    }
}
#endif
