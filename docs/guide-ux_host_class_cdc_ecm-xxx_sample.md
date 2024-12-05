# Purpose
* Build USBX Host CDC-ECM/ASIX Example from existing host Example, which
  * Enumerates a connected ethernet bridge device (CDC-ECM and/or ASIX)
    * When device connected, it runs at fixed IP address 192.168.77.77
      * Accept pings
        * set static IP 192.168.77.1 on another end
        * ping 192.168.77.77
      * Accept IPerf tests
        * access http://192.168.77.77, select tests
        * run iperf on another end to issue the tests

# Steps
## Get a working host storage Example
* Download: https://github.com/azure-rtos/samples/releases/download/v6.1_rel/Azure_RTOS_6.1_MIMXRT1060_IAR_Samples_2021_11_03.zip
  * Extract
  * Confirm project `sample_usbx_host_mass_storage` works
* Alternates:
  * ST F746 example to start: https://github.com/azure-rtos/samples/releases/download/v6.0.2_rel/Azure_RTOS_6.0.2_STM32F746G-DISCO_IAR_Samples_2020_08_18.zip
    * Extract
    * Replace usbx/common and uxbx/ports files with latest USBX code
    * Add new files to "usbx - Debug" (just add all or check linking errors to add minimal)
    * Confirm project `sample_usbx_host_mass_storage` works
    * By default NX is not included in `sample_usbx_host_mass_storage` to add:
      * Include: `$PROJ_DIR$\..\netxduo\ports\cortex_m7\iar\inc`
      * Lib: `netxduo.a`
    * By default `iperf` is not included in NX, ignore `iperf` part, you can still `ping` your device
  * https://github.com/azure-rtos/samples/releases/download/v6.2_rel/Azure_RTOS_6.2_MIMXRT1060_IAR_Samples_2022_11_30.zip

## Confirm modules are ready
* Check if NetX is included in project
* Check if ux_host_class_cdc_ecm.* are included in project
* Check if ux_host_class_asix.* are included in project (if support ASIX devices)
* Check if ux_network_driver.* are included in project

## Modifications in application code

* Assumption for error handling function:
```c
static INT _halt_line = 0;
static inline void _error_halt(int line)
{
    _halt_line = line;
    while(_halt_line);
}
```

* Assumption for definitions
```c
#define DEMO_STACK_SIZE             (2 * 1024)

/* Allocated from USBX memory space.  */
#define PAYLOAD_SIZE                UX_MAX(UX_HOST_CLASS_ASIX_NX_PAYLOAD_SIZE, UX_HOST_CLASS_CDC_ECM_NX_PAYLOAD_SIZE)
#define NX_PACKET_POOL_SIZE         ((PAYLOAD_SIZE + sizeof(NX_PACKET)) * 25) /* iPerf not working if it's too small.  */
#define HTTP_STACK_SIZE             2048
#define IPERF_STACK_SIZE            2048

#define IP_THREAD_STACK_SIZE        2048
#define ARP_THREAD_STACK_SIZE       1024

#define DEMO_IP_ADDRESS             IP_ADDRESS(192,168,77,77)
#define DEMO_NETWORK_MASK           IP_ADDRESS(255,255,255,0)
```

* To simplify, memories NetX used are allocated through USBX memory allocation, but they can also be prepared other way.
  * If DMA is used by USB controller driver, make sure:
    * Controller driver handles caching syncing or
    * The pool memory is in cache safe area.
    * In the demo it assumes the memory referenced is always cache safe.

### Add NetX related things

* Functions prototypes
```c
void  demo_nx_thread_entry(ULONG arg);

extern  VOID nx_iperf_entry(NX_PACKET_POOL *pool_ptr, NX_IP *ip_ptr, UCHAR* http_stack, ULONG http_stack_size, UCHAR *iperf_stack, ULONG iperf_stack_size);
```

* Global variables
```c
TX_THREAD                           demo_nx_thread;

NX_PACKET_POOL                      nx_pool;

NX_IP                               *nx_ip;

UCHAR                               *nx_pool_memory;

UCHAR                               *nx_ip_stack;
UCHAR                               *nx_arp_stack;

UCHAR                               *http_stack;
UCHAR                               *iperf_stack;
```

