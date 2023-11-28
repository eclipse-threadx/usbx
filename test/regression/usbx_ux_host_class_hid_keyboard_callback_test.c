/* This tests focuses on code coverage of ux_host_class_hid_keyboard_callback.   */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_host_class_hid.h"
#include "ux_host_class_hid_keyboard.h"
#include "ux_device_class_hid.h"
#include "ux_device_stack.h"
#include "usbx_test_common_hid.h"

/* Define constants.  */
#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024
#define                             UX_DEMO_BUFFER_SIZE 2048
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE 512
#define                             UX_DEMO_XMIT_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE 64

/* Define local/extern function prototypes.  */
static void                         demo_thread_entry(ULONG);
static UINT                         demo_thread_hid_callback(UX_SLAVE_CLASS_HID *, UX_SLAVE_CLASS_HID_EVENT *);
static TX_THREAD                    tx_demo_thread_host_simulation;
static TX_THREAD                    tx_demo_thread_slave_simulation;
static void                         tx_demo_thread_host_simulation_entry(ULONG);
static void                         tx_demo_thread_slave_simulation_entry(ULONG);


/* Define global data structures.  */
static ULONG                               error_counter;
static TX_THREAD                           demo_thread;
static UX_HOST_CLASS                       *class_driver;
static ULONG                               class_driver_index;
static UX_HOST_CLASS_HID                   *hid;
static UX_HOST_CLASS_HID_CLIENT            *hid_client;
static UX_HOST_CLASS_HID_KEYBOARD          *keyboard;
static UINT                                status;
static UINT                                transfer_completed;
static ULONG                               requested_length;
static TX_SEMAPHORE                        demo_semaphore;

static ULONG                               keyboard_char;
static ULONG                               keyboard_state;
static UCHAR                               keyboard_queue[1024];
static ULONG                               keyboard_queue_index;

static UX_SLAVE_CLASS_HID_PARAMETER        hid_parameter;

static UCHAR hid_keyboard_report[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x09, 0x39,                    //   USAGE (Caps Lock)
    0x09, 0x53,                    //   USAGE (Num Lock)
    0x09, 0x47,                    //   USAGE (Scroll Lock)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x05,                    //   REPORT_SIZE (5)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};
#define HID_KEYBOARD_REPORT_LENGTH (sizeof(hid_keyboard_report)/sizeof(hid_keyboard_report)[0])


#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED 52
static UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
        0x81, 0x0A, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,

    /* Configuration descriptor */
        0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x02, 0x00, 0x01, 0x03, 0x00, 0x00,
        0x00,

    /* HID descriptor */
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_KEYBOARD_REPORT_LENGTH),
        MSB(HID_KEYBOARD_REPORT_LENGTH),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };


#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED 62
static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Device qualifier descriptor */
        0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x01, 0x00,

    /* Configuration descriptor */
        0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x02, 0x00, 0x01, 0x03, 0x00, 0x00,
        0x00,

    /* HID descriptor */
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_KEYBOARD_REPORT_LENGTH),
        MSB(HID_KEYBOARD_REPORT_LENGTH),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };


    /* String Device Framework :
     Byte 0 and 1 : Word containing the language ID : 0x0904 for US
     Byte 2       : Byte containing the index of the descriptor
     Byte 3       : Byte containing the length of the descriptor string
    */

