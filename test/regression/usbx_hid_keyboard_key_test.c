/* This file tests that the host correctly receives the keys the device sends to it. */

#include "usbx_test_common_hid.h"

#include "ux_host_class_hid_keyboard.h"

#define HOST_WAIT_TIME  1
#define SLAVE_WAIT_TIME (6*HOST_WAIT_TIME)

#ifndef UX_HID_KEYBOARD_PHANTOM_STATE
#define UX_HID_KEYBOARD_PHANTOM_STATE 0x01
#endif
#define KEY_START (UX_HID_KEYBOARD_PHANTOM_STATE+1)

static UCHAR ux_host_class_hid_keyboard_regular_array[] =
{
   0,0,0,0,                                                                 
   'a','b','c','d','e','f','g','h','i','j','k','l','m','n',                 
   'o','p','q','r','s','t','u','v','w','x','y','z',                         
   '1','2','3','4','5','6','7','8','9','0',                                 
   0x0d,0x1b,0x08,0x07,0x20,'-','=','[',']',                                
   '\\','#',';',0x27,'`',',','.','/',0xf0,                                  
   0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,             
   0x00,0xf1,0x00,0xd2,0xc7,0xc9,0xd3,0xcf,0xd1,0xcd,0xcb,0xd0,0xc8,0xf2,   
   '/','*','-','+',                                                         
   0x0d,'1','2','3','4','5','6','7','8','9','0','.','\\',0x00,0x00,'=',     
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static UCHAR ux_host_class_hid_keyboard_capslock_array[] =
{
   0,0,0,0,
   'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
   '1','2','3','4','5','6','7','8','9','0',
   0x0d,0x1b,0x08,0x07,0x20,'-','=','[',']',
   '\\','#',';',0x27,'`',',','.','/',0xf0,
   0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,
   0x00,0xf1,0x00,0xd2,0xc7,0xc9,0xd3,0xcf,0xd1,0xcd,0xcb,0xd0,0xc8,0xf2,
   '/','*','-','+',
   0x0d,'1','2','3','4','5','6','7','8','9','0','.','\\',0x00,0x00,'=',
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static UCHAR ux_host_class_hid_keyboard_shift_array[] =
{
   0,0,0,0,                                                                 
   'A','B','C','D','E','F','G','H','I','J','K','L','M','N',                 
   'O','P','Q','R','S','T','U','V','W','X','Y','Z',                         
   '!','@','#','$','%','^','&','*','(',')',                                 
   0x0d,0x1b,0x08,0x07,0x20,'_','+','{','}',                                
   '|','~',':','"','~','<','>','?',0xf0,                                    
   0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,             
   0x00,0xf1,0x00,0xd2,0xc7,0xc9,0xd3,0xcf,0xd1,0xcd,0xcb,0xd0,0xc8,0xf2,   
   '/','*','-','+',                                                         
   0x0d,'1','2','3','4','5','6','7','8','9','0','.','\\',0x00,0x00,'=',     
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static UCHAR ux_host_class_hid_keyboard_shift_capslock_array[] =
{
   0,0,0,0,
   'a','b','c','d','e','f','g','h','i','j', 'k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
   '!','@','#','$','%','^','&','*','(',')',
   0x0d,0x1b,0x08,0x07,0x20,'_','+','{','}',
   '|','~',':','"','~','<','>','?',0xf0,
   0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,
   0x00,0xf1,0x00,0xd2,0xc7,0xc9,0xd3,0xcf,0xd1,0xcd,0xcb,0xd0,0xc8,0xf2,
   '/','*','-','+',
   0x0d,'1','2','3','4','5','6','7','8','9','0','.','\\',0x00,0x00,'=',
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static UCHAR ux_host_class_hid_keyboard_numlock_on_array[] =
{
   '/','*','-','+',
   0x0d,'1','2','3','4','5','6','7','8','9','0','.','\\',0x00,0x00,'=',
};

static UCHAR ux_host_class_hid_keyboard_numlock_off_array[] =
{
   '/','*','-','+',
   0x0d,0xcf,0xd0,0xd1,0xcb,'5',0xcd,0xc7,0xc8,0xc9,0xd2,0xd3,'\\',0x00,0x00,'=',
};


#define DUMMY_USBX_MEMORY_SIZE (64*1024)
static UCHAR dummy_usbx_memory[DUMMY_USBX_MEMORY_SIZE];

static volatile ULONG test_host_phase = 0;
static volatile ULONG test_slave_phase = 0;

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
    0x25, ARRAY_COUNT(ux_host_class_hid_keyboard_regular_array) + 1,  //   LOGICAL_MAXIMUM ()
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, ARRAY_COUNT(ux_host_class_hid_keyboard_regular_array) + 1,  //   USAGE_MAXIMUM ()
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


UINT  _ux_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter);


static UINT ux_system_host_change_function(ULONG a, UX_HOST_CLASS *b, VOID *c)
{
    return 0;
}

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    /* @BUG_FIX_PENDING: ux_dcd_sim_slave_function.c doesn't support transfer aborts, which happen during device unregistration of a class. */
    if (error_code != UX_FUNCTION_NOT_SUPPORTED &&
        error_code != UX_BUFFER_OVERFLOW /* Key queue overflow! */)
    {

        /* Failed test.  */
        printf("Error on line %d, system_level: %d, system_context: %d, error code: 0x%x\n", __LINE__, system_level, system_context, error_code);
        test_control_return(1);
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_hid_keyboard_key_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Inform user.  */
    printf("Running HID Keyboard Key Test....................................... ");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(ux_system_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name, ux_host_class_hid_keyboard_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
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

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the hid class parameters.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_report_descriptor;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;

    /* Initialize the device hid class. The class is connected with interface 2 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,2, (VOID *)&hid_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main demo thread.  */
    status =  tx_thread_create(&tx_demo_thread_slave_simulation, "tx demo slave simulation", tx_demo_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Running HID Keyboard Basic Functionality Test....................... ERROR #10\n");
        test_control_return(1);
    }

}


static UINT  _wait_key(UX_HOST_CLASS_HID_KEYBOARD *keyboard, ULONG *keyboard_key, ULONG *keyboard_state)
{

UINT        status;
UINT        i;


    for(i = 0; i < 200; i ++)
    {
        status = ux_host_class_hid_keyboard_key_get(keyboard, keyboard_key, keyboard_state);
        if (status == UX_SUCCESS)
        {
            // printf("Key: %lx,%lx\n", *keyboard_key, *keyboard_state);

#if defined(UX_HOST_CLASS_HID_KEYBOARD_EVENTS_KEY_CHANGES_MODE_REPORT_LOCK_KEYS) || defined(UX_HOST_CLASS_HID_KEYBOARD_EVENTS_KEY_CHANGES_MODE_REPORT_MODIFIER_KEYS)
            if (*keyboard_state & UX_HID_KEYBOARD_STATE_FUNCTION)
                continue;
#endif

#if !defined(UX_HOST_CLASS_HID_KEYBOARD_EVENTS_KEY_CHANGES_MODE_REPORT_KEY_DOWN_ONLY)
            if (*keyboard_state & UX_HID_KEYBOARD_STATE_KEY_UP)
                continue;
#endif
            return UX_SUCCESS;
        }
        _ux_utility_delay_ms(1);
    }
    return UX_ERROR;
}


static UINT  tx_demo_phase_sync(ULONG in_host, ULONG nb_loop, ULONG tick_in_loop)
{

ULONG i;


    for (i = 0; i < nb_loop; i ++)
    {
        if (in_host)
        {
            if (test_host_phase <= test_slave_phase)
                return UX_SUCCESS;
        }
        else
        {
            if (test_slave_phase <= test_host_phase)
                return UX_SUCCESS;
        }
        _ux_utility_thread_sleep(tick_in_loop);
    }
    return UX_ERROR;
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                            status;
UX_HOST_CLASS_HID_KEYBOARD      *keyboard;
ULONG                           keyboard_key;
ULONG                           keyboard_state;
ULONG                           expected_key;
ULONG                           num_keypad_keys;
ULONG                           i, n;
UCHAR                           *test_array[4] = {
    ux_host_class_hid_keyboard_regular_array,
    ux_host_class_hid_keyboard_shift_array,
    ux_host_class_hid_keyboard_capslock_array,
    ux_host_class_hid_keyboard_shift_capslock_array
};

    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Get the HID client */
    hid_client = hid -> ux_host_class_hid_client;

    /* Check if the instance of the keyboard is live */
    while (hid_client -> ux_host_class_hid_client_local_instance == UX_NULL)
        tx_thread_sleep(10);

    /* Get the keyboard instance */
    keyboard =  (UX_HOST_CLASS_HID_KEYBOARD *)hid_client -> ux_host_class_hid_client_local_instance;

    /**************************************************************************/
    /** 1. Test receiving maximum keys at once.                              **/
    test_host_phase ++;
    // printf("###### host step: %ld ######\n", test_host_phase);

    for (i = 4; i < 10; i++)
    {

        status = _wait_key(keyboard, &keyboard_key, &keyboard_state);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d: code 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        expected_key = ux_host_class_hid_keyboard_regular_array[i];

        if (keyboard_key != expected_key)
        {

            printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }
    }

    /**************************************************************************/
    /** 2. Test keys.                                                        **/
    test_host_phase ++;
    // printf("###### host step: %ld ######\n", test_host_phase);

    for (n = 0; n < 4; n ++)
    {
        for (i = KEY_START; i < ARRAY_COUNT(ux_host_class_hid_keyboard_regular_array); i++)
        {

            /* These keys change the keyboard state and are not retrievable. */
            if (i == UX_HID_LED_KEY_CAPS_LOCK ||
                i == UX_HID_LED_KEY_NUM_LOCK ||
                i == UX_HID_LED_KEY_SCROLL_LOCK)
                continue;

            status = _wait_key(keyboard, &keyboard_key, &keyboard_state);
            if (status != UX_SUCCESS)
            {
                printf("ERROR #%d: code 0x%x\n", __LINE__, status);
                test_control_return(1);
            }

            // printf("test key: %lx @ %lx\n", keyboard_key, keyboard_state);
            expected_key = test_array[n][i];

            if (keyboard_key != expected_key)
            {

                printf("Error on line %d, test %ld.%ld, state 0x%lx, key 0x%lx <> 0x%lx\n", __LINE__, n, i, keyboard_state, expected_key, keyboard_key);
                test_control_return(1);
            }
        }
    }

    /**************************************************************************/
    /** 3. Test number pad keys.                                             **/
    test_host_phase ++;
    // printf("###### host step: %ld ######\n", test_host_phase);

    /** Test numlock on keys.                                                **/

    num_keypad_keys = (UX_HID_KEYBOARD_KEYS_KEYPAD_UPPER_RANGE - UX_HID_KEYBOARD_KEYS_KEYPAD_LOWER_RANGE) + 1;

    for (i = 0; i < ARRAY_COUNT(ux_host_class_hid_keyboard_numlock_on_array); i++)
    {

        status = _wait_key(keyboard, &keyboard_key, &keyboard_state);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d: code 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        expected_key = ux_host_class_hid_keyboard_numlock_on_array[i];

        if (keyboard_key != ux_host_class_hid_keyboard_numlock_on_array[i])
        {

            printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }
    }

    /** Test numlock off keys. **/

    for (i = 0; i < ARRAY_COUNT(ux_host_class_hid_keyboard_numlock_off_array); i++)
    {

        status = _wait_key(keyboard, &keyboard_key, &keyboard_state);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d: code 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        // printf("test key: %lx @ %lx\n", keyboard_key, keyboard_state);

        expected_key = ux_host_class_hid_keyboard_numlock_off_array[i];

        if (keyboard_key != ux_host_class_hid_keyboard_numlock_off_array[i])
        {

            printf("Error on line %d, test %ld, state 0x%lx, key 0x%lx <> 0x%lx\n", __LINE__, i, keyboard_state, expected_key, keyboard_key);
            test_control_return(1);
        }
    }

    /**************************************************************************/
    /** 4. Test states.                                                      **/
    test_host_phase ++;
    // printf("###### host step: %ld ######\n", test_host_phase);

    /** Test keyboard on states. **/

    status = _wait_key(keyboard, &keyboard_key, &keyboard_state);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Ensure every bit is enabled (represented by 0xff07 (search for "Define HID Keyboard States." in ux_host_class_hid_keyboard.h)). */
    if ((keyboard_state & 0xFFFF) != 0xff07)
    {

        printf("Error on line %d, test %ld.%ld, state 0x%lx\n", __LINE__, n, i, keyboard_state);
        test_control_return(1);
    }

    /** Test keyboard off states. **/

    status = _wait_key(keyboard, &keyboard_key, &keyboard_state);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Ensure every bit is disabled. */
    if ((keyboard_state & 0xFFFF) != 0x0000)
    {

        printf("Error on line %d, test %ld.%ld, state 0x%lx\n", __LINE__, n, i, keyboard_state);
        test_control_return(1);
    }

    /**************************************************************************/
    /** 5. Test raw key.                                                     **/

    /* Disable decode. */
    ux_host_class_hid_keyboard_ioctl(keyboard, UX_HID_KEYBOARD_IOCTL_DISABLE_KEYS_DECODE, UX_NULL);
    test_host_phase ++;
    // printf("###### host step: %ld ######\n", test_host_phase);

    for (i = KEY_START; i < ARRAY_COUNT(ux_host_class_hid_keyboard_regular_array); i++)
    {
#if 0
        /* These keys change the keyboard state and are not retrievable. */
        if (i == UX_HID_LED_KEY_CAPS_LOCK ||
            i == UX_HID_LED_KEY_NUM_LOCK ||
            i == UX_HID_LED_KEY_SCROLL_LOCK)
            continue;
#endif

        status = _wait_key(keyboard, &keyboard_key, &keyboard_state);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d: code 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        expected_key = i;

        if (keyboard_key != expected_key)
        {

            printf("Error on line %d, test %ld, 0x%lx <> 0x%lx\n", __LINE__, i, expected_key, keyboard_key);
            // test_control_return(1);
        }
    }

    /**************************************************************************/
    /** 6. Test invalid key.                                                 **/

    /* Enable decode. */
    ux_host_class_hid_keyboard_ioctl(keyboard, UX_HID_KEYBOARD_IOCTL_ENABLE_KEYS_DECODE, UX_NULL);
    test_host_phase ++;
    // printf("###### host step: %ld ######\n", test_host_phase);

    /* Wait slave execute. */
    if (tx_demo_phase_sync(UX_TRUE, 10, SLAVE_WAIT_TIME) != UX_SUCCESS)
    {
        printf("Error in line %d, thread phase error %ld <> %ld!\n", __LINE__, test_host_phase, test_slave_phase);
        test_control_return(1);
    }

    /* Wait a invalid key (discarded). */
    ux_utility_thread_sleep(HOST_WAIT_TIME * 2);

    /**************************************************************************/
    /** 7. Test key array overflow.                                          **/

    /* Flush keys.  */
    do {
        status = ux_host_class_hid_keyboard_key_get(keyboard, &keyboard_key, &keyboard_state);
    } while(status == UX_SUCCESS);

    /* No read, test array full. */
    ux_host_class_hid_keyboard_ioctl(keyboard, UX_HID_KEYBOARD_IOCTL_DISABLE_KEYS_DECODE, UX_NULL);
    test_host_phase ++;
    // printf("###### host step: %ld ######\n", test_host_phase);

    /* Wait until test done. */
    while(test_slave_phase != 0)
        _ux_utility_delay_ms(10);

    /* There are keys remain in key array. */
    for (i = 0; i < UX_HOST_CLASS_HID_KEYBOARD_USAGE_ARRAY_LENGTH / 2 - 1; i ++)
    {

        status = ux_host_class_hid_keyboard_key_get(keyboard, &keyboard_key, &keyboard_state);
        if(status != UX_SUCCESS)
        {
            printf("Error on line %d.%ld, error code 0x%x\n", __LINE__, i, status);
            test_control_return(1);
        }
        if (keyboard_key != 5)
        {
            printf("Error on line %d, key 0x%lx,0x%lx @ %ld\n", __LINE__, keyboard_key, keyboard_state, i);
            test_control_return(1);
        }
    }
    status = ux_host_class_hid_keyboard_key_get(keyboard, &keyboard_key, &keyboard_state);
    if(status == UX_SUCCESS)
    {
        printf("Error on line %d, error code 0x%x\n", __LINE__, status);
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

UINT                            status;
UX_SLAVE_DEVICE                 *device;
UX_SLAVE_INTERFACE              *interface;
UX_SLAVE_CLASS_HID              *hid;
UX_SLAVE_CLASS_HID_EVENT        hid_event;
UINT                            i, n;
UCHAR                           state_modifier[2] = {0, (0x01 << 1)};

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* reset the HID event structure.  */
    ux_utility_memory_set(&hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));

    /* Is the device configured ? */
    while (device->ux_slave_device_state != UX_DEVICE_CONFIGURED)

        /* Then wait.  */
        tx_thread_sleep(10);

    /* Get the interface.  We use the first interface, this is a simple device.  */
    interface = device->ux_slave_device_first_interface;

    /* Form that interface, derive the HID owner.  */
    hid = interface->ux_slave_interface_class_instance;

    /**************************************************************************/
    /** 1. Test receiving maximum keys at once.  **/
    test_slave_phase ++;
    // printf("****** slave step: %ld ******\n", test_slave_phase);

    hid_event.ux_device_class_hid_event_length = 8;

    /* Modification byte. */
    hid_event.ux_device_class_hid_event_buffer[0] = 0;

    /* Reserved byte. */
    hid_event.ux_device_class_hid_event_buffer[1] = 0;

    /* Set keys. */
    for (i = 0; i < 6; i++)
        hid_event.ux_device_class_hid_event_buffer[2 + i] = 4 + i;

    ux_device_class_hid_event_set(hid, &hid_event);
#if 0
    /* Release keys. */
    for (i = 0; i < 6; i++)
        hid_event.ux_device_class_hid_event_buffer[2 + i] = 0;

    ux_device_class_hid_event_set(hid, &hid_event);
#endif
    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /* Reset for next test. */
    ux_utility_memory_set(&hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));

    /**************************************************************************/
    /** 2. Test keys.                                                        **/
    test_slave_phase ++;
    // printf("****** slave step: %ld ******\n", test_slave_phase);

    /** Test regular keys with/without SHIFT. Only go up to maximum value specified by report descriptor. **/

    for (n = 0; n < 2; n ++)
    {

        for (i = KEY_START; i < ARRAY_COUNT(ux_host_class_hid_keyboard_regular_array); i++)
        {

            /* These keys change the keyboard state and are not retrievable. */
            if (i == UX_HID_LED_KEY_CAPS_LOCK ||
                i == UX_HID_LED_KEY_NUM_LOCK ||
                i == UX_HID_LED_KEY_SCROLL_LOCK)
                continue;

            hid_event.ux_device_class_hid_event_length = 8;

            /* Modification byte. */
            hid_event.ux_device_class_hid_event_buffer[0] = state_modifier[n];

            /* Reserved byte. */
            hid_event.ux_device_class_hid_event_buffer[1] = 0;

            /* Key byte. */
            hid_event.ux_device_class_hid_event_buffer[2] = i;

            status = ux_device_class_hid_event_set(hid, &hid_event);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
                test_control_return(1);
            }

            ux_utility_thread_sleep(SLAVE_WAIT_TIME);
        }

    }
    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /** Test CAPS LOCK with/without SHIFT keys. Only go up to maximum value specified by report descriptor. **/

    /* Turn CAPS LOCK on. */
    hid_event.ux_device_class_hid_event_length = 8;
    hid_event.ux_device_class_hid_event_buffer[0] = 0;
    hid_event.ux_device_class_hid_event_buffer[1] = 0;
    hid_event.ux_device_class_hid_event_buffer[2] = UX_HID_LED_KEY_CAPS_LOCK;
    status = ux_device_class_hid_event_set(hid, &hid_event);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    for (n = 0; n < 2; n ++)
    {

        for (i = KEY_START; i < ARRAY_COUNT(ux_host_class_hid_keyboard_regular_array); i++)
        {

            /* These keys change the keyboard state and are not retrievable. */
            if (i == UX_HID_LED_KEY_CAPS_LOCK ||
                i == UX_HID_LED_KEY_NUM_LOCK ||
                i == UX_HID_LED_KEY_SCROLL_LOCK)
                continue;

            hid_event.ux_device_class_hid_event_length = 8;

            /* Modification byte. */
            hid_event.ux_device_class_hid_event_buffer[0] = state_modifier[n];

            /* Reserved byte. */
            hid_event.ux_device_class_hid_event_buffer[1] = 0;

            /* Key byte. */
            hid_event.ux_device_class_hid_event_buffer[2] = i;

            status = ux_device_class_hid_event_set(hid, &hid_event);
            if (status != UX_SUCCESS)
            {

                /* ERROR */
                printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
                test_control_return(1);
            }

            ux_utility_thread_sleep(SLAVE_WAIT_TIME);
        }
    }
    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /* Turn CAPS LOCK off. */
    hid_event.ux_device_class_hid_event_length = 8;
    hid_event.ux_device_class_hid_event_buffer[0] = 0;
    hid_event.ux_device_class_hid_event_buffer[1] = 0;
    hid_event.ux_device_class_hid_event_buffer[2] = UX_HID_LED_KEY_CAPS_LOCK;
    status = ux_device_class_hid_event_set(hid, &hid_event);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /**************************************************************************/
    /** 3. Test num pad keys.                                                **/
    test_slave_phase ++;
    // printf("****** slave step: %ld ******\n", test_slave_phase);

    /** Test NUM LOCK on keys. NUM LOCK is on by default. **/

    for (i = UX_HID_KEYBOARD_KEYS_KEYPAD_LOWER_RANGE; i <= UX_HID_KEYBOARD_KEYS_KEYPAD_UPPER_RANGE; i++)
    {

        hid_event.ux_device_class_hid_event_length = 8;

        /* Modification byte. */
        hid_event.ux_device_class_hid_event_buffer[0] = 0;

        /* Reserved byte. */
        hid_event.ux_device_class_hid_event_buffer[1] = 0;

        /* Key byte. */
        hid_event.ux_device_class_hid_event_buffer[2] = i;

        status = ux_device_class_hid_event_set(hid, &hid_event);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        ux_utility_thread_sleep(SLAVE_WAIT_TIME);
    }
    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /** Test NUM LOCK off keys. **/

    /* Turn NUM LOCK off. */
    hid_event.ux_device_class_hid_event_length = 8;
    hid_event.ux_device_class_hid_event_buffer[0] = 0;
    hid_event.ux_device_class_hid_event_buffer[1] = 0;
    hid_event.ux_device_class_hid_event_buffer[2] = UX_HID_LED_KEY_NUM_LOCK;
    ux_device_class_hid_event_set(hid, &hid_event);

    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    for (i = UX_HID_KEYBOARD_KEYS_KEYPAD_LOWER_RANGE; i <= UX_HID_KEYBOARD_KEYS_KEYPAD_UPPER_RANGE; i++)
    {

        hid_event.ux_device_class_hid_event_length = 8;

        /* Modification byte. */
        hid_event.ux_device_class_hid_event_buffer[0] = 0;

        /* Reserved byte. */
        hid_event.ux_device_class_hid_event_buffer[1] = 0;

        /* Key byte. */
        hid_event.ux_device_class_hid_event_buffer[2] = i;

        status = ux_device_class_hid_event_set(hid, &hid_event);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        ux_utility_thread_sleep(SLAVE_WAIT_TIME);
    }
    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /* Turn NUM LOCK on. */
    hid_event.ux_device_class_hid_event_length = 8;
    hid_event.ux_device_class_hid_event_buffer[0] = 0;
    hid_event.ux_device_class_hid_event_buffer[1] = 0;
    hid_event.ux_device_class_hid_event_buffer[2] = UX_HID_LED_KEY_NUM_LOCK;
    status = ux_device_class_hid_event_set(hid, &hid_event);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /**************************************************************************/
    /** 4. Test states.                                                      **/
    test_slave_phase ++;
    // printf("****** slave step: %ld ******\n", test_slave_phase);

    /** Test key states. **/

    /* Enable every bit in key state. */
    hid_event.ux_device_class_hid_event_length = 8;
    hid_event.ux_device_class_hid_event_buffer[0] = 0xff;
    hid_event.ux_device_class_hid_event_buffer[1] = 0;
    hid_event.ux_device_class_hid_event_buffer[2] = UX_HID_LED_KEY_CAPS_LOCK;
    hid_event.ux_device_class_hid_event_buffer[3] = UX_HID_LED_KEY_SCROLL_LOCK;
    hid_event.ux_device_class_hid_event_buffer[4] = UX_HID_LED_KEY_NUM_LOCK;

    status = ux_device_class_hid_event_set(hid, &hid_event);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /* Add a key so the host can receive it, allowing checking of key state. */
    hid_event.ux_device_class_hid_event_length = 8;
    hid_event.ux_device_class_hid_event_buffer[0] = 0xff;
    hid_event.ux_device_class_hid_event_buffer[1] = 0;
    hid_event.ux_device_class_hid_event_buffer[2] = 0;
    hid_event.ux_device_class_hid_event_buffer[3] = 0;
    hid_event.ux_device_class_hid_event_buffer[4] = 0x04;

    status = ux_device_class_hid_event_set(hid, &hid_event);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /** Test key states. **/

    /* Disable every bit in key state. */
    hid_event.ux_device_class_hid_event_length = 8;
    hid_event.ux_device_class_hid_event_buffer[0] = 0x00;
    hid_event.ux_device_class_hid_event_buffer[1] = 0;
    hid_event.ux_device_class_hid_event_buffer[2] = UX_HID_LED_KEY_CAPS_LOCK;
    hid_event.ux_device_class_hid_event_buffer[3] = UX_HID_LED_KEY_SCROLL_LOCK;
    hid_event.ux_device_class_hid_event_buffer[4] = UX_HID_LED_KEY_NUM_LOCK;

    /* Add a different key so the host can receive it, allowing checking of key state. */
    hid_event.ux_device_class_hid_event_buffer[5] = 0x05;

    status = ux_device_class_hid_event_set(hid, &hid_event);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    hid_event.ux_device_class_hid_event_buffer[2] = 0;
    hid_event.ux_device_class_hid_event_buffer[3] = 0;
    hid_event.ux_device_class_hid_event_buffer[4] = 0;
    hid_event.ux_device_class_hid_event_buffer[5] = 0;

    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /**************************************************************************/
    /** 5. Test raw key.                                                     **/
    test_slave_phase ++;
    // printf("****** slave step: %ld ******\n", test_slave_phase);

    /* Wait decode mode change. */
    if (tx_demo_phase_sync(UX_FALSE, 20, SLAVE_WAIT_TIME) != UX_SUCCESS)
    {
        printf("Error in line %d, thread phase error %ld <> %ld!\n", __LINE__, test_host_phase, test_slave_phase);
        test_control_return(1);
    }

    /** Test raw key. **/

    for (i = KEY_START; i < ARRAY_COUNT(ux_host_class_hid_keyboard_regular_array); i++)
    {

        hid_event.ux_device_class_hid_event_length = 8;

        /* Modification byte. */
        hid_event.ux_device_class_hid_event_buffer[0] = 0;

        /* Reserved byte. */
        hid_event.ux_device_class_hid_event_buffer[1] = 0;

        /* Key byte. */
        hid_event.ux_device_class_hid_event_buffer[2] = i;

        status = ux_device_class_hid_event_set(hid, &hid_event);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        ux_utility_thread_sleep(SLAVE_WAIT_TIME);
    }
    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /**************************************************************************/
    /** 6. Test invalid key.                                                 **/
    test_slave_phase ++;
    // printf("****** slave step: %ld ******\n", test_slave_phase);

    /* Wait decode mode change. */
    if (tx_demo_phase_sync(UX_FALSE, 10, SLAVE_WAIT_TIME) != UX_SUCCESS)
    {
        printf("Error in line %d, thread phase error %ld <> %ld!\n", __LINE__, test_host_phase, test_slave_phase);
        test_control_return(1);
    }

    /* Key byte. */
    hid_event.ux_device_class_hid_event_buffer[2] = ARRAY_COUNT(ux_host_class_hid_keyboard_regular_array);

    status = ux_device_class_hid_event_set(hid, &hid_event);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    /* Key byte release. */
    hid_event.ux_device_class_hid_event_buffer[2] = 0;

    status = ux_device_class_hid_event_set(hid, &hid_event);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /**************************************************************************/
    /** 7. Test key array overflow.                                          **/
    test_slave_phase ++;
    // printf("****** slave step: %ld ******\n", test_slave_phase);

    /* Wait decode mode change. */
    if (tx_demo_phase_sync(UX_FALSE, 10, SLAVE_WAIT_TIME) != UX_SUCCESS)
    {
        printf("Error in line %d, thread phase error %ld <> %ld!\n", __LINE__, test_host_phase, test_slave_phase);
        test_control_return(1);
    }

    /** Test key array full. **/

    for (i = 0; i < UX_HOST_CLASS_HID_KEYBOARD_USAGE_ARRAY_LENGTH + 1; i++)
    {

        /* Press key.  */
        hid_event.ux_device_class_hid_event_length = 8;
        hid_event.ux_device_class_hid_event_buffer[0] = 0;
        hid_event.ux_device_class_hid_event_buffer[1] = 0;
        hid_event.ux_device_class_hid_event_buffer[2] = 5;
        status = ux_device_class_hid_event_set(hid, &hid_event);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        ux_utility_thread_sleep(SLAVE_WAIT_TIME);

        /* Release previous key.  */
        hid_event.ux_device_class_hid_event_length = 8;
        hid_event.ux_device_class_hid_event_buffer[0] = 0;
        hid_event.ux_device_class_hid_event_buffer[1] = 0;
        hid_event.ux_device_class_hid_event_buffer[2] = 0;
        status = ux_device_class_hid_event_set(hid, &hid_event);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        ux_utility_thread_sleep(SLAVE_WAIT_TIME);
    }
    ux_utility_thread_sleep(SLAVE_WAIT_TIME);

    /* All done. */
    test_slave_phase = 0;
}