* Start a new thread for NetX, after calling `ux_system_initialize`
```c
    UCHAR *demo_nx_thread_stack = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, DEMO_STACK_SIZE);
    status = tx_thread_create(&demo_nx_thread, "nx demo", demo_nx_thread_entry, 0,
                     demo_nx_thread_stack, DEMO_STACK_SIZE,
                     30, 30, 1, TX_AUTO_START);
    if (status)
        _error_halt(__LINE__);
```

* NetX demo thread entry is implement as following:
```c
void  demo_nx_thread_entry(ULONG arg)
{

UINT                status;

    /* Create a packet pool.  */
    nx_pool_memory = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, NX_PACKET_POOL_SIZE);
    if (nx_pool_memory == UX_NULL)
        _error_halt(__LINE__);
    status = nx_packet_pool_create(&nx_pool, "NetX Main Packet Pool",
                                   (PAYLOAD_SIZE + sizeof(NX_PACKET)),
                                   nx_pool_memory, NX_PACKET_POOL_SIZE);

    /* Create IP instance.  */
    nx_ip = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(NX_IP));
    if (nx_ip == UX_NULL)
        _error_halt(__LINE__);
    nx_ip_stack = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, IP_THREAD_STACK_SIZE);
    if (nx_ip_stack == UX_NULL)
        _error_halt(__LINE__);
    status = nx_ip_create(nx_ip, "NetX IP Instance 0",
                          DEMO_IP_ADDRESS, DEMO_NETWORK_MASK,
                          &nx_pool, _ux_network_driver_entry,
                          nx_ip_stack, IP_THREAD_STACK_SIZE, 1);

    /* Enable packet fragmentation.  */
    status = nx_ip_fragment_enable(nx_ip);
    if (status)
        _error_halt(__LINE__);

    /* Enable UDP traffic.  */
    status =  nx_udp_enable(nx_ip);
    if (status)
        _error_halt(__LINE__);

    /* Enable ICMP.  */
    status =  nx_icmp_enable(nx_ip);
    if (status)
        _error_halt(__LINE__);

    /* Enable TCP traffic.  */
    status =  nx_tcp_enable(nx_ip);
    if (status)
        _error_halt(__LINE__);

    /* Enable ARP and supply ARP cache memory for IP Instance 0.  */
    nx_arp_stack = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, ARP_THREAD_STACK_SIZE);
    if (nx_arp_stack == UX_NULL)
        _error_halt(__LINE__);
    status =  nx_arp_enable(nx_ip, (void *) nx_arp_stack, ARP_THREAD_STACK_SIZE);

    /* Set the HTTP stack and IPerf stack.  */
    http_stack = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, HTTP_STACK_SIZE);
    if (http_stack == UX_NULL)
        _error_halt(__LINE__);
    iperf_stack = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, IPERF_STACK_SIZE);
    if (iperf_stack == UX_NULL)
        _error_halt(__LINE__);

    /* Call entry function to start iperf test.  */
    nx_iperf_entry(&nx_pool, nx_ip, http_stack, HTTP_STACK_SIZE, iperf_stack, IPERF_STACK_SIZE);
}
```


### Modify USBX related things
* Add include file
```c
#include "ux_host_class_cdc_ecm.h"
```
```c
#include "ux_host_class_asix.h"
```

* Add global variables
```c
UX_DEVICE                           *device = UX_NULL;  /* Last operated device.  */
UX_HOST_CLASS_ASIX                  *asix = UX_NULL;
UX_HOST_CLASS_CDC_ECM               *cdc_ecm = UX_NULL;

ULONG                               other_event = 0xFFFFFFFF;
UX_HOST_CLASS                       *other_class = UX_NULL;
VOID                                *other_inst = UX_NULL;
```

