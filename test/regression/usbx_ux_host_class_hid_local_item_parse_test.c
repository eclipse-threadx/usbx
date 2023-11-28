/* This test ensures that the parser generates an error upon encountering an invalid close delimiter tag. */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"

#include "ux_test.h"
#include "ux_test_hcd_sim_host.h"

#define TEST_SLAVE_REQUEST_CONTROL_MAX_LENGTH 8192

#define REP2_0(v0,v1)
#define REP2_1(v0,v1) v0,v1,
#define REP2_2(v0,v1) REP2_1(v0,v1) v0,v1,
#define REP2_3(v0,v1) REP2_2(v0,v1) v0,v1,
#define REP2_4(v0,v1) REP2_3(v0,v1) v0,v1,
#define REP2_5(v0,v1) REP2_4(v0,v1) v0,v1,
#define REP2_6(v0,v1) REP2_5(v0,v1) v0,v1,
#define REP2_7(v0,v1) REP2_6(v0,v1) v0,v1,
#define REP2_8(v0,v1) REP2_7(v0,v1) v0,v1,
#define REP2_9(v0,v1) REP2_8(v0,v1) v0,v1,
#define REP2_10(v0,v1) REP2_9(v0,v1) v0,v1,
#define REP2_20(v0,v1) REP2_10(v0,v1) REP2_10(v0,v1)
#define REP2_50(v0,v1) REP2_10(v0,v1) REP2_10(v0,v1) REP2_10(v0,v1) REP2_10(v0,v1) REP2_10(v0,v1)
#define REP2_100(v0,v1) REP2_50(v0,v1) REP2_50(v0,v1)
#define REP2_200(v0,v1) REP2_100(v0,v1) REP2_100(v0,v1)
#define REP2_500(v0,v1) REP2_100(v0,v1) REP2_100(v0,v1) REP2_100(v0,v1) REP2_100(v0,v1) REP2_100(v0,v1)
#define REP2_1000(v0,v1) REP2_500(v0,v1) REP2_500(v0,v1)

#define REP4_0(v0,v1,v2,v3)
#define REP4_1(v0,v1,v2,v3) v0,v1,v2,v3,
#define REP4_2(v0,v1,v2,v3) REP4_1(v0,v1,v2,v3) v0,v1,v2,v3,
#define REP4_3(v0,v1,v2,v3) REP4_2(v0,v1,v2,v3) v0,v1,v2,v3,
#define REP4_4(v0,v1,v2,v3) REP4_3(v0,v1,v2,v3) v0,v1,v2,v3,
#define REP4_5(v0,v1,v2,v3) REP4_4(v0,v1,v2,v3) v0,v1,v2,v3,
#define REP4_6(v0,v1,v2,v3) REP4_5(v0,v1,v2,v3) v0,v1,v2,v3,
#define REP4_7(v0,v1,v2,v3) REP4_6(v0,v1,v2,v3) v0,v1,v2,v3,
#define REP4_8(v0,v1,v2,v3) REP4_7(v0,v1,v2,v3) v0,v1,v2,v3,
#define REP4_9(v0,v1,v2,v3) REP4_8(v0,v1,v2,v3) v0,v1,v2,v3,
#define REP4_10(v0,v1,v2,v3) REP4_9(v0,v1,v2,v3) v0,v1,v2,v3,
#define REP4_20(v0,v1,v2,v3) REP4_10(v0,v1,v2,v3) REP4_10(v0,v1,v2,v3)
#define REP4_50(v0,v1,v2,v3) REP4_10(v0,v1,v2,v3) REP4_10(v0,v1,v2,v3) REP4_10(v0,v1,v2,v3) REP4_10(v0,v1,v2,v3) REP4_10(v0,v1,v2,v3)
#define REP4_100(v0,v1,v2,v3) REP4_50(v0,v1,v2,v3) REP4_50(v0,v1,v2,v3)
#define REP4_200(v0,v1,v2,v3) REP4_100(v0,v1,v2,v3) REP4_100(v0,v1,v2,v3)
#define REP4_500(v0,v1,v2,v3) REP4_100(v0,v1,v2,v3) REP4_100(v0,v1,v2,v3) REP4_100(v0,v1,v2,v3) REP4_100(v0,v1,v2,v3) REP4_100(v0,v1,v2,v3)
#define REP4_1000(v0,v1,v2,v3) REP4_500(v0,v1,v2,v3) REP4_500(v0,v1,v2,v3)

static UINT callback_error_code;

