#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_network_driver.h"
#include "ux_host_class_cdc_ecm.h"
#include "ux_device_class_rndis.h"
#include "ux_device_class_cdc_ecm.h"
#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"
#include "ux_test.h"
#include "ux_hcd_sim_host.h"
#include "ux_dcd_sim_slave.h"

#define DEMO_IP_THREAD_STACK_SIZE           (8*1024)
#define HOST_IP_ADDRESS                     IP_ADDRESS(192,168,1,176)
#define HOST_SOCKET_PORT_UDP                    45054
#define DEVICE_IP_ADDRESS                   IP_ADDRESS(192,168,1,175)
#define DEVICE_SOCKET_PORT_UDP                  45055

#define PACKET_PAYLOAD                      1400
#define PACKET_POOL_SIZE                    (PACKET_PAYLOAD*10000)
#define ARP_MEMORY_SIZE                     1024

/* Define local constants.  */

#define UX_DEMO_STACK_SIZE                  (4*1024)
#define UX_USBX_MEMORY_SIZE                 (128*1024)

/* Host */

static UX_HOST_CLASS                        *class_driver_host;
static UX_HOST_CLASS_CDC_ECM                *cdc_ecm_host;
static UX_HOST_CLASS_CDC_ECM                **cdc_ecm_host_ptr;
static TX_THREAD                            thread_host;
static UCHAR                                thread_stack_host[UX_DEMO_STACK_SIZE];
static NX_IP                                nx_ip_host;
static NX_PACKET_POOL                       packet_pool_host;
static NX_UDP_SOCKET                        udp_socket_host;
static CHAR                                 *packet_pool_memory_host;
static CHAR                                 ip_thread_stack_host[DEMO_IP_THREAD_STACK_SIZE];
static CHAR                                 arp_memory_host[ARP_MEMORY_SIZE];

/* Device */

static TX_THREAD                            thread_device;
static UX_HOST_CLASS                        *class_driver_device;
static UX_SLAVE_CLASS_RNDIS                 *rndis_device;
static UX_SLAVE_CLASS_RNDIS_PARAMETER       rndis_parameter;
static UCHAR                                thread_stack_device[UX_DEMO_STACK_SIZE];
static NX_IP                                nx_ip_device;
static NX_PACKET_POOL                       packet_pool_device;
static NX_UDP_SOCKET                        udp_socket_device;
static CHAR                                 *packet_pool_memory_device;
static CHAR                                 ip_thread_stack_device[DEMO_IP_THREAD_STACK_SIZE];
static CHAR                                 arp_memory_device[ARP_MEMORY_SIZE];

static UCHAR                                global_is_device_initialized;

static ULONG global_basic_test_num_writes_host;
static ULONG global_basic_test_num_reads_host;

static ULONG global_basic_test_num_writes_device;
static ULONG global_basic_test_num_reads_device;

/* Define local prototypes and definitions.  */
static void thread_entry_host(ULONG arg);
static void thread_entry_device(ULONG arg);

//#define USE_ZERO_ENDPOINT_SETTING

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
    
#ifdef USE_ZERO_ENDPOINT_SETTING
    0x58, 0x00, /* wTotalLength */
#else
    0x4f, 0x00, /* wTotalLength */
#endif
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

#ifdef USE_ZERO_ENDPOINT_SETTING
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
#else
    /* Interface Descriptor */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x01, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x02, /* bNumEndpoints */
    0x0a, /* bInterfaceClass - CDC - Data */
    0x00, /* bInterfaceSubClass - Should be 0x00 */
    0x00, /* bInterfaceProtocol - No class specific protocol required */
    0x00, /* iInterface */
#endif

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

static UINT class_cdc_ecm_get_host(void)
{

UX_HOST_CLASS   *class;
UINT            status;

    /* Find the main storage container */
    status =  ux_host_stack_class_get(_ux_system_host_class_cdc_ecm_name, &class);
    if (status != UX_SUCCESS)
        test_control_return(0);

    /* We get the first instance of the storage device */
    do
    {
        status =  ux_host_stack_class_instance_get(class, 0, (void **) &cdc_ecm_host);
        tx_thread_sleep(10);
    } while (status != UX_SUCCESS);

    /* We still need to wait for the cdc-ecm status to be live */
    while (cdc_ecm_host -> ux_host_class_cdc_ecm_state != UX_HOST_CLASS_INSTANCE_LIVE)
        tx_thread_sleep(10);

    return(UX_SUCCESS);
}

