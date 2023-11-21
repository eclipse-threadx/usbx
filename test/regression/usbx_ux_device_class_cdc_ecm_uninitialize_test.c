/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_network_driver.h"
#include "ux_host_class_cdc_ecm.h"
#include "ux_device_class_cdc_ecm.h"
#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"
#include "ux_test.h"

#define DEMO_IP_THREAD_STACK_SIZE           (8*1024)
#define HOST_IP_ADDRESS                     IP_ADDRESS(192,168,1,176)
#define HOST_SOCKET_PORT_UDP                    45054
#define DEVICE_IP_ADDRESS                   IP_ADDRESS(192,168,1,175)
#define DEVICE_SOCKET_PORT_UDP                  45055

#define PACKET_PAYLOAD                      1400
#define PACKET_POOL_SIZE                    (PACKET_PAYLOAD*20)
#define ARP_MEMORY_SIZE                     1024

/* Define local constants.  */

#define UX_DEMO_STACK_SIZE  (4*1024)
#define UX_USBX_MEMORY_SIZE (128*1024)

/* Host */

static CHAR                                 global_buffer_write_host[128];
static CHAR                                 global_buffer_read_host[128];
static ULONG                                global_basic_test_num_writes_host;
static ULONG                                global_basic_test_num_reads_host;
static UX_HOST_CLASS                        *class_driver_host;
static UX_HOST_CLASS_CDC_ECM                *cdc_ecm_host;
static TX_THREAD                            thread_host;
static UCHAR                                thread_stack_host[UX_DEMO_STACK_SIZE];
static NX_IP                                nx_ip_host;

/* Device */

static CHAR                                 global_buffer_write_device[128];
static CHAR                                 global_buffer_read_device[128];
static ULONG                                global_basic_test_num_writes_device;
static ULONG                                global_basic_test_num_reads_device;
static TX_THREAD                            thread_device;
static UX_HOST_CLASS                        *class_driver_device;
static UX_SLAVE_CLASS_CDC_ECM               *cdc_ecm_device;
static UX_SLAVE_CLASS_CDC_ECM_PARAMETER     cdc_ecm_parameter;
static UCHAR                                thread_stack_device[UX_DEMO_STACK_SIZE];
static NX_IP                                nx_ip_device;

/* Define local prototypes and definitions.  */
static void thread_entry_host(ULONG arg);
static void thread_entry_device(ULONG arg);

static UCHAR usbx_memory[UX_USBX_MEMORY_SIZE];

static unsigned char device_framework_high_speed[] = {

    /* Device Descriptor */
    0x12, /* bLength */
    0x01, /* bDescriptorType */
    0x10, 0x01, /* bcdUSB */
    0xef, /* bDeviceClass - Depends on bDeviceSubClass */
    0x02, /* bDeviceSubClass - Depends on bDeviceProtocol */
    0x01, /* bDeviceProtocol - There's an IAD */
    0x40, /* bMaxPacketSize0 */
    0x70, 0x07, /* idVendor */
    0x42, 0x10, /* idProduct */
    0x00, 0x01, /* bcdDevice */
    0x01, /* iManufacturer */
    0x02, /* iProduct */
    0x03, /* iSerialNumber */
    0x01, /* bNumConfigurations */

    /* Configuration Descriptor */
    0x09, /* bLength */
    0x02, /* bDescriptorType */

    0x58, 0x00, /* wTotalLength */
    0x02, /* bNumInterfaces */
    0x01, /* bConfigurationValue */
    0x00, /* iConfiguration */
    0xc0, /* bmAttributes - Self-powered */
    0x00, /* bMaxPower */

    /* Interface Association Descriptor */
    0x08, /* bLength */
    0x0b, /* bDescriptorType */
    0x00, /* bFirstInterface */
    0x02, /* bInterfaceCount */
    0x02, /* bFunctionClass - CDC - Communication */
    0x06, /* bFunctionSubClass - ECM */
    0x00, /* bFunctionProtocol - No class specific protocol required */
    0x00, /* iFunction */

    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x00, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x01, /* bNumEndpoints */
    0x02, /* bInterfaceClass - CDC - Communication */
    0x06, /* bInterfaceSubClass - ECM */
    0x00, /* bInterfaceProtocol - No class specific protocol required */
    0x00, /* iInterface */

    /* CDC Header Functional Descriptor */
    0x05, /* bLength */
    0x24, /* bDescriptorType */
    0x00, /* bDescriptorSubType */
    0x10, 0x01, /* bcdCDC */

    /* CDC ECM Functional Descriptor */
    0x0d, /* bLength */
    0x24, /* bDescriptorType */
    0x0f, /* bDescriptorSubType */
    0x04, /* iMACAddress */
    0x00, 0x00, 0x00, 0x00, /* bmEthernetStatistics */
    0xea, 0x05, /* wMaxSegmentSize */
    0x00, 0x00, /* wNumberMCFilters */
    0x00, /* bNumberPowerFilters */

    /* CDC Union Functional Descriptor */
    0x05, /* bLength */
    0x24, /* bDescriptorType */
    0x06, /* bDescriptorSubType */
    0x00, /* bmMasterInterface */
    0x01, /* bmSlaveInterface0 */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x83, /* bEndpointAddress */
    0x03, /* bmAttributes - Interrupt */
    0x08, 0x00, /* wMaxPacketSize */
    0x08, /* bInterval */

    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x01, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x00, /* bNumEndpoints */
    0x0a, /* bInterfaceClass - CDC - Data */
    0x00, /* bInterfaceSubClass - Should be 0x00 */
    0x00, /* bInterfaceProtocol - No class specific protocol required */
    0x00, /* iInterface */

    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x01, /* bInterfaceNumber */
    0x01, /* bAlternateSetting */
    0x02, /* bNumEndpoints */
    0x0a, /* bInterfaceClass - CDC - Data */
    0x00, /* bInterfaceSubClass - Should be 0x00 */
    0x00, /* bInterfaceProtocol - No class specific protocol required */
    0x00, /* iInterface */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x02, /* bEndpointAddress */
    0x02, /* bmAttributes - Bulk */
    0x40, 0x00, /* wMaxPacketSize */
    0x00, /* bInterval */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    0x05, /* bDescriptorType */
    0x81, /* bEndpointAddress */
    0x02, /* bmAttributes - Bulk */
    0x40, 0x00, /* wMaxPacketSize */
    0x00, /* bInterval */

};

