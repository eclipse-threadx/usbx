/* This test concentrates on the ux_host_class_hid_report_get API. */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"


static UCHAR hid_report_descriptor[] = { 

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
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
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
#define HID_REPORT_LENGTH sizeof(hid_report_descriptor)/sizeof(hid_report_descriptor[0])


#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED 52
static UCHAR device_framework_full_speed[DEVICE_FRAMEWORK_LENGTH_FULL_SPEED] = {

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
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_REPORT_LENGTH),
        MSB(HID_REPORT_LENGTH),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };
    
    
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED 62
static UCHAR device_framework_high_speed[DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED] = {

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
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_REPORT_LENGTH),
        MSB(HID_REPORT_LENGTH),

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

UINT  _ux_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter);

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_class_hid_report_get_test_application_define(void *first_unused_memory)
#endif
{
    
UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Inform user.  */
    printf("Running ux_host_class_hid_report_get Test........................... ");
    
    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
    
    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name, ux_host_class_hid_keyboard_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
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

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the hid class parameters for a keyboard.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_report_descriptor;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;

    /* Initilize the device hid class. The class is connected with interface 2 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry, 
                                                1,2, (VOID *)&hid_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }


    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
    
    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,  
            stack_pointer, UX_DEMO_STACK_SIZE, 
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
            
    /* Create the main demo thread.  */
    status =  tx_thread_create(&tx_demo_thread_slave_simulation, "tx demo slave simulation", tx_demo_thread_slave_simulation_entry, 0,  
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE, 
            20, 20, 1, TX_AUTO_START);
      
    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

}