static VOID demo_rndis_instance_activate(VOID *rndis_instance)
{

    /* Save the CDC instance.  */
    rndis_device = (UX_SLAVE_CLASS_RNDIS *) rndis_instance;
}

static VOID demo_rndis_instance_deactivate(VOID *rndis_instance)
{

    /* Reset the CDC instance.  */
    rndis_device = UX_NULL;
}

static void read_packet_udp(NX_UDP_SOCKET *udp_socket, ULONG num_reads, CHAR *name)
{

NX_PACKET 	*rcv_packet;
ULONG       num_writes_from_peer;

#ifndef LOCAL_MACHINE
    if (num_reads % 100 == 0)
#endif
        stepinfo("%s reading packet# %lu\n", name, num_reads);

    UX_TEST_CHECK_SUCCESS(nx_udp_socket_receive(udp_socket, &rcv_packet, NX_WAIT_FOREVER));

    num_writes_from_peer = *(ULONG *)rcv_packet->nx_packet_prepend_ptr;
    if (num_writes_from_peer != num_reads)
        test_control_return(0);

    UX_TEST_CHECK_SUCCESS(nx_packet_release(rcv_packet));
}

static void write_packet_udp(NX_UDP_SOCKET *udp_socket, NX_PACKET_POOL *packet_pool, ULONG ip_address, ULONG port, ULONG num_writes, CHAR *name)
{

NX_PACKET 	*out_packet;

    UX_TEST_CHECK_SUCCESS(nx_packet_allocate(packet_pool, &out_packet, NX_UDP_PACKET, MS_TO_TICK(1000)));

    *(ULONG *)out_packet->nx_packet_prepend_ptr = num_writes;
    out_packet->nx_packet_length = sizeof(ULONG);
    out_packet->nx_packet_append_ptr = out_packet->nx_packet_prepend_ptr + out_packet->nx_packet_length;

#ifndef LOCAL_MACHINE
    if (num_writes % 100 == 0)
#endif
        stepinfo("%s writing packet# %lu\n", name, num_writes);

    UX_TEST_CHECK_SUCCESS(nx_udp_socket_send(udp_socket, out_packet, ip_address, port));
}

/* Define what the initial system looks like.  */
#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_rndis_basic_test_application_define(void *first_unused_memory)
#endif
{

CHAR *memory_pointer = first_unused_memory;

    /* Inform user.  */
    printf("Running RNDIS Basic Functionality Test.............................. ");

    stepinfo("\n");

    /* Initialize USBX Memory. */
    UX_TEST_CHECK_SUCCESS(ux_system_initialize(memory_pointer, UX_USBX_MEMORY_SIZE, UX_NULL, 0));
    memory_pointer += UX_USBX_MEMORY_SIZE;

    /* It looks weird if this doesn't have a comment! */
    ux_utility_error_callback_register(ux_test_error_callback);

    /* Perform the initialization of the network driver. */
    UX_TEST_CHECK_SUCCESS(ux_network_driver_init());

    /* Initialize the NetX system. */
    nx_system_initialize();

    /* Now allocate memory for the packet pools. Note that using the memory passed
       to us by ThreadX is mucho bettero than putting it in global memory because
       we can reuse the memory for each test. So no more having to worry about
       running out of memory! */
    packet_pool_memory_host = memory_pointer;
    memory_pointer += PACKET_POOL_SIZE;
    packet_pool_memory_device = memory_pointer;
    memory_pointer += PACKET_POOL_SIZE;

    /* Create the host thread. */
    UX_TEST_CHECK_SUCCESS(tx_thread_create(&thread_host, "host thread", thread_entry_host, 0,
                                           thread_stack_host, UX_DEMO_STACK_SIZE,
                                           30, 30, 1, TX_AUTO_START));

    /* Create the slave thread. */
    UX_TEST_CHECK_SUCCESS(tx_thread_create(&thread_device, "device thread", thread_entry_device, 0,
                                           thread_stack_device, UX_DEMO_STACK_SIZE,
                                           30, 30, 1, TX_AUTO_START));
}

/* Needs to be large enough to hold NetX packet data and RNDIS header. */
static UCHAR host_bulk_endpoint_transfer_data[16*1024];

