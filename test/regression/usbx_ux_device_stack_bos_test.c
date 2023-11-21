/* This test is designed to test the BOS.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"


/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_RUN             1
#define UX_TEST_MEMORY_SIZE     (64*1024)


/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           buffer[UX_TEST_BUFFER_SIZE];

/* Define USBX test global variables.  */

#define DW3(x)     (((x) >> 24) & 0xFFu)
#define DW2(x)     (((x) >> 16) & 0xFFu)
#define DW1(x)     (((x) >>  8) & 0xFFu)
#define DW0(x)     ( (x)        & 0xFFu)
#define W1(x)      (((x) >>  8) & 0xFFu)
#define W0(x)      ( (x)        & 0xFFu)

#define _DEVICE_DESCRIPTOR \
    /* Device descriptor.  */       \
    18, UX_DEVICE_DESCRIPTOR_ITEM,  \
    W0(0x201), W1(0x201),           \
    UX_CLASS_BILLBOARD_CLASS,       \
    UX_CLASS_BILLBOARD_SUBCLASS,    \
    UX_CLASS_BILLBOARD_PROTOCOL,    \
    64,                             \
    W0(0xec08), W1(0xec08),         \
    W0(0x0001), W1(0x0001),         \
    W0(0x0001), W1(0x0001),         \
    0, 0, 0,                        \
    1

#define _DEVICE_QUALIFIER_DESCRIPTOR \
    /* Device Qualifier Descriptor.  */     \
    10, UX_DEVICE_QUALIFIER_DESCRIPTOR_ITEM,\
    W0(0x201), W1(0x201),                   \
    UX_CLASS_BILLBOARD_CLASS,               \
    UX_CLASS_BILLBOARD_SUBCLASS,            \
    UX_CLASS_BILLBOARD_PROTOCOL,            \
    8,                                      \
    1,                                      \
    0

#define _CONFIGURATION_DESCRIPTOR \
    /* Configuration Descriptor.  */    \
    9, UX_CONFIGURATION_DESCRIPTOR_ITEM,\
    W0(18), W1(18),                     \
    1, 1,                               \
    0, 0xc0, 0x32

#define _INTERFACE_DESCRIPTOR \
    /* Interface Descriptor.  */    \
    9, UX_INTERFACE_DESCRIPTOR_ITEM,\
    0, 0, 0,                        \
    UX_CLASS_BILLBOARD_CLASS,       \
    UX_CLASS_BILLBOARD_SUBCLASS,    \
    UX_CLASS_BILLBOARD_PROTOCOL,    \
    0

#define _BOS_DESCRIPTORS_LENGTH (5+7+56+8+8+8)
#define _BOS_DESCRIPTORS \
    /* BOS descriptor.  */                                      \
    5, UX_BOS_DESCRIPTOR_ITEM,                                  \
    W0(_BOS_DESCRIPTORS_LENGTH), W1(_BOS_DESCRIPTORS_LENGTH), 1,\
    /* USB 2.0 Extension descriptor  */                                       \
    7, UX_DEVICE_CAPABILITY_DESCRIPTOR_ITEM, UX_CAPABILITY_USB_2_0_EXTENSION, \
    DW0(0x00000002u), DW1(0x00000002u), DW2(0x00000002u), DW3(0x00000002u),   \
    /* Billboard Capability Descriptor Example (44+3*4=56)  */\
    56, UX_DEVICE_CAPABILITY_DESCRIPTOR_ITEM, UX_CAPABILITY_BILLBOARD, 0,   \
    1, 0,                                                                   \
    W0(0), W1(0),                                                           \
    0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                         \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                         \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                         \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                         \
    W0(0x0000), W0(0x0000),                                                 \
    0x00, 0x00,                                                             \
    W0(0x8087), W1(0x8087), 0x00, 0x00,                                     \
    W0(0x8087), W1(0x8087), 0x01, 0x00,                                     \
    W0(0xFF01), W1(0xFF01), 0x00, 0x00,                                     \
    /* Billboard Alternate Mode Capability Descriptor Examples  */\
    8, UX_DEVICE_CAPABILITY_DESCRIPTOR_ITEM, UX_CAPABILITY_BILLBOARD_EX,    \
    0, DW0(0x00000010), DW1(0x00000010), DW2(0x00000010), DW3(0x00000010),  \
    8, UX_DEVICE_CAPABILITY_DESCRIPTOR_ITEM, UX_CAPABILITY_BILLBOARD_EX,    \
    1, DW0(0x00000002), DW1(0x00000002), DW2(0x00000002), DW3(0x00000002),  \
    8, UX_DEVICE_CAPABILITY_DESCRIPTOR_ITEM, UX_CAPABILITY_BILLBOARD_EX,    \
    2, DW0(0x000C00C5), DW1(0x000C00C5), DW2(0x000C00C5), DW3(0x000C00C5)


