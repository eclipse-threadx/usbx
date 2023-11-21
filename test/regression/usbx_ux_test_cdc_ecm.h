#ifndef USBX_UX_TEST_CDC_ECM_H
#define USBX_UX_TEST_CDC_ECM_H

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
#include "ux_test_actions.h"

//#define LOCAL_MACHINE

typedef struct DEVICE_INIT_DATA
{
    UCHAR *framework;
    ULONG framework_length;
    UCHAR *string_framework;
    ULONG string_framework_length;
    UCHAR dont_register_hcd;
    UCHAR dont_connect_to_host;
} DEVICE_INIT_DATA;

#define DEMO_IP_THREAD_STACK_SIZE           (8*1024)
#define HOST_IP_ADDRESS                     IP_ADDRESS(192,168,1,176)
#define HOST_SOCKET_PORT_UDP                45054
#define HOST_SOCKET_PORT_TCP                45056
#define DEVICE_IP_ADDRESS                   IP_ADDRESS(192,168,1,175)
#define DEVICE_SOCKET_PORT_UDP              45055
#define DEVICE_SOCKET_PORT_TCP              45057

#ifndef TEST_NX_PACKET_CHAIN
#define TCP_WINDOW                          1600
#define PACKET_PAYLOAD                      1600
#define WRITE_APPEND_SIZE                   (0)
#define PACKET_POOL_SIZE                    (PACKET_PAYLOAD*10000)
#define HOST_IP_POOL_SIZE                   (PACKET_PAYLOAD*UX_HOST_CLASS_CDC_ECM_NX_PKPOOL_ENTRIES)
#define DEVICE_IP_POOL_SIZE                 (PACKET_PAYLOAD*UX_DEVICE_CLASS_CDC_ECM_NX_PKPOOL_ENTRIES)
#else
#define TCP_WINDOW                          1400
#define PACKET_PAYLOAD                      (256)
#define WRITE_APPEND_SIZE                   (300)
#define PACKET_POOL_SIZE                    (PACKET_PAYLOAD*10000)
#define HOST_IP_POOL_SIZE                   (PACKET_PAYLOAD*UX_HOST_CLASS_CDC_ECM_NX_PKPOOL_ENTRIES)
#define DEVICE_IP_POOL_SIZE                 (PACKET_PAYLOAD*UX_DEVICE_CLASS_CDC_ECM_NX_PKPOOL_ENTRIES)
#endif
#define ARP_MEMORY_SIZE                     1024

/* Define local constants.  */

#define UX_DEMO_STACK_SIZE                  (4*1024)
#define UX_USBX_MEMORY_SIZE                 (128*1024)

/* Define basic test constants.  */

#ifndef BASIC_TEST_NUM_ITERATIONS
#define BASIC_TEST_NUM_ITERATIONS               10
#endif
#ifndef BASIC_TEST_NUM_PACKETS_PER_ITERATION
#define BASIC_TEST_NUM_PACKETS_PER_ITERATION    10
#endif
#define BASIC_TEST_NUM_TOTAL_PACKETS            (BASIC_TEST_NUM_ITERATIONS*BASIC_TEST_NUM_PACKETS_PER_ITERATION)
#define BASIC_TEST_HOST                         0
#define BASIC_TEST_DEVICE                       1
#define BASIC_TEST_TCP                          0
#define BASIC_TEST_UDP                          1

/* Host */

static UX_HOST_CLASS                        *class_driver_host;
static UX_HOST_CLASS_CDC_ECM                *cdc_ecm_host;
static UX_HOST_CLASS_CDC_ECM                *cdc_ecm_host_from_system_change_function;
static TX_THREAD                            thread_host;
static UCHAR                                thread_stack_host[UX_DEMO_STACK_SIZE];
static NX_IP                                nx_ip_host;
static NX_PACKET_POOL                       ip_pool_host;
static UCHAR                                ip_pool_host_name[] = "NetX Host IP Pool";
static NX_PACKET_POOL                       packet_pool_host;
static NX_PACKET_POOL                       *packet_pool_host_ptr = &packet_pool_host;
static NX_UDP_SOCKET                        udp_socket_host;
static NX_TCP_SOCKET                        tcp_socket_host;
static CHAR                                 *packet_pool_memory_host;
static CHAR                                 ip_thread_stack_host[DEMO_IP_THREAD_STACK_SIZE];
static CHAR                                 arp_memory_host[ARP_MEMORY_SIZE];