static UINT  my_ux_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter)
{

UX_TRANSFER *transfer_request;
UX_ENDPOINT *endpoint;


    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;
        endpoint = transfer_request->ux_transfer_request_endpoint;

        /* Bulk out? */
        if ((endpoint->ux_endpoint_descriptor.bmAttributes == 0x02) &&
            (endpoint->ux_endpoint_descriptor.bEndpointAddress & 0x80) == 0)
        {

            UX_TEST_ASSERT(transfer_request->ux_transfer_request_requested_length + UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH <= sizeof(host_bulk_endpoint_transfer_data));

            /* Fix it, now! - we need to add the RNDIS header. */

            /* Copy that packet payload. */
            memcpy(host_bulk_endpoint_transfer_data + UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH, 
                   transfer_request->ux_transfer_request_data_pointer, 
                   transfer_request->ux_transfer_request_requested_length);

            /* Add the RNDIS header to this packet.  */

            _ux_utility_long_put(host_bulk_endpoint_transfer_data + UX_DEVICE_CLASS_RNDIS_PACKET_MESSAGE_TYPE, UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_MSG);

            _ux_utility_long_put(host_bulk_endpoint_transfer_data + UX_DEVICE_CLASS_RNDIS_PACKET_MESSAGE_LENGTH, 
                                 transfer_request->ux_transfer_request_requested_length + UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH);

            _ux_utility_long_put(host_bulk_endpoint_transfer_data + UX_DEVICE_CLASS_RNDIS_PACKET_DATA_OFFSET,
                                 UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH - UX_DEVICE_CLASS_RNDIS_PACKET_DATA_OFFSET);

            _ux_utility_long_put(host_bulk_endpoint_transfer_data + UX_DEVICE_CLASS_RNDIS_PACKET_DATA_LENGTH, 
                                 transfer_request->ux_transfer_request_requested_length);

            /* The original data pointer points to the packet, so no leak. We also
               only allow one transfer at a time, so no worries with overriding data. */
            transfer_request->ux_transfer_request_data_pointer = host_bulk_endpoint_transfer_data;
            transfer_request->ux_transfer_request_requested_length += UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH;
        }
    }

    return _ux_hcd_sim_host_entry(hcd, function, parameter);
}

static UINT  my_ux_dcd_sim_slave_function(UX_SLAVE_DCD *dcd, UINT function, VOID *parameter)
{

UX_SLAVE_TRANSFER   *transfer_request;
UX_SLAVE_ENDPOINT   *endpoint;
UINT                netx_packet_length;
UINT                i;


    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;
        endpoint = transfer_request->ux_slave_transfer_request_endpoint;

        /* Bulk in? */
        if ((endpoint->ux_slave_endpoint_descriptor.bmAttributes == 0x02) &&
            (endpoint->ux_slave_endpoint_descriptor.bEndpointAddress & 0x80) != 0)
        {

            /* Fix it, now! - we need to remove the RNDIS header. */

            netx_packet_length = transfer_request->ux_slave_transfer_request_requested_length - UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH;

            /* Just shift the packet over the RNDIS header. */
            for (i = 0; i < netx_packet_length; i++)
            {

                transfer_request->ux_slave_transfer_request_data_pointer[i] = transfer_request->ux_slave_transfer_request_data_pointer[i + UX_DEVICE_CLASS_RNDIS_PACKET_HEADER_LENGTH];
            }

            transfer_request->ux_slave_transfer_request_requested_length = netx_packet_length;
        }
    }

    return _ux_dcd_sim_slave_function(dcd, function, parameter);
}