static UCHAR hid_report_descriptor[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0xa9, 0x01,                    //   DELIMITER (Open)
    0x0b, 0x20, 0x00, 0x05, 0x00,  //     USAGE (Gaming Controls:Point of View)
    0x0b, 0x39, 0x00, 0x01, 0x00,  //     USAGE (Generic Desktop:Hat switch)
    0x0b, 0x20, 0x00, 0x05, 0x00,  //     USAGE (Gaming Controls:Point of View)
    0xa9, 0x00,                    //   DELIMITER (Close)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x03,                    //   LOGICAL_MAXIMUM (3)
    0x35, 0x00,                    //   PHYSICAL_MINIMUM (0)
    0x46, 0x0e, 0x01,              //   PHYSICAL_MAXIMUM (270)
    0x65, 0x14,                    //   UNIT (Eng Rot:Angular Pos)
    0x55, 0x00,                    //   UNIT_EXPONENT (0)
    0x75, 0x04,                    //   REPORT_SIZE (4)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0xc0                           // END_COLLECTION
};
#define HID_REPORT_LENGTH sizeof(hid_report_descriptor)/sizeof(hid_report_descriptor[0])

#if UX_HOST_CLASS_HID_USAGES == 1024
static UCHAR hid_report_descriptor1[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0xa9, 0x01,                    //   DELIMITER (Open)
    REP4_5(0x19, 1, 0x29, 200)
    REP4_1(0x19, 1, 0x29,  23)
    REP2_5(0x09, 0x30)
    0x0b, 0x20, 0x00, 0x05, 0x00,  //     USAGE (Gaming Controls:Point of View)
    0x0b, 0x39, 0x00, 0x01, 0x00,  //     USAGE (Generic Desktop:Hat switch)
    0x0b, 0x20, 0x00, 0x05, 0x00,  //     USAGE (Gaming Controls:Point of View)
    0xa9, 0x00,                    //   DELIMITER (Close)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x03,                    //   LOGICAL_MAXIMUM (3)
    0x35, 0x00,                    //   PHYSICAL_MINIMUM (0)
    0x46, 0x0e, 0x01,              //   PHYSICAL_MAXIMUM (270)
    0x65, 0x14,                    //   UNIT (Eng Rot:Angular Pos)
    0x55, 0x00,                    //   UNIT_EXPONENT (0)
    0x75, 0x04,                    //   REPORT_SIZE (4)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0xc0                           // END_COLLECTION
};
#define HID_REPORT_LENGTH1 sizeof(hid_report_descriptor1)/sizeof(hid_report_descriptor1[0])

static UCHAR hid_report_descriptor2[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0xa9, 0x01,                    //   DELIMITER (Open)
    REP4_5(0x19, 1, 0x29, 200)
    REP4_1(0x19, 1, 0x29,  30)
    0x0b, 0x20, 0x00, 0x05, 0x00,  //     USAGE (Gaming Controls:Point of View)
    0x0b, 0x39, 0x00, 0x01, 0x00,  //     USAGE (Generic Desktop:Hat switch)
    0x0b, 0x20, 0x00, 0x05, 0x00,  //     USAGE (Gaming Controls:Point of View)
    0xa9, 0x00,                    //   DELIMITER (Close)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x03,                    //   LOGICAL_MAXIMUM (3)
    0x35, 0x00,                    //   PHYSICAL_MINIMUM (0)
    0x46, 0x0e, 0x01,              //   PHYSICAL_MAXIMUM (270)
    0x65, 0x14,                    //   UNIT (Eng Rot:Angular Pos)
    0x55, 0x00,                    //   UNIT_EXPONENT (0)
    0x75, 0x04,                    //   REPORT_SIZE (4)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0xc0                           // END_COLLECTION
};
#define HID_REPORT_LENGTH2 sizeof(hid_report_descriptor2)/sizeof(hid_report_descriptor2[0])
#else /* UX_HOST_CLASS_HID_USAGES == 1024 */
static inline void hid_usage_min_max_add(UCHAR *ptr, UCHAR min, UCHAR max)
{
    ptr[0] = 0x19;
    ptr[1] = min;
    ptr[2] = 0x29;
    ptr[3] = max;
}

static inline void hid_usage_add(UCHAR *ptr, UCHAR usage)
{
    ptr[0] = 0x09;
    ptr[1] = usage;
}