/* Device */

static TX_THREAD                            thread_device;
static UX_HOST_CLASS                        *class_driver_device;
static UX_SLAVE_CLASS_CDC_ECM               *cdc_ecm_device;
static UX_SLAVE_CLASS_CDC_ECM_PARAMETER     cdc_ecm_parameter;
static UCHAR                                thread_stack_device[UX_DEMO_STACK_SIZE];
static NX_IP                                nx_ip_device;
static NX_PACKET_POOL                       ip_pool_device;
static NX_PACKET_POOL                       packet_pool_device;
static NX_UDP_SOCKET                        udp_socket_device;
static NX_TCP_SOCKET                        tcp_socket_device;
static CHAR                                 *packet_pool_memory_device;
static CHAR                                 ip_thread_stack_device[DEMO_IP_THREAD_STACK_SIZE];
static CHAR                                 arp_memory_device[ARP_MEMORY_SIZE];

static UCHAR                                global_is_device_initialized;
static UCHAR                                global_host_ready_for_application;

static ULONG                                global_basic_test_num_writes_host;
static ULONG                                global_basic_test_num_reads_host;
static ULONG                                global_basic_test_num_writes_device;
static ULONG                                global_basic_test_num_reads_device;

/* Define local prototypes and definitions.  */
static void thread_entry_host(ULONG arg);
static void thread_entry_device(ULONG arg);
static void post_init_host();
static void post_init_device();