static void thread_entry_host(ULONG input)
{

UINT 		    i;
UINT 		    num_iters;

    /* Wait for device to initialize before starting the HCD thread; also, there
       seems to be some race condition with simultaneous NetX initialization:
       somehow, device was calling the host CDC-ECM write. */
    while (!global_is_device_initialized)
        tx_thread_sleep(10);

    /* Wait for device to initialize. */
    while (!global_is_device_initialized)
        tx_thread_sleep(10);

    /* Create the IP instance. */

    UX_TEST_CHECK_SUCCESS(nx_packet_pool_create(&packet_pool_host, "NetX Host Packet Pool", PACKET_PAYLOAD, packet_pool_memory_host, PACKET_POOL_SIZE));
    UX_TEST_CHECK_SUCCESS(nx_ip_create(&nx_ip_host, "NetX Host Thread", HOST_IP_ADDRESS, 0xFF000000UL,
                          &packet_pool_host, _ux_network_driver_entry, ip_thread_stack_host, DEMO_IP_THREAD_STACK_SIZE, 1));

    /* Setup ARP. */

    UX_TEST_CHECK_SUCCESS(nx_arp_enable(&nx_ip_host, (void *)arp_memory_host, ARP_MEMORY_SIZE));
    UX_TEST_CHECK_SUCCESS(nx_arp_static_entry_create(&nx_ip_host, DEVICE_IP_ADDRESS, 0x0000001E, 0x80032CD8));

    /* Setup UDP. */

    UX_TEST_CHECK_SUCCESS(nx_udp_enable(&nx_ip_host));
    UX_TEST_CHECK_SUCCESS(nx_udp_socket_create(&nx_ip_host, &udp_socket_host, "USB HOST UDP SOCKET", NX_IP_NORMAL, NX_DONT_FRAGMENT, 20, 20));
    UX_TEST_CHECK_SUCCESS(nx_udp_socket_bind(&udp_socket_host, HOST_SOCKET_PORT_UDP, NX_NO_WAIT));

    /* The code below is required for installing the host portion of USBX. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_initialize(UX_NULL));

    /* Register cdc_ecm class.  */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_class_register(_ux_system_host_class_cdc_ecm_name, ux_host_class_cdc_ecm_entry));

    ux_test_ignore_all_errors();

    /* Register all the USB host controllers available in this system. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize, 0, 0));

    /* Change entry function.  */
    _ux_system_host->ux_system_host_hcd_array[0].ux_hcd_entry_function = my_ux_hcd_sim_host_entry;

    /* Find the storage class. */
    class_cdc_ecm_get_host();

	/* Now wait for the link to be up.  */
    while (cdc_ecm_host -> ux_host_class_cdc_ecm_link_state != UX_HOST_CLASS_CDC_ECM_LINK_STATE_UP)
        tx_thread_sleep(10);

    for (num_iters = 0; num_iters < 100; num_iters++)
    {

        for (i = 0; i < 10; i++)
            write_packet_udp(&udp_socket_host, &packet_pool_host, DEVICE_IP_ADDRESS, DEVICE_SOCKET_PORT_UDP, global_basic_test_num_writes_host++, "host");

        for (i = 0; i < 10; i++)
            read_packet_udp(&udp_socket_host, global_basic_test_num_reads_host++, "host");
    }

    /* Wait for all transfers to complete. */
    while (global_basic_test_num_reads_host != 1000 || global_basic_test_num_reads_device != 1000)
        tx_thread_sleep(10);

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}