* Find `ux_host_stack_initialize`, if host change callback is not registered, register a change callback for CDC_ECM interface finding
```c
UINT  demo_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst);
```
```c
    /* The code below is required for installing the host portion of USBX.  */
    status =  ux_host_stack_initialize(demo_system_host_change_function);
    if (status != UX_SUCCESS)
        return;
```
```c
static UINT demo_device_conn_disc(ULONG event, UX_HOST_CLASS *unused, UX_DEVICE *inst)
{
ULONG                   cfg_index;
ULONG                   ifc_index;
UX_CONFIGURATION        *cfg;
UX_INTERFACE            *ifc;
UINT                    status;


    /* Log last operated device.  */
    device = inst;

    /* Removal, there is nothing to do.  */
    if (event == UX_DEVICE_DISCONNECTION)
        return(UX_SUCCESS);

    /* Insertion, if it's not configured, check configuration settings.  */
    if (inst -> ux_device_state == UX_DEVICE_CONFIGURED)
        return(UX_SUCCESS);

    /* Check if there are multiple configurations.  */
    if (inst -> ux_device_descriptor.bNumConfigurations <= 1)
        return(UX_SUCCESS);

    /* Find CDC-ECM.  */
    for (cfg_index = 0; cfg_index < inst -> ux_device_descriptor.bNumConfigurations; cfg_index ++)
    {
        status = ux_host_stack_device_configuration_get(inst, cfg_index, &cfg);
        if (status != UX_SUCCESS)
            break;

        /* Check interfaces.  */
        for (ifc_index = 0; ifc_index < cfg -> ux_configuration_descriptor.bNumInterfaces; ifc_index ++)
        {
            status = ux_host_stack_configuration_interface_get(cfg, ifc_index, 0, &ifc);
            if (status != UX_SUCCESS)
                break;

            /* Check interface class/subclass/protocol.  */
            if (ifc -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS &&
                ifc -> ux_interface_descriptor.bInterfaceSubClass == UX_HOST_CLASS_CDC_ECM_CONTROL_SUBCLASS)
            {

                /* Activate the configuration.  */
                status = ux_host_stack_device_configuration_activate(cfg);
                if (status == UX_SUCCESS)
                    return(UX_SUCCESS);

                /* Failed, break interface check and try next configuration.  */
                break;
            }
        }
    }

    /* Done.  */
    return(UX_SUCCESS);
}
static UINT demo_function_ins_rm(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{
    if (cls->ux_host_class_entry_function == _ux_host_class_asix_entry)
    {
        if (event == UX_DEVICE_INSERTION)
            asix = (UX_HOST_CLASS_ASIX *)inst;
        else
            asix = UX_NULL;
        return(UX_SUCCESS);
    }
    if (cls->ux_host_class_entry_function == _ux_host_class_cdc_ecm_entry)
    {
        if (event == UX_DEVICE_INSERTION)
            cdc_ecm = (UX_HOST_CLASS_CDC_ECM *)inst;
        else
            cdc_ecm = UX_NULL;
        return(UX_SUCCESS);
    }
    return(UX_ERROR);
}
UINT demo_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{
    switch (event)
    {
    case UX_DEVICE_CONNECTION:
    case UX_DEVICE_DISCONNECTION:
        return(demo_device_conn_disc(event, UX_NULL, (UX_DEVICE *)inst));

    case UX_DEVICE_INSERTION:
    case UX_DEVICE_REMOVAL:
        if (demo_function_ins_rm(event, cls, inst) == UX_SUCCESS)
            return(UX_SUCCESS);

    default:
        break;
    }
    other_event = event;
    other_class = cls;
    other_inst = inst;
    return(UX_SUCCESS);
}
```


* Add class registration
  Find the code that register storage class via `ux_host_stack_class_register` and add following code to register CDC-ECM and/or ASIX class.
```c
    /* Register CDC-ECM class.  */
    status = ux_host_stack_class_register(_ux_system_host_class_cdc_ecm_name, ux_host_class_cdc_ecm_entry);
    if (status != UX_SUCCESS)
        return;
```
```c
    /* Register ASIX class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_asix_name, ux_host_class_asix_entry);
    if (status != UX_SUCCESS)
        return;
```