#define DEFAULT_FRAMEWORK_LENGTH sizeof(default_device_framework)
static unsigned char default_device_framework[] = {

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

    /* Configuration Descriptor @ 18 */
    0x09, /* bLength */
    0x02, /* bDescriptorType */
    0x58, 0x00, /* wTotalLength */
    0x02, /* bNumInterfaces */
    0x01, /* bConfigurationValue */
    0x00, /* iConfiguration */
    0xc0, /* bmAttributes - Self-powered */
    0x00, /* bMaxPower */

    /* Interface Association Descriptor @ 18+9=27 */
    0x08, /* bLength */
    0x0b, /* bDescriptorType */
    0x00, /* bFirstInterface */
    0x02, /* bInterfaceCount */
    0x02, /* bFunctionClass - CDC - Communication */
    0x06, /* bFunctionSubClass - ECM */
    0x00, /* bFunctionProtocol - No class specific protocol required */
    0x00, /* iFunction */

    /* Interface Descriptor @ 27+8=35 */
    0x09, /* bLength */
    0x04, /* bDescriptorType */
    0x00, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x01, /* bNumEndpoints */
    0x02, /* bInterfaceClass - CDC - Communication */
    0x06, /* bInterfaceSubClass - ECM */
    0x00, /* bInterfaceProtocol - No class specific protocol required */
    0x00, /* iInterface */

    /* CDC Header Functional Descriptor @ 35+9=44 */
    0x05, /* bLength */
    0x24, /* bDescriptorType */
    0x00, /* bDescriptorSubType */
    0x10, 0x01, /* bcdCDC */

    /* CDC ECM Functional Descriptor @ 44+5=49 */
    0x0d, /* bLength */
    0x24, /* bDescriptorType */
    0x0f, /* bDescriptorSubType */
    0x04, /* iMACAddress */
    0x00, 0x00, 0x00, 0x00, /* bmEthernetStatistics */
    0xea, 0x05, /* wMaxSegmentSize */
    0x00, 0x00, /* wNumberMCFilters */
    0x00, /* bNumberPowerFilters */

    /* CDC Union Functional Descriptor @ 49+13=62 */
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

static unsigned char default_string_framework[] = {

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

    /* Multiple languages are supported on the device, to add
       a language besides english, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
static unsigned char language_id_framework[] = {

    /* English. */
        0x09, 0x04
    };

static DEVICE_INIT_DATA default_device_init_data = { 
    .framework = default_device_framework, 
    .framework_length = sizeof(default_device_framework),
    .string_framework = default_string_framework,
    .string_framework_length = sizeof(default_string_framework)
};

static void ux_test_cdc_ecm_initialize_use_framework(void *first_unused_memory, DEVICE_INIT_DATA *device_init_data)
{

CHAR *memory_pointer = first_unused_memory;

    /* Initialize possible uninitialized device init values. */

    if (device_init_data->framework == NULL)
    {
        device_init_data->framework = default_device_framework;
        device_init_data->framework_length = sizeof(default_device_framework);
    }

    if (device_init_data->string_framework == NULL)
    {
        device_init_data->string_framework = default_string_framework;
        device_init_data->string_framework_length = sizeof(default_string_framework);
    }

    /* Initialize USBX Memory. */
    ux_system_initialize(memory_pointer, UX_USBX_MEMORY_SIZE, UX_NULL, 0);
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
    memory_pointer += PACKET_POOL_SIZE + HOST_IP_POOL_SIZE;
    packet_pool_memory_device = memory_pointer;
    memory_pointer += PACKET_POOL_SIZE + DEVICE_IP_POOL_SIZE;

    /* Create the host thread. */
    UX_TEST_CHECK_SUCCESS(tx_thread_create(&thread_host, "host thread", thread_entry_host, (ULONG)(ALIGN_TYPE)device_init_data,
                                           thread_stack_host, UX_DEMO_STACK_SIZE,
                                           30, 30, 1, TX_DONT_START));
    UX_THREAD_EXTENSION_PTR_SET(&thread_host, device_init_data)
    tx_thread_resume(&thread_host);

    /* Create the slave thread. */
    UX_TEST_CHECK_SUCCESS(tx_thread_create(&thread_device, "device thread", thread_entry_device, (ULONG)(ALIGN_TYPE)device_init_data,
                                           thread_stack_device, UX_DEMO_STACK_SIZE,
                                           30, 30, 1, TX_DONT_START));
    UX_THREAD_EXTENSION_PTR_SET(&thread_device, device_init_data)
    tx_thread_resume(&thread_device);
}

static void ux_test_cdc_ecm_initialize(void *first_unused_memory)
{

    ux_test_cdc_ecm_initialize_use_framework(first_unused_memory, &default_device_init_data);
}

static UINT system_change_function(ULONG event, UX_HOST_CLASS *class, VOID *instance)
{
    if (event == UX_DEVICE_INSERTION)
    {printf("hINS\n");
        cdc_ecm_host_from_system_change_function = instance;
    }
    else if (event == UX_DEVICE_REMOVAL)
    {printf("hRM\n");
        cdc_ecm_host_from_system_change_function = UX_NULL;
    }
    return(UX_SUCCESS);
}

static void class_cdc_ecm_get_host(void)
{

UX_HOST_CLASS   *class;

    /* Find the main storage container */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_class_get(_ux_system_host_class_cdc_ecm_name, &class));

    /* We get the first instance of the storage device */
    UX_TEST_CHECK_SUCCESS(ux_test_host_stack_class_instance_get(class, 0, (void **) &cdc_ecm_host));

    /* We still need to wait for the cdc-ecm status to be live */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uint(&cdc_ecm_host -> ux_host_class_cdc_ecm_state, UX_HOST_CLASS_INSTANCE_LIVE));

	/* Now wait for the link to be up.  */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_host->ux_host_class_cdc_ecm_link_state, UX_HOST_CLASS_CDC_ECM_LINK_STATE_UP));
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

/* Copied and pasted from ux_device_class_cdc_ecm_change.c. */
static void ux_test_device_class_cdc_ecm_set_link_state(UX_SLAVE_CLASS_CDC_ECM *cdc_ecm, UCHAR new_link_state)
{

    /* Declare the link to be down. */
    cdc_ecm_device -> ux_slave_class_cdc_ecm_link_state =  new_link_state;

    /* We have a thread waiting for an event, we wake it up with a NETWORK NOTIFICATION CHANGE event. 
       In turn they will release the NetX resources used and suspend.  */
    UX_TEST_CHECK_SUCCESS(_ux_utility_event_flags_set(&cdc_ecm_device -> ux_slave_class_cdc_ecm_event_flags_group, UX_DEVICE_CLASS_CDC_ECM_NETWORK_NOTIFICATION_EVENT, TX_OR)); 
}