static void thread_entry_device(ULONG input)
{

UINT                i;
UINT 		        status;
UINT                num_iters;
UCHAR               *notification_buffer;
UX_SLAVE_TRANSFER   *interrupt_transfer;

    /* Create the IP instance.  */

    UX_TEST_CHECK_SUCCESS(nx_packet_pool_create(&packet_pool_device, "NetX Device Packet Pool", PACKET_PAYLOAD, packet_pool_memory_device, PACKET_POOL_SIZE));

    UX_TEST_CHECK_SUCCESS(nx_ip_create(&nx_ip_device, "NetX Device Thread", DEVICE_IP_ADDRESS, 0xFF000000L, &packet_pool_device, 
                                       _ux_network_driver_entry, ip_thread_stack_device, DEMO_IP_THREAD_STACK_SIZE, 1));

    /* Setup ARP.  */

    UX_TEST_CHECK_SUCCESS(nx_arp_enable(&nx_ip_device, (void *)arp_memory_device, ARP_MEMORY_SIZE));
    UX_TEST_CHECK_SUCCESS(nx_arp_static_entry_create(&nx_ip_device, HOST_IP_ADDRESS, 0x0000001E, 0x5841B878));

    /* Setup UDP.  */

    UX_TEST_CHECK_SUCCESS(nx_udp_enable(&nx_ip_device));
    UX_TEST_CHECK_SUCCESS(nx_udp_socket_create(&nx_ip_device, &udp_socket_device, "USB DEVICE UDP SOCKET", NX_IP_NORMAL, NX_DONT_FRAGMENT, 20, 20));
    UX_TEST_CHECK_SUCCESS(nx_udp_socket_bind(&udp_socket_device, DEVICE_SOCKET_PORT_UDP, NX_NO_WAIT));

    /* The code below is required for installing the device portion of USBX. */
    status = ux_device_stack_initialize(device_framework_high_speed, FRAMEWORK_LENGTH,
                                        device_framework_full_speed, FRAMEWORK_LENGTH,
                                        string_framework, sizeof(string_framework),
                                        language_id_framework, sizeof(language_id_framework),
                                        UX_NULL);
    if (status)
        test_control_return(0);

    /* Set the parameters for callback when insertion/extraction of a CDC device. */
    rndis_parameter.ux_slave_class_rndis_instance_activate   =  demo_rndis_instance_activate;
    rndis_parameter.ux_slave_class_rndis_instance_deactivate =  demo_rndis_instance_deactivate;
    
    /* Define a local NODE ID.  */
    rndis_parameter.ux_slave_class_rndis_parameter_local_node_id[0] = 0x00;
    rndis_parameter.ux_slave_class_rndis_parameter_local_node_id[1] = 0x1e;
    rndis_parameter.ux_slave_class_rndis_parameter_local_node_id[2] = 0x58;
    rndis_parameter.ux_slave_class_rndis_parameter_local_node_id[3] = 0x41;
    rndis_parameter.ux_slave_class_rndis_parameter_local_node_id[4] = 0xb8;
    rndis_parameter.ux_slave_class_rndis_parameter_local_node_id[5] = 0x78;

    /* Define a remote NODE ID.  */
    rndis_parameter.ux_slave_class_rndis_parameter_remote_node_id[0] = 0x00;
    rndis_parameter.ux_slave_class_rndis_parameter_remote_node_id[1] = 0x1e;
    rndis_parameter.ux_slave_class_rndis_parameter_remote_node_id[2] = 0x58;
    rndis_parameter.ux_slave_class_rndis_parameter_remote_node_id[3] = 0x41;
    rndis_parameter.ux_slave_class_rndis_parameter_remote_node_id[4] = 0xb8;
    rndis_parameter.ux_slave_class_rndis_parameter_remote_node_id[5] = 0x79;

    /* Set extra parameters used by the RNDIS query command with certain OIDs.  */
    rndis_parameter.ux_slave_class_rndis_parameter_vendor_id          =  0x04b4 ;
    rndis_parameter.ux_slave_class_rndis_parameter_driver_version     =  0x1127;
    ux_utility_memory_copy(rndis_parameter.ux_slave_class_rndis_parameter_vendor_description, "ELOGIC RNDIS", 12);

    /* Initialize the device rndis class. This class owns both interfaces. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_rndis_name, ux_device_class_rndis_entry, 1, 0, &rndis_parameter);
    if (status)
        test_control_return(0);

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();
    if (status)
        test_control_return(0);

    _ux_system_slave->ux_system_slave_dcd.ux_slave_dcd_function = my_ux_dcd_sim_slave_function;

    global_is_device_initialized = UX_TRUE;

    while (!rndis_device)
        tx_thread_sleep(10);

    while (rndis_device -> ux_slave_class_rndis_link_state != UX_DEVICE_CLASS_RNDIS_LINK_STATE_UP)
        tx_thread_sleep(10);

    /* Since host is CDC-ECM, it's waiting for the LINK_UP notification from the
       interrupt endpoint. RNDIS does not send this, so we have to do it manually. */
    {
        interrupt_transfer = &rndis_device->ux_slave_class_rndis_interrupt_endpoint->ux_slave_endpoint_transfer_request;

        /* Build the Network Notification response.  */
        notification_buffer = interrupt_transfer->ux_slave_transfer_request_data_pointer;

        /* Set the request type.  */
        *(notification_buffer + UX_SETUP_REQUEST_TYPE) = UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;

        /* Set the request itself.  */
        *(notification_buffer + UX_SETUP_REQUEST) = 0;
        
        /* Set the value. It is the network link.  */
        _ux_utility_short_put(notification_buffer + UX_SETUP_VALUE, (USHORT)(rndis_device->ux_slave_class_rndis_link_state));

        /* Set the Index. It is interface.  The interface used is the DATA interface. Here we simply take the interface number of the CONTROL and add 1 to it
           as it is assumed the classes are contiguous in number. */
        _ux_utility_short_put(notification_buffer + UX_SETUP_INDEX, (USHORT)(rndis_device->ux_slave_class_rndis_interface->ux_slave_interface_descriptor.bInterfaceNumber + 1));

        /* And the length is zero.  */
        *(notification_buffer + UX_SETUP_LENGTH) = 0;

        /* Send the request to the device controller.  */
        status =  _ux_device_stack_transfer_request(interrupt_transfer, UX_DEVICE_CLASS_CDC_ECM_INTERRUPT_RESPONSE_LENGTH,
                                                            UX_DEVICE_CLASS_CDC_ECM_INTERRUPT_RESPONSE_LENGTH);
        /* Check error code. */
        if (status != UX_SUCCESS)
            test_control_return(0);
    }

    for (num_iters = 0; num_iters < 100; num_iters++)
    {

        for (i = 0; i < 10; i++)
            write_packet_udp(&udp_socket_device, &packet_pool_device, HOST_IP_ADDRESS, HOST_SOCKET_PORT_UDP, global_basic_test_num_writes_device++, "device");

        for (i = 0; i < 10; i++)
            read_packet_udp(&udp_socket_device, global_basic_test_num_reads_device++, "device");
    }
}