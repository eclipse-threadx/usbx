/* This file tests the
 * _ux_device_class_hid_control_request
 * _ux_device_class_hid_interrupt_thread
 */

#include "usbx_test_common_hid.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

#include "ux_host_class_hid_mouse.h"
#include "ux_host_class_hid_keyboard.h"

#define DUMMY_USBX_MEMORY_SIZE          (64*1024)

static UCHAR                            dummy_usbx_memory[DUMMY_USBX_MEMORY_SIZE];

static UX_SLAVE_CLASS_HID               *slave_hid = UX_NULL;
static UX_SLAVE_CLASS_HID_EVENT         slave_hid_event;

#define DEMO_PACKET_SIZE                8
#define DEVICE_BUFFER_LENGTH            32 /* 4*8  */ + 1

static TX_SEMAPHORE                     device_semaphore;
static UCHAR                            device_buffer[DEVICE_BUFFER_LENGTH];
static UINT                             device_read_status;
static ULONG                            device_read_request_length;
static ULONG                            device_read_actual_length;
static ULONG                            device_read_count;

#define HOST_BUFFER_LENGTH              32 /* 4*8  */

static UCHAR                            host_buffer[HOST_BUFFER_LENGTH];
static ULONG                            host_buffer_length;

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

/* Configuration descriptor 9 bytes */
#define CFG_DESC(wTotalLength, bNumInterfaces, bConfigurationValue)\
    /* Configuration 1 descriptor 9 bytes */\
    0x09, 0x02, LSB(wTotalLength), MSB(wTotalLength),\
    (bNumInterfaces), (bConfigurationValue), 0x00,\
    0x40, 0x00,
#define CFG_DESC_LEN 9

/* HID Mouse interface descriptors 9+9+7=25 bytes */
#define HID_MOUSE_IFC_DESC_ALL(ifc, interrupt_epa)     \
    /* Interface descriptor */\
    0x09, 0x04, (ifc), 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,\
    /* HID descriptor */\
    0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_REPORT_LENGTH),\
    MSB(HID_REPORT_LENGTH),\
    /* Endpoint descriptor (Interrupt) */\
    0x07, 0x05, (interrupt_epa), 0x03, 0x08, 0x00, 0x08,
#define HID_MOUSE_IFC_DESC_ALL_LEN 25

/* HID Test interface descriptors 9+9+7+7=32 bytes */
#define HID_TEST_IFC_DESC_ALL(ifc, epa0, epa0_type, epa1, epa1_type)     \
    /* Interface descriptor */\
    0x09, 0x04, (ifc), 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,\
    /* HID descriptor */\
    0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_REPORT_LENGTH),\
    MSB(HID_REPORT_LENGTH),\
    /* Endpoint descriptor */\
    0x07, 0x05, (epa0), (epa0_type), 0x08, 0x00, 0x08,\
    /* Endpoint descriptor */\
    0x07, 0x05, (epa1), (epa1_type), 0x08, 0x00, 0x08,
#define HID_TEST_IFC_DESC_ALL_LEN 32

static UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
    0x81, 0x0A, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01,

    CFG_DESC(CFG_DESC_LEN+HID_TEST_IFC_DESC_ALL_LEN, 1, 1)
    /* Interrupt IN @ 1st */
    HID_TEST_IFC_DESC_ALL(0, 0x81, 3, 0x02, 3)
};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)


static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
    0x03, 0x01,

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x01, 0x00,

    CFG_DESC(CFG_DESC_LEN+HID_MOUSE_IFC_DESC_ALL_LEN, 1, 1)
    /* Interrupt IN @ 1st */
    HID_TEST_IFC_DESC_ALL(0, 0x81, 3, 0x02, 3)
};
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)


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

static VOID instance_activate_callback(VOID *parameter)
{

    slave_hid = (UX_SLAVE_CLASS_HID *)parameter;
}

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    // printf("Error on line %d: 0x%x, 0x%x, 0x%x\n", __LINE__, system_level, system_context, error_code);
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_class_hid_read_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;


    /* Inform user.  */
    printf("Running ux_device_class_hid_ read/interrupt OUT tests .............. ");
    stepinfo("\n");

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
    hid_parameter.ux_slave_class_hid_instance_activate         = instance_activate_callback;

    /* Initialize the device hid class. The class is connected with interface 2 */
    status  = ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                             1,0, (VOID *)&hid_parameter);
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

    /* Create the main device simulation thread.  */
    stack_pointer += UX_DEMO_STACK_SIZE;
    status =  tx_thread_create(&tx_demo_thread_device_simulation, "tx demo device simulation", tx_demo_thread_device_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR $%d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
}


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                status;
UX_DEVICE                           *device;
UX_ENDPOINT                         *endpoint;

    stepinfo(">>>>>>>>>> Thread start\n");

    _ux_utility_delay_ms(500);

    /* Get device instance. */
    status = ux_host_stack_device_get(0, &device);

    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: get_device fail, 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>> Get HID class instance\n");

    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }
    if (slave_hid == UX_NULL)
    {
        printf("Error on line %d, HID slave instance error\n", __LINE__);
        test_control_return(1);
    }