static void read_packet_tcp(NX_TCP_SOCKET *tcp_socket, ULONG num_reads, CHAR *name)
{

NX_PACKET 	*rcv_packet;
ULONG       num_writes_from_peer;

#ifndef LOCAL_MACHINE
    if (num_reads % 100 == 0)
#endif
        stepinfo("%s reading tcp packet# %lu\n", name, num_reads);

    UX_TEST_CHECK_SUCCESS(nx_tcp_socket_receive(tcp_socket, &rcv_packet, NX_WAIT_FOREVER));

    num_writes_from_peer = *(ULONG *)rcv_packet->nx_packet_prepend_ptr;
    UX_TEST_ASSERT(num_writes_from_peer == num_reads);

    UX_TEST_CHECK_SUCCESS(nx_packet_release(rcv_packet));
}

static void write_packet_tcp(NX_TCP_SOCKET *tcp_socket, NX_PACKET_POOL *packet_pool, ULONG num_writes, CHAR *name)
{

NX_PACKET 	*out_packet;

    UX_TEST_CHECK_SUCCESS(nx_packet_allocate(packet_pool, &out_packet, NX_TCP_PACKET, NX_WAIT_FOREVER));

    *(ULONG *)out_packet->nx_packet_prepend_ptr = num_writes;
    out_packet->nx_packet_length = sizeof(ULONG);
    out_packet->nx_packet_append_ptr = out_packet->nx_packet_prepend_ptr + out_packet->nx_packet_length;
#if WRITE_APPEND_SIZE > 0
    /* Append data until packet chain created.  */
    for (int i = 0, remain = WRITE_APPEND_SIZE; i < WRITE_APPEND_SIZE; i += 10)
    {
        remain = WRITE_APPEND_SIZE - i;
        if (remain < 10)
        {
            nx_packet_data_append(out_packet, "0123456789", remain, packet_pool, NX_WAIT_FOREVER);
            break;
        }
        else
        {
            nx_packet_data_append(out_packet, "0123456789", 10, packet_pool, NX_WAIT_FOREVER);
        }
    }
#endif

#ifndef LOCAL_MACHINE
    if (num_writes % 100 == 0)
#endif
        stepinfo("%s writing tcp packet# %lu\n", name, num_writes);

    UX_TEST_CHECK_SUCCESS(nx_tcp_socket_send(tcp_socket, out_packet, NX_WAIT_FOREVER));
}

static void read_packet_udp(NX_UDP_SOCKET *udp_socket, ULONG num_reads, CHAR *name)
{

NX_PACKET 	*rcv_packet;
ULONG       num_writes_from_peer;

#ifndef LOCAL_MACHINE
    if (num_reads % 100 == 0)
#endif
        stepinfo("%s reading udp packet# %lu\n", name, num_reads);

    UX_TEST_CHECK_SUCCESS(nx_udp_socket_receive(udp_socket, &rcv_packet, NX_WAIT_FOREVER));

    num_writes_from_peer = *(ULONG *)rcv_packet->nx_packet_prepend_ptr;
    UX_TEST_ASSERT(num_writes_from_peer == num_reads);

    UX_TEST_CHECK_SUCCESS(nx_packet_release(rcv_packet));
}

static void write_udp(NX_UDP_SOCKET *udp_socket, NX_PACKET_POOL *packet_pool, ULONG ip_address, ULONG port, ULONG num_writes, CHAR *name, ULONG len)
{

NX_PACKET 	*out_packet;
ULONG       i, remain;

    UX_TEST_CHECK_SUCCESS(nx_packet_allocate(packet_pool, &out_packet, NX_UDP_PACKET, NX_WAIT_FOREVER));

    *(ULONG *)out_packet->nx_packet_prepend_ptr = num_writes;
    out_packet->nx_packet_length = sizeof(ULONG);
    out_packet->nx_packet_append_ptr = out_packet->nx_packet_prepend_ptr + out_packet->nx_packet_length;
    if (len > 4)
    {

        /* Append packet data.  */
        for (i = 0, remain = len - 4; i < len - 4; i += 10)
        {
            remain = len - 4 - i;
            if (remain < 10)
            {
                nx_packet_data_append(out_packet, "0123456789", remain, packet_pool, NX_WAIT_FOREVER);
                break;
            }
            else
            {
                nx_packet_data_append(out_packet, "0123456789", 10, packet_pool, NX_WAIT_FOREVER);
            }
        }
    }

#ifndef LOCAL_MACHINE
    if (num_writes % 100 == 0)
#endif
        stepinfo("%s writing udp packet# %lu, %ld\n", name, num_writes, out_packet->nx_packet_length);

    UX_TEST_CHECK_SUCCESS(nx_udp_socket_send(udp_socket, out_packet, ip_address, port));
}