static unsigned char string_framework[] = {

    /* Manufacturer string descriptor : Index 1 - "Express Logic" */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72, 0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 - "EL CDCECM Device" */
        0x09, 0x04, 0x02, 0x10,
        0x45, 0x4c, 0x20, 0x43, 0x44, 0x43, 0x45, 0x43,
        0x4d, 0x20, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65,

    /* Serial Number string descriptor : Index 3 - "0001" */
        0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31,

    /* MAC Address string descriptor : Index 4 - "001E5841B879" */
        0x09, 0x04, 0x04, 0x0C,
        0x30, 0x30, 0x31, 0x45, 0x35, 0x38,
        0x34, 0x31, 0x42, 0x38, 0x37, 0x39,

};

static unsigned char *device_framework_full_speed = device_framework_high_speed;
#define FRAMEWORK_LENGTH sizeof(device_framework_high_speed)

    /* Multiple languages are supported on the device, to add
       a language besides english, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
static unsigned char language_id_framework[] = {

    /* English. */
        0x09, 0x04
    };

/* Define local variables.  */

static void class_cdc_ecm_get_host(void)
{

UX_HOST_CLASS   *class;

    /* Find the main storage container */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_class_get(_ux_system_host_class_cdc_ecm_name, &class));

    /* We get the first instance of the storage device */
    UX_TEST_CHECK_SUCCESS(ux_test_host_stack_class_instance_get(class, 0, (void **) &cdc_ecm_host));

    /* We still need to wait for the cdc-ecm status to be live */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uint(&cdc_ecm_host -> ux_host_class_cdc_ecm_state, UX_HOST_CLASS_INSTANCE_LIVE));
}

static VOID demo_cdc_ecm_instance_activate(VOID *cdc_ecm_instance)
{

    /* Save the CDC instance.  */
    cdc_ecm_device = (UX_SLAVE_CLASS_CDC_ECM *) cdc_ecm_instance;
}