#if defined(UX_DEVICE_CLASS_HID_INTERRUPT_OUT_SUPPORT)
    endpoint = hid -> ux_host_class_hid_interface -> ux_interface_first_endpoint;
    if (endpoint -> ux_endpoint_descriptor.bEndpointAddress & 0x80)
        endpoint = endpoint -> ux_endpoint_next_endpoint;
    if (endpoint == UX_NULL)
    {
        printf("ERROR #%d: endpoint OUT not found\n", __LINE__);
        test_control_return(1);
    }
    endpoint->ux_endpoint_transfer_request.ux_transfer_request_timeout_value = 100;
    /* Device read 8, host send 0.  */
    device_read_request_length = 8;
    device_read_count = 0;
    tx_semaphore_put(&device_semaphore);
    status = ux_test_host_endpoint_write(endpoint, host_buffer, 0, UX_NULL);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(device_read_count == 1);
    UX_TEST_ASSERT(device_read_status == UX_SUCCESS);
    UX_TEST_ASSERT(device_read_actual_length == 0);
    /* Device read 8, host send 1.  */
    tx_semaphore_put(&device_semaphore);
    ux_utility_memory_set(host_buffer, 0x5A, sizeof(host_buffer));
    ux_utility_memory_set(device_buffer, 0x37, sizeof(device_buffer));
    status = ux_test_host_endpoint_write(endpoint, host_buffer, 1, UX_NULL);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(device_read_count == 2);
    UX_TEST_ASSERT(device_read_status == UX_SUCCESS);
    UX_TEST_ASSERT(device_read_actual_length == 1);
    UX_TEST_ASSERT(device_buffer[0] == host_buffer[0]);
    UX_TEST_ASSERT(device_buffer[1] == 0X37);
    /* Device read 8, host send 8.  */
    tx_semaphore_put(&device_semaphore);
    ux_utility_memory_set(host_buffer, 0x5A, sizeof(host_buffer));
    ux_utility_memory_set(device_buffer, 0x37, sizeof(device_buffer));
    status = ux_test_host_endpoint_write(endpoint, host_buffer, 8, UX_NULL);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(device_read_count == 3);
    UX_TEST_ASSERT(device_read_status == UX_SUCCESS);
    UX_TEST_ASSERT(device_read_actual_length == 8);
    UX_TEST_ASSERT(device_buffer[0] == host_buffer[0]);
    UX_TEST_ASSERT(device_buffer[7] == host_buffer[7]);
    UX_TEST_ASSERT(device_buffer[8] == 0X37);
    /* Device read 32, host send 8, then 3.  */
    device_read_request_length = 32;
    tx_semaphore_put(&device_semaphore);
    ux_utility_memory_set(host_buffer, 0x5A, sizeof(host_buffer));
    ux_utility_memory_set(device_buffer, 0x37, sizeof(device_buffer));
    status = ux_test_host_endpoint_write(endpoint, host_buffer, 8, UX_NULL);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(device_read_count == 3);
    status = ux_test_host_endpoint_write(endpoint, host_buffer, 3, UX_NULL);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(device_read_count == 4);
    UX_TEST_ASSERT(device_read_status == UX_SUCCESS);
    UX_TEST_ASSERT(device_read_actual_length == 11);
    UX_TEST_ASSERT(device_buffer[0] == host_buffer[0]);
    UX_TEST_ASSERT(device_buffer[10] == host_buffer[10]);
    UX_TEST_ASSERT(device_buffer[11] == 0X37);
#endif

    _ux_utility_delay_ms(500);

    stepinfo(">>>>>>>>>> Test done\n");

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


static void  tx_demo_thread_device_simulation_entry(ULONG arg)
{

UINT                    status;

    status = tx_semaphore_create(&device_semaphore, "device semaphore", 0);
    if (status != TX_SUCCESS)
    {
        printf("ERROR #%d device semaphore creation error 0x%x\n", __LINE__, status);
        test_control_return(1);
        return;
    }

    while(1)
    {
        if (slave_hid == UX_NULL)
        {
            tx_thread_sleep(10);
            continue;
        }
        /* Wait semaphore to start read.  */
        status = tx_semaphore_get(&device_semaphore, TX_WAIT_FOREVER);
        if (status != TX_SUCCESS)
        {
            printf("ERROR #%d device semaphore get error 0x%x\n", __LINE__, status);
            test_control_return(1);
            return;
        }
        /* Read and save status.  */
#if defined(UX_DEVICE_STANDALONE)
        do
        {
            ux_system_tasks_run();
            status = ux_device_class_hid_read_run(slave_hid, device_buffer, device_read_request_length, &device_read_actual_length);
        } while(status == UX_STATE_WAIT);
        status = (status == UX_STATE_NEXT) ? UX_SUCCESS : UX_ERROR;
#else
        status = ux_device_class_hid_read(slave_hid, device_buffer, device_read_request_length, &device_read_actual_length);
#endif
        device_read_status = status;
        device_read_count ++;
    }
}


static UINT    demo_thread_hid_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{
    return(UX_SUCCESS);
}