static void write_packet_udp(NX_UDP_SOCKET *udp_socket, NX_PACKET_POOL *packet_pool, ULONG ip_address, ULONG port, ULONG num_writes, CHAR *name)
{
    write_udp(udp_socket, packet_pool, ip_address, port, num_writes, name, 4 + WRITE_APPEND_SIZE);
}

static void cdc_ecm_basic_test(UCHAR host_or_device, UCHAR socket_type)
{

UINT i;
UINT num_iters;

    if (host_or_device == BASIC_TEST_HOST)
    {

        if (socket_type == BASIC_TEST_TCP)
        {

            for (num_iters = 0; num_iters < BASIC_TEST_NUM_ITERATIONS; num_iters++)
            {

                for (i = 0; i < BASIC_TEST_NUM_PACKETS_PER_ITERATION; i++)
                    write_packet_tcp(&tcp_socket_host, &packet_pool_host, global_basic_test_num_writes_host++, "host");

                for (i = 0; i < BASIC_TEST_NUM_PACKETS_PER_ITERATION; i++)
                    read_packet_tcp(&tcp_socket_host, global_basic_test_num_reads_host++, "host");
            }
        }
        else
        {

            for (num_iters = 0; num_iters < BASIC_TEST_NUM_ITERATIONS; num_iters++)
            {

                for (i = 0; i < BASIC_TEST_NUM_PACKETS_PER_ITERATION; i++)
                    write_packet_udp(&udp_socket_host, &packet_pool_host, DEVICE_IP_ADDRESS, DEVICE_SOCKET_PORT_UDP, global_basic_test_num_writes_host++, "host");

                for (i = 0; i < BASIC_TEST_NUM_PACKETS_PER_ITERATION; i++)
                    read_packet_udp(&udp_socket_host, global_basic_test_num_reads_host++, "host");
            }
        }

        /* Wait for all transfers to complete. */
        UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&global_basic_test_num_writes_host, BASIC_TEST_NUM_TOTAL_PACKETS));
        UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&global_basic_test_num_reads_host, BASIC_TEST_NUM_TOTAL_PACKETS));

        if (socket_type == BASIC_TEST_TCP)

            /* Wait for all ACKs to complete. */
            /* TODO: find better way. */
            tx_thread_sleep(MS_TO_TICK(1000));

        /* Reset for next test. */
        global_basic_test_num_reads_host = 0;
        global_basic_test_num_writes_host = 0;
    }
    else
    {

        if (socket_type == BASIC_TEST_TCP)
        {

            for (num_iters = 0; num_iters < BASIC_TEST_NUM_ITERATIONS; num_iters++)
            {

                for (i = 0; i < BASIC_TEST_NUM_PACKETS_PER_ITERATION; i++)
                    write_packet_tcp(&tcp_socket_device, &packet_pool_device, global_basic_test_num_writes_device++, "device");

                for (i = 0; i < BASIC_TEST_NUM_PACKETS_PER_ITERATION; i++)
                    read_packet_tcp(&tcp_socket_device, global_basic_test_num_reads_device++, "device");
            }
        }
        else
        {

            for (num_iters = 0; num_iters < BASIC_TEST_NUM_ITERATIONS; num_iters++)
            {

                for (i = 0; i < BASIC_TEST_NUM_PACKETS_PER_ITERATION; i++)
                    write_packet_udp(&udp_socket_device, &packet_pool_device, HOST_IP_ADDRESS, HOST_SOCKET_PORT_UDP, global_basic_test_num_writes_device++, "device");

                for (i = 0; i < BASIC_TEST_NUM_PACKETS_PER_ITERATION; i++)
                    read_packet_udp(&udp_socket_device, global_basic_test_num_reads_device++, "device");
            }
        }

        /* Wait for all transfers to complete. */
        UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&global_basic_test_num_writes_device, BASIC_TEST_NUM_TOTAL_PACKETS));
        UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&global_basic_test_num_reads_device, BASIC_TEST_NUM_TOTAL_PACKETS));

        /* Reset for next test. */
        global_basic_test_num_reads_device = 0;
        global_basic_test_num_writes_device = 0;
    }
}