static UINT ux_hcd_sim_host_entry_filter(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT status;

    switch(function)
    {
    case UX_HCD_TRANSFER_REQUEST:

        status = UX_ERROR;

        break;


    default:

        status = _ux_hcd_sim_host_entry(hcd, function, parameter);
        break;
    }

    return status;
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                status;
UX_HOST_CLASS_HID_REPORT_GET_ID     report_id;
UX_HOST_CLASS_HID_CLIENT_REPORT     input_report_request;
UX_HOST_CLASS_HID_REPORT            *input_report_descriptor;
UX_HOST_CLASS_HID_KEYBOARD          *keyboard;
UCHAR                               input_report_buffer_char[1024];
ULONG                               *input_report_buffer_long = (ULONG *)input_report_buffer_char;


    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the HID client */
    hid_client = hid -> ux_host_class_hid_client;

    /* Check if the instance of the keyboard is live */
    while (hid_client -> ux_host_class_hid_client_local_instance == UX_NULL)
        tx_thread_sleep(10);

    /* Get the keyboard instance */
    keyboard = (UX_HOST_CLASS_HID_KEYBOARD *)hid_client->ux_host_class_hid_client_local_instance;

    /**************************************************/
    /** Test case: functionality test. **/
    /**************************************************/

    /* Disable the default method of retrieving events (periodic interrupt endpoint) since we'll be using control transfers. */
    status = ux_host_class_hid_periodic_report_stop(hid);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the report ID for the keyboard. */
    report_id.ux_host_class_hid_report_get_report = UX_NULL;
    report_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    status = _ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    input_report_descriptor = report_id.ux_host_class_hid_report_get_report;

    /* Fill out input report request. */
    input_report_request.ux_host_class_hid_client_report = input_report_descriptor;
    input_report_request.ux_host_class_hid_client_report_buffer = input_report_buffer_long;
    input_report_request.ux_host_class_hid_client_report_length = input_report_descriptor -> ux_host_class_hid_report_byte_length;

    /* For the first part of this test, we request the raw report. */
    
    input_report_request.ux_host_class_hid_client_report_flags = UX_HOST_CLASS_HID_REPORT_RAW;

    /* Poll until we've received an input report with actual data. */
    while (1)
    {

        /* Reset the actual length. */
        input_report_request.ux_host_class_hid_client_report_actual_length = 0;

        /* Get a raw input report from the device. */
        status = ux_host_class_hid_report_get(hid, &input_report_request);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        /* Was there actual data? The modifier should be set if there is. */
        if (input_report_buffer_char[0] != 0)
            break;

        tx_thread_sleep(10);
    }

    /* Check the data. */

    /* Is this not the modifier we're expecting? */
    if (input_report_buffer_char[0] != 0x45)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Are these not the keys we're expecting? */
    if (input_report_buffer_char[2] != 0x04 ||
        input_report_buffer_char[3] != 0x05 ||
        input_report_buffer_char[4] != 0x06 ||
        input_report_buffer_char[5] != 0x07 ||
        input_report_buffer_char[6] != 0x08 ||
        input_report_buffer_char[7] != 0x09)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* We just tested getting the report raw; this second part is getting it decompressed.
       Note that the decompressed version is interleaved as 'Usage Value Usage Value...' */
    
    input_report_request.ux_host_class_hid_client_report_flags = UX_HOST_CLASS_HID_REPORT_DECOMPRESSED;

    /* Poll until we've received an input report with actual data. */
    while (1)
    {

        /* Reset the actual length. */
        input_report_request.ux_host_class_hid_client_report_actual_length = 0;

        /* Get a raw input report from the device. */
        status = ux_host_class_hid_report_get(hid, &input_report_request);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d\n", __LINE__);
            test_control_return(1);
        }

        /* Was there actual data? This value should be non-zero if it is. */
        if (input_report_buffer_long[2*0 + 1] != 0)
            break;

        tx_thread_sleep(10);
    }

    /* Check the data. */

    /* Are these not the modifiers we're expecting? */
    if (input_report_buffer_long[2*0 + 0] != 0x000700e0 ||
        input_report_buffer_long[2*0 + 1] != 1 ||
        input_report_buffer_long[2*1 + 0] != 0x000700e1 ||
        input_report_buffer_long[2*1 + 1] != 0 ||
        input_report_buffer_long[2*2 + 0] != 0x000700e2 ||
        input_report_buffer_long[2*2 + 1] != 1 ||
        input_report_buffer_long[2*3 + 0] != 0x000700e3 ||
        input_report_buffer_long[2*3 + 1] != 0 ||
        input_report_buffer_long[2*4 + 0] != 0x000700e4 ||
        input_report_buffer_long[2*4 + 1] != 0 ||
        input_report_buffer_long[2*5 + 0] != 0x000700e5 ||
        input_report_buffer_long[2*5 + 1] != 0 ||
        input_report_buffer_long[2*6 + 0] != 0x000700e6 ||
        input_report_buffer_long[2*6 + 1] != 1 ||
        input_report_buffer_long[2*7 + 0] != 0x000700e7 ||
        input_report_buffer_long[2*7 + 1] != 0)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Are these not the usages and keys we're expecting? */
    if (input_report_buffer_long[2*9  + 0] != 0x00070004 ||
        input_report_buffer_long[2*9  + 1] != 0x00000004 ||
        input_report_buffer_long[2*10 + 0] != 0x00070005 ||
        input_report_buffer_long[2*10 + 1] != 0x00000005 ||
        input_report_buffer_long[2*11 + 0] != 0x00070006 ||
        input_report_buffer_long[2*11 + 1] != 0x00000006 ||
        input_report_buffer_long[2*12 + 0] != 0x00070007 ||
        input_report_buffer_long[2*12 + 1] != 0x00000007 ||
        input_report_buffer_long[2*13 + 0] != 0x00070008 ||
        input_report_buffer_long[2*13 + 1] != 0x00000008 ||
        input_report_buffer_long[2*14 + 0] != 0x00070009 ||
        input_report_buffer_long[2*14 + 1] != 0x00000009)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
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
    
    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;
   
    /* reset the HID event structure.  */
    ux_utility_memory_set(&hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));
    
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

            /* First byte is a modifier byte. Set some bits: 10100010.  */
            hid_event.ux_device_class_hid_event_buffer[0] = 0x45;

            /* Second byte is reserved. */
            hid_event.ux_device_class_hid_event_buffer[1] = 0;
            
            /* The 6 next bytes are keys.  */
            hid_event.ux_device_class_hid_event_buffer[2] = 0x04;
            hid_event.ux_device_class_hid_event_buffer[3] = 0x05;
            hid_event.ux_device_class_hid_event_buffer[4] = 0x06;
            hid_event.ux_device_class_hid_event_buffer[5] = 0x07;
            hid_event.ux_device_class_hid_event_buffer[6] = 0x08;
            hid_event.ux_device_class_hid_event_buffer[7] = 0x09;

            /* Set the keyboard event.  */
            ux_device_class_hid_event_set(hid, &hid_event);
        }
    }
}