static UCHAR device_framework_full_speed[] = {

    _DEVICE_DESCRIPTOR,
    _BOS_DESCRIPTORS,
    _CONFIGURATION_DESCRIPTOR,
    _INTERFACE_DESCRIPTOR,
};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)

static UCHAR device_framework_high_speed[] = {

    _DEVICE_DESCRIPTOR,
    _BOS_DESCRIPTORS,
    _DEVICE_QUALIFIER_DESCRIPTOR,
    _CONFIGURATION_DESCRIPTOR,
    _INTERFACE_DESCRIPTOR,
};
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)


/* String Device Framework :
     Byte 0 and 1 : Word containing the language ID : 0x0904 for US
     Byte 2       : Byte containing the index of the descriptor
     Byte 3       : Byte containing the length of the descriptor string
*/
static UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72, 0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
        0x09, 0x04, 0x02, 0x0c,
        0x44, 0x61, 0x74, 0x61, 0x50, 0x75, 0x6d, 0x70,
        0x44, 0x65, 0x6d, 0x6f,

    /* Serial Number string descriptor : Index 3 */
        0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31
};
#define STRING_FRAMEWORK_LENGTH sizeof(string_framework)


/* Multiple languages are supported on the device, to add
       a language besides English, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
static UCHAR language_id_framework[] = {

    /* English. */
        0x09, 0x04
};
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(language_id_framework)


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static TX_THREAD           ux_test_thread_host_simulation;
static TX_THREAD           ux_test_thread_slave_simulation;
static void                ux_test_thread_host_simulation_entry(ULONG);
static void                ux_test_thread_slave_simulation_entry(ULONG);


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
    if (error_code != UX_DEVICE_HANDLE_UNKNOWN &&
        error_code != UX_DEVICE_ENUMERATION_FAILURE)
    {
        /* Failed test.  */
        printf("Error #%d, system_level: %d, system_context: %d, error code: 0x%x\n", __LINE__, system_level, system_context, error_code);
        test_control_return(1);
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_stack_bos_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;


    printf("Running ux_device_stack BOS Test....................................");

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX.  */
    status =  ux_host_stack_initialize(UX_NULL);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
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

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_host_simulation, "test host simulation", ux_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}


static void  ux_test_thread_host_simulation_entry(ULONG arg)
{

UINT                    status;
INT                     i;
UX_DEVICE               *device;
UX_ENDPOINT             *control_endpoint;
UX_TRANSFER             *transfer_request;


    /* Wait device connection.  */
    for (i = 0; i < 10; i ++)
    {
        status = ux_host_stack_device_get(0, &device);
        if (status == UX_SUCCESS)
            break;
        ux_utility_delay_ms(10);
    }
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    control_endpoint = &device->ux_device_control_endpoint;
    transfer_request = &control_endpoint->ux_endpoint_transfer_request;

    /* Wait until enumeration retries end.  */
    ux_utility_delay_ms(1000);

    /* Test BOS request.  */
    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  0xFF;
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             UX_BOS_DESCRIPTOR_ITEM << 8;
    transfer_request -> ux_transfer_request_index =             0;

    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        error_counter ++;
    }
    if (transfer_request->ux_transfer_request_actual_length != _BOS_DESCRIPTORS_LENGTH)
    {

        printf("ERROR #%d: 0x%x\n", __LINE__, status);
        error_counter ++;
    }

    /* Sleep for a tick to make sure everything is complete.  */
    tx_thread_sleep(1);

    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* Test error.  */
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    else
    {

        /* Successful test.  */
        printf("SUCCESS\n");
        test_control_return(0);
    }
}