static void thread_entry_host(ULONG device_init_data_ptr)
{

DEVICE_INIT_DATA *device_init_data;

    UX_THREAD_EXTENSION_PTR_GET(device_init_data, DEVICE_INIT_DATA, device_init_data_ptr);

    /* Wait for device to initialize. */
    UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&global_is_device_initialized, 1));

    /* Create the IP instance. */

    UX_TEST_CHECK_SUCCESS(nx_packet_pool_create(&packet_pool_host, "NetX Host Packet Pool", PACKET_PAYLOAD, packet_pool_memory_host, PACKET_POOL_SIZE));
    UX_TEST_CHECK_SUCCESS(nx_packet_pool_create(&ip_pool_host, ip_pool_host_name, PACKET_PAYLOAD, packet_pool_memory_host+PACKET_POOL_SIZE, HOST_IP_POOL_SIZE));
    UX_TEST_CHECK_SUCCESS(nx_ip_create(&nx_ip_host, "NetX Host Thread", HOST_IP_ADDRESS, 0xFF000000UL,
                          &ip_pool_host, _ux_network_driver_entry, ip_thread_stack_host, DEMO_IP_THREAD_STACK_SIZE, 1));

    /* Setup ARP. */

    UX_TEST_CHECK_SUCCESS(nx_arp_enable(&nx_ip_host, (void *)arp_memory_host, ARP_MEMORY_SIZE));
    UX_TEST_CHECK_SUCCESS(nx_arp_static_entry_create(&nx_ip_host, DEVICE_IP_ADDRESS, 0x0000001E, 0x80032CD8));

    /* Setup UDP. */

    UX_TEST_CHECK_SUCCESS(nx_udp_enable(&nx_ip_host));
    UX_TEST_CHECK_SUCCESS(nx_udp_socket_create(&nx_ip_host, &udp_socket_host, "USB HOST UDP SOCKET", NX_IP_NORMAL, NX_DONT_FRAGMENT, 20, 20));
    UX_TEST_CHECK_SUCCESS(nx_udp_socket_bind(&udp_socket_host, HOST_SOCKET_PORT_UDP, NX_NO_WAIT));

    /* Setup TCP. */

    UX_TEST_CHECK_SUCCESS(nx_tcp_enable(&nx_ip_host));
    UX_TEST_CHECK_SUCCESS(nx_tcp_socket_create(&nx_ip_host, &tcp_socket_host, "USB HOST TCP SOCKET", NX_IP_NORMAL, NX_DONT_FRAGMENT, NX_IP_TIME_TO_LIVE, TCP_WINDOW, NX_NULL, NX_NULL));
    UX_TEST_CHECK_SUCCESS(nx_tcp_server_socket_listen(&nx_ip_host, HOST_SOCKET_PORT_TCP, &tcp_socket_host, 5, NX_NULL));

    /* The code below is required for installing the host portion of USBX. */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_initialize(system_change_function));

    /* Register cdc_ecm class.  */
    UX_TEST_CHECK_SUCCESS(ux_host_stack_class_register(_ux_system_host_class_cdc_ecm_name, ux_host_class_cdc_ecm_entry));

    if (!device_init_data->dont_register_hcd)
    {

        /* Register all the USB host controllers available in this system. */
        UX_TEST_CHECK_SUCCESS(ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize, 0, 0));

        /* Find the storage class. */
        class_cdc_ecm_get_host();

        if (!device_init_data->dont_connect_to_host)
        {

            /* Connect to device TCP socket. */
            UX_TEST_CHECK_SUCCESS(nx_tcp_server_socket_accept(&tcp_socket_host, NX_WAIT_FOREVER));
        }
    }
    global_host_ready_for_application = 1;

    /* Call test code. */
    post_init_host();

    /* We need to disconnect the host and device. This is because the NetX cleaning
       process (in usbxtestcontrol.c) includes disconnect the device, which tries
       to send a RST packet to the peer (or something). By disconnecting here,
       we ensure the deactivate routines notify the network driver so that the
       packet tranmissiong is stopped there. */
    ux_test_disconnect_slave();
    ux_test_disconnect_host_wait_for_enum_completion();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}