/* usage : 200 x N + M */
#define _DIV200(V) ((V)/200)
#define _MOD200(V) ((V)%200)
#define _P1_START  (0)
#define _P1_SIZE   (8)
#define _P2_START  (_P1_SIZE)
#define _P2_SIZE   (HID_REPORT_LENGTH - _P1_SIZE)
static UCHAR hid_report_descriptor_mem[HID_REPORT_LENGTH + _DIV200(UX_HOST_CLASS_HID_USAGES + 10) * 4 + 10 * 4];
static UCHAR *hid_report_descriptor1 = hid_report_descriptor_mem;
static UCHAR *hid_report_descriptor2 = hid_report_descriptor_mem;
#define HID_REPORT_LENGTH1 (HID_REPORT_LENGTH+_DIV200(UX_HOST_CLASS_HID_USAGES-1)*4+1*4+5*2)
#define HID_REPORT_LENGTH2 (HID_REPORT_LENGTH+_DIV200(UX_HOST_CLASS_HID_USAGES+5)*4+2*4)
static void hid_report_descriptor_generate(ULONG length)
{

UINT            i;
UCHAR           *ptr = hid_report_descriptor_mem;
UINT            nb_min_max0, nb_min_max1, nb_usage;


    _ux_utility_memory_copy(ptr, hid_report_descriptor + _P1_START, _P1_SIZE);
    ptr += _P1_SIZE;

    /* Build descriptor.  */
    switch(length)
    {
    case HID_REPORT_LENGTH1:
        nb_min_max0 = _DIV200(UX_HOST_CLASS_HID_USAGES-1);
        nb_min_max1 = 1;
        nb_usage    = 5;
        break;

    case HID_REPORT_LENGTH2:
        nb_min_max0 = _DIV200(UX_HOST_CLASS_HID_USAGES+5);
        nb_min_max1 = 2;
        nb_usage    = 0;
        break;
    
    default:
        nb_min_max0 = 0;
        nb_min_max1 = 0;
        nb_usage    = 0;
    }
    for (i = 0; i < nb_min_max0; i ++)
    {
        hid_usage_min_max_add(ptr, 1, 200);
        ptr += 4;
    }
    for (i = 0; i < nb_min_max1; i ++)
    {
        hid_usage_min_max_add(ptr, 1, _MOD200(UX_HOST_CLASS_HID_USAGES));
        ptr += 4;
    }
    for (i = 0; i < nb_usage; i ++)
    {
        hid_usage_add(ptr, 0x30);
        ptr += 2;
    }

    _ux_utility_memory_copy(ptr, hid_report_descriptor + _P2_START, _P2_SIZE);
    #if 0
    printf("N usage max: %d\n", UX_HOST_CLASS_HID_USAGES);
    printf("Len %ld or (%d, %d) - %d, %d, %d\n", length, HID_REPORT_LENGTH1, HID_REPORT_LENGTH2, nb_min_max0, nb_min_max1, nb_usage);
    printf("hid_report_descriptor:");
    for (i = 0; i < HID_REPORT_LENGTH; i ++)
    {
        if ((i & 3) == 0) printf("\n");
        printf(" %2x", hid_report_descriptor[i]);
    }
    printf("\n");
    printf("hid_report_descriptor_mem:");
    for (i = 0; i < length; i ++)
    {
        if ((i & 3) == 0) printf("\n");
        printf(" %2x", hid_report_descriptor_mem[i]);
    }
    printf("\n");
    #endif
}
#endif

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



static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    if (error_code != UX_HOST_CLASS_HID_USAGE_OVERFLOW &&
        error_code != UX_DEVICE_ENUMERATION_FAILURE &&
        error_code != UX_HOST_CLASS_INSTANCE_UNKNOWN &&
        error_code != UX_DESCRIPTOR_CORRUPTED)
    {

        /* Something went wrong.  */
        printf("Error on line %d: code 0x%x\n", __LINE__, error_code);
        test_control_return(1);
    }
}

static VOID set_hid_descriptor(UCHAR *descriptor, ULONG length)
{

UX_SLAVE_CLASS     *class;
UX_SLAVE_CLASS_HID *hid_class;


    device_framework_full_speed[DEVICE_FRAMEWORK_LENGTH_FULL_SPEED - 9] = LSB(length);
    device_framework_full_speed[DEVICE_FRAMEWORK_LENGTH_FULL_SPEED - 8] = MSB(length);
    device_framework_high_speed[DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED - 9] = LSB(length);
    device_framework_high_speed[DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED - 8] = MSB(length);

    /* Modify class settings.  */
    class = &_ux_system_slave->ux_system_slave_class_array[0];
    hid_class = (UX_SLAVE_CLASS_HID*)class->ux_slave_class_instance;

    hid_class->ux_device_class_hid_report_address = descriptor;
    hid_class->ux_device_class_hid_report_length  = length; 

#if UX_HOST_CLASS_HID_USAGES != 1024
    hid_report_descriptor_generate(length);
#endif
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_class_hid_local_item_parse_test_application_define(void *first_unused_memory)
#endif
{
    
UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
UINT                            descriptor_size = HID_REPORT_LENGTH;

    /* Inform user.  */
    printf("Running ux_host_class_hid_local_item_parse Test..................... ");
    stepinfo("\n");

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

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the hid class parameters.  */
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
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT status;

    stepinfo(">>>>>>>>>>> Test normal connect\n");

    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>> Test USAGE overflow\n");
    ux_test_hcd_sim_host_disconnect();
    set_hid_descriptor(hid_report_descriptor1, HID_REPORT_LENGTH1);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    status = demo_class_hid_get();
    if (status == UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>> Test USAGE_MAXIMUM overflow\n");
    ux_test_hcd_sim_host_disconnect();
    set_hid_descriptor(hid_report_descriptor2, HID_REPORT_LENGTH2);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    status = demo_class_hid_get();
    if (status == UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
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