#define STRING_FRAMEWORK_LENGTH 40
static UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
        0x09, 0x04, 0x02, 0x0c,
        0x55, 0x53, 0x42, 0x20, 0x4b, 0x65, 0x79, 0x62,
        0x6f, 0x61, 0x72, 0x64,

    /* Serial Number string descriptor : Index 3 */
        0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31
    };


    /* Multiple languages are supported on the device, to add
       a language besides english, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
#define LANGUAGE_ID_FRAMEWORK_LENGTH 2
static UCHAR language_id_framework[] = {

    /* English. */
        0x09, 0x04
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

    /* @BUG_FIX_PENDING: ux_dcd_sim_slave_function.c doesn't support transfer aborts, which happen during device unregistration of a class. */
    if (error_code != UX_FUNCTION_NOT_SUPPORTED)
    {

        /* Failed test.  */
        printf("Error on line %d, system_level: %d, system_context: %d, error code: %d\n", __LINE__, system_level, system_context, error_code);
        test_control_return(1);
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_hid_keyboard_callback_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
UINT                            descriptor_size = HID_KEYBOARD_REPORT_LENGTH;

    /* Inform user.  */
    printf("Running HID Keyboard Callback Test.................................. ");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name, ux_host_class_hid_keyboard_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
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

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the hid class parameters for a keyboard.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_keyboard_report;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_KEYBOARD_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;

    /* Initilize the device hid class. The class is connected with interface 2 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,2, (VOID *)&hid_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }


    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main demo thread.  */
    status =  tx_thread_create(&tx_demo_thread_slave_simulation, "tx demo slave simulation", tx_demo_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

}


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT    status;
UINT    max_hid_loop;

    /* Initialize max loop value.  */
    max_hid_loop = 2*UX_HOST_CLASS_HID_KEYBOARD_USAGE_ARRAY_LENGTH;

    /* Find the HID class */
    status =  demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Get the HID client */
    hid_client = hid -> ux_host_class_hid_client;

    /* Check if the instance of the keyboard is live */
    while (hid_client -> ux_host_class_hid_client_local_instance == UX_NULL)
        tx_thread_sleep(10);

    /* Get the keyboard instance */
    keyboard =  (UX_HOST_CLASS_HID_KEYBOARD *)hid_client -> ux_host_class_hid_client_local_instance;

    /* Init the keyboard queue index.  */
    keyboard_queue_index = 0;

    while (max_hid_loop--)
    {
        /* Get a key/state from the keyboard.  */
        status = ux_host_class_hid_keyboard_key_get(keyboard, &keyboard_char, &keyboard_state);

        /* Check if there is something.  */
        if (status == UX_SUCCESS)
        {
            /* We have a character in the queue.  */
            keyboard_queue[keyboard_queue_index] = (UCHAR) keyboard_char;

            /* Can we accept more ?  */
            if(keyboard_queue_index < 1024)
                keyboard_queue_index++;

        }

        tx_thread_sleep(10);
    }

    /* Now disconnect the device.  */
    _ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}

static UINT    demo_thread_hid_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{
    return(UX_SUCCESS);
}

static void  tx_demo_thread_slave_simulation_entry(ULONG arg)
{

UX_SLAVE_DEVICE                 *device;
UX_SLAVE_INTERFACE              *interface;
UX_SLAVE_CLASS_HID              *hid;
UX_SLAVE_CLASS_HID_EVENT        hid_event;
UCHAR                           key1;
UCHAR                           key2;

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* Set the first key to 'a' which is 04.  */
    key1 = 0x04;

    /* Set the second key to the first keypad key. */
    key2 = 0x54;

    /* reset the HID event structure.  */
    ux_utility_memory_set(&hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));

    /* First byte is a modifier byte.  */
    hid_event.ux_device_class_hid_event_buffer[0] = 0xff;

    /* Second byte is part modifier and part padding. */
    hid_event.ux_device_class_hid_event_buffer[1] = 0xff;

    while(1)
    {

        /* Is the device configured ? */
        while (device -> ux_slave_device_state != UX_DEVICE_CONFIGURED)

            /* Then wait.  */
            tx_thread_sleep(10);

        /* Until the device stays configured.  */
        while (device -> ux_slave_device_state == UX_DEVICE_CONFIGURED)
        {

            /* Get the interface.  We use the first interface, this is a simple device.  */
            interface =  device -> ux_slave_device_first_interface;

            /* Form that interface, derive the HID owner.  */
            hid = interface -> ux_slave_interface_class_instance;

            /* Wait for 2 seconds. */
            ux_utility_thread_sleep(20);

            /* Then insert a key into the keyboard event.  Length is fixed to 8.  */
            hid_event.ux_device_class_hid_event_length = 8;

            /* Toggle first and second byte.  */
            hid_event.ux_device_class_hid_event_buffer[0] ^= 0xff;
            hid_event.ux_device_class_hid_event_buffer[1] ^= 0xff;

            /* The 6 next bytes are keys. We only have two keys here.  */
            hid_event.ux_device_class_hid_event_buffer[2] = key1;
            hid_event.ux_device_class_hid_event_buffer[3] = key2;

            /* Set the keyboard event.  */
            ux_device_class_hid_event_set(hid, &hid_event);

            /* Next event has the key depressed.  */
            hid_event.ux_device_class_hid_event_buffer[2] = 0;

            /* Length is fixed to 8.  */
            hid_event.ux_device_class_hid_event_length = 8;

            /* Set the keyboard event.  */
            ux_device_class_hid_event_set(hid, &hid_event);

            /* Are we at the end of alphabet?  */
            if (key1 != (0x04 + 26))

                /* Next key.  */
                key1++;

            else

                /* Start over again.  */
                key1 = 0x04;

            /* Are we at the end of keypad keys?  */
            if (key2 != 0x67)

                /* Next key.  */
                key2++;

            else

                /* Start over again.  */
                key2 = 0x54;

        }
    }
}