static void thread_entry_device(ULONG device_init_data_ptr)
{

DEVICE_INIT_DATA *device_init_data;

    UX_THREAD_EXTENSION_PTR_GET(device_init_data, DEVICE_INIT_DATA, device_init_data_ptr)

    /* Create the IP instance.  */

    UX_TEST_CHECK_SUCCESS(nx_packet_pool_create(&packet_pool_device, "NetX Device Packet Pool", PACKET_PAYLOAD, packet_pool_memory_device, PACKET_POOL_SIZE));
    UX_TEST_CHECK_SUCCESS(nx_packet_pool_create(&ip_pool_device, "NetX Device IP Pool", PACKET_PAYLOAD, packet_pool_memory_device+PACKET_POOL_SIZE, DEVICE_IP_POOL_SIZE));

    UX_TEST_CHECK_SUCCESS(nx_ip_create(&nx_ip_device, "NetX Device Thread", DEVICE_IP_ADDRESS, 0xFF000000L, &ip_pool_device, 
                                       _ux_network_driver_entry, ip_thread_stack_device, DEMO_IP_THREAD_STACK_SIZE, 1));

    /* Setup ARP.  */

    UX_TEST_CHECK_SUCCESS(nx_arp_enable(&nx_ip_device, (void *)arp_memory_device, ARP_MEMORY_SIZE));
    UX_TEST_CHECK_SUCCESS(nx_arp_static_entry_create(&nx_ip_device, HOST_IP_ADDRESS, 0x0000001E, 0x5841B878));

    /* Setup UDP.  */

    UX_TEST_CHECK_SUCCESS(nx_udp_enable(&nx_ip_device));
    UX_TEST_CHECK_SUCCESS(nx_udp_socket_create(&nx_ip_device, &udp_socket_device, "USB DEVICE UDP SOCKET", NX_IP_NORMAL, NX_DONT_FRAGMENT, 20, 20));
    UX_TEST_CHECK_SUCCESS(nx_udp_socket_bind(&udp_socket_device, DEVICE_SOCKET_PORT_UDP, NX_NO_WAIT));

    /* Setup TCP. */

    UX_TEST_CHECK_SUCCESS(nx_tcp_enable(&nx_ip_device));
    UX_TEST_CHECK_SUCCESS(nx_tcp_socket_create(&nx_ip_device, &tcp_socket_device, "USB DEVICE TCP SOCKET", NX_IP_NORMAL, NX_DONT_FRAGMENT, NX_IP_TIME_TO_LIVE, 100, NX_NULL, NX_NULL));
    UX_TEST_CHECK_SUCCESS(nx_tcp_client_socket_bind(&tcp_socket_device, DEVICE_SOCKET_PORT_TCP, NX_WAIT_FOREVER));

    /* The code below is required for installing the device portion of USBX.
       In this demo, DFU is possible and we have a call back for state change. */
    UX_TEST_CHECK_SUCCESS(ux_device_stack_initialize(device_init_data->framework, device_init_data->framework_length,
                                                      device_init_data->framework, device_init_data->framework_length,
                                                      device_init_data->string_framework, device_init_data->string_framework_length,
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

    /* Initialize the device cdc_ecm class. */
    UX_TEST_CHECK_SUCCESS(ux_device_stack_class_register(_ux_system_slave_class_cdc_ecm_name, ux_device_class_cdc_ecm_entry, 1, 0, &cdc_ecm_parameter));

    /* Initialize the simulated device controller.  */
    UX_TEST_CHECK_SUCCESS(_ux_test_dcd_sim_slave_initialize());

    global_is_device_initialized = UX_TRUE;

    if (!device_init_data->dont_connect_to_host)
    {

        if (!device_init_data->dont_register_hcd)
        {

            UX_TEST_CHECK_SUCCESS(ux_test_wait_for_non_null((VOID **)&cdc_ecm_device));
            UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_ulong(&cdc_ecm_device->ux_slave_class_cdc_ecm_link_state, UX_DEVICE_CLASS_CDC_ECM_LINK_STATE_UP));

            /* Connect to host TCP socket. */
            UX_TEST_CHECK_SUCCESS(nx_tcp_client_socket_connect(&tcp_socket_device, HOST_IP_ADDRESS, HOST_SOCKET_PORT_TCP, NX_WAIT_FOREVER));
        }

        /* Wait for host - believe this is so that we know host is always first... gives us some 'determinism'. */
        UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uchar(&global_host_ready_for_application, 1));
    }

    /* Call test code. */
    post_init_device();
}

#endif