static VOID demo_cdc_ecm_instance_deactivate(VOID *cdc_ecm_instance)
{

    /* Reset the CDC instance.  */
    cdc_ecm_device = UX_NULL;
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_ux_device_class_cdc_ecm_uninitialize_test_application_define(void *first_unused_memory)
#endif
{

    /* Inform user.  */
    printf("Running ux_device_class_cdc_ecm_uninitialize Test................... ");

    stepinfo("\n");

    /* Initialize USBX Memory. */
    ux_system_initialize((CHAR *)usbx_memory, UX_USBX_MEMORY_SIZE, UX_NULL, 0);

    /* It looks weird if this doesn't have a comment! */
    ux_utility_error_callback_register(ux_test_error_callback);

    /* Perform the initialization of the network driver. */
    UX_TEST_CHECK_SUCCESS(ux_network_driver_init());

    nx_system_initialize();

    /* Create the slave thread. */
    UX_TEST_CHECK_SUCCESS(tx_thread_create(&thread_device, "device thread", thread_entry_device, 0,
                                           thread_stack_device, UX_DEMO_STACK_SIZE,
                                           30, 30, 1, TX_AUTO_START));
}

static void thread_entry_device(ULONG input)
{

ULONG                                   free_memory_pre_init;
ULONG                                   free_memory_post_uninit;
UX_SLAVE_CLASS                          *class;
UX_SLAVE_CLASS_CDC_ECM                  *cdc_ecm;
UX_SLAVE_CLASS_COMMAND                  command;

    /* The code below is required for installing the device portion of USBX.
       In this demo, DFU is possible and we have a call back for state change. */
    UX_TEST_CHECK_SUCCESS(ux_device_stack_initialize(device_framework_high_speed, FRAMEWORK_LENGTH,
                                                      device_framework_full_speed, FRAMEWORK_LENGTH,
                                                      string_framework, sizeof(string_framework),
                                                      language_id_framework, sizeof(language_id_framework),
                                                      UX_NULL));

    /* Set the parameters for callback when insertion/extraction of a CDC device.  Set to NULL.*/
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_instance_activate   =  demo_cdc_ecm_instance_activate;
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_instance_deactivate =  demo_cdc_ecm_instance_deactivate;

    /* Define a NODE ID.  */
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[0] = 0x00;
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[1] = 0x1e;
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[2] = 0x58;
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[3] = 0x41;
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[4] = 0xb8;
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_local_node_id[5] = 0x78;

    /* Define a remote NODE ID.  */
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[0] = 0x00;
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[1] = 0x1e;
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[2] = 0x58;
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[3] = 0x41;
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[4] = 0xb8;
    cdc_ecm_parameter.ux_slave_class_cdc_ecm_parameter_remote_node_id[5] = 0x79;

    /* Test. */
    {
        /* Save the amount of free memory before initing. */
        free_memory_pre_init = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

        /* Initialize the device cdc_ecm class. */
        UX_TEST_CHECK_SUCCESS(ux_device_stack_class_register(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry, 1, 0, &cdc_ecm_parameter));

        /* And deinitialize the class.  */
        UX_TEST_CHECK_SUCCESS(ux_device_stack_class_unregister(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry));

        /* Save the amount of free memory after uniniting. */
        free_memory_post_uninit = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

        /* Make sure they're the same.  */
        UX_TEST_ASSERT(free_memory_pre_init == free_memory_post_uninit);

    }

    /* Test: class instance freed before invoking uninitialize.  */
    {
        /* Save the amount of free memory before initing. */
        free_memory_pre_init = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

        /* Initialize the device cdc_ecm class. */
        UX_TEST_CHECK_SUCCESS(ux_device_stack_class_register(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry, 1, 0, &cdc_ecm_parameter));

        /* Obtain class driver.  */
        class = &_ux_system_slave -> ux_system_slave_class_array[0];

        /* Confirm class registered.  */
        UX_TEST_ASSERT(class -> ux_slave_class_status == UX_USED);
        UX_TEST_ASSERT(class -> ux_slave_class_instance != UX_NULL);

        /* Modify class instance for test.  */
        cdc_ecm = (UX_SLAVE_CLASS_CDC_ECM *)class -> ux_slave_class_instance;
        class -> ux_slave_class_instance = UX_NULL;

        /* And deinitialize the class, unregister is not used since it may change class state.  */
        command.ux_slave_class_command_request    =  UX_SLAVE_CLASS_COMMAND_UNINITIALIZE;
        command.ux_slave_class_command_class_ptr  =  class;
        UX_TEST_CHECK_SUCCESS(ux_device_class_cdc_ecm_entry(&command));

        /* Memory still valid. */
        // UX_TEST_ASSERT(cdc_ecm -> ux_slave_class_cdc_ecm_pool_memory != UX_NULL);

        /* Restore normal data.  */
        class -> ux_slave_class_instance = (VOID*)cdc_ecm;

        /* And deinitialize the class.  */
        UX_TEST_CHECK_SUCCESS(ux_device_stack_class_unregister(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry));

        /* Save the amount of free memory after uniniting. */
        free_memory_post_uninit = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

        /* Make sure they're the same.  */
        UX_TEST_ASSERT(free_memory_pre_init == free_memory_post_uninit);
    }

    /* Deinitialize the device side of usbx.  */
    UX_TEST_CHECK_SUCCESS(_ux_device_stack_uninitialize());

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}
