/* This test is designed to test the BOS.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_test.h"
#include "ux_test_utility_sim.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_dcd_sim_slave.h"
#include "ux_host_class_dummy.h"


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

static UX_HOST_CLASS_DUMMY             *dummy;

static UX_DEVICE                       *device;

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

static UINT                test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst);

/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}

static ULONG enum_done_count = 0;
static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    if (error_code == UX_DEVICE_ENUMERATION_FAILURE)
        enum_done_count ++;

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
void    usbx_ux_host_stack_enum_bMaxPacketSize0_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;


    printf("Running ux_host_stack bMaxPacketSize0 Test..........................");

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
    status =  ux_host_stack_initialize(test_host_change_function);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register a dummy class.  */
    status  = ux_host_stack_class_register(_ux_host_class_dummy_name, _ux_host_class_dummy_entry);
    UX_TEST_CHECK_SUCCESS(status);

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
ULONG                   mem_free;
const struct _test {
    ULONG               bMaxPacketSize0;
    UINT                expect_no_device;
} tests[] = {
    {  0, UX_FALSE},
    {  8, UX_FALSE},
    { 16, UX_FALSE},
    { 32, UX_FALSE},
    { 64, UX_FALSE},
    {128, UX_FALSE},
    { 20, UX_FALSE},
};
#define N_TESTS (sizeof(tests)/sizeof(struct _test))

    /* Wait device connection.  */
    stepinfo(">>>>>>>>>>>>>>>> Test connect\n");
    status = ux_test_wait_for_value_ulong(&enum_done_count, 1);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    status = ux_test_wait_for_null_wait_time((VOID**)&device, 1000);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    for (i = 0; i < N_TESTS; i ++)
    {
        // printf("Test %d\n", i);

        enum_done_count = 0;
        device_framework_full_speed[7] = tests[i].bMaxPacketSize0;
        device_framework_high_speed[7] = tests[i].bMaxPacketSize0;
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        status = ux_test_wait_for_value_ulong(&enum_done_count, 1);
        if (status != UX_SUCCESS)
        {
            printf("ERROR #%d\n", __LINE__);
            test_control_return(1);
        }

        status = ux_host_stack_device_get(0, &device);
        if (tests[i].expect_no_device)
        {
            if (status == UX_SUCCESS)
            {
                printf("ERROR #%d\n", __LINE__);
                test_control_return(1);
            }
        }
        else
        {
            if (status != UX_SUCCESS)
            {
                printf("ERROR #%d\n", __LINE__);
                test_control_return(1);
            }

            ux_test_dcd_sim_slave_disconnect();
            ux_test_hcd_sim_host_disconnect();
            status = ux_test_wait_for_null_wait_time((VOID**)&device, 3000);
            if (status != UX_SUCCESS)
            {
                printf("ERROR #%d\n", __LINE__);
                test_control_return(1);
            }
        }

        if (_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available != mem_free)
        {
            printf("ERROR #%d\n", __LINE__);
            test_control_return(1);
        }
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

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_DUMMY *dummy_ptr = (UX_HOST_CLASS_DUMMY *) inst;

    // printf("hCHG:%lx,%p,%p\n", event, cls, inst);
    switch(event)
    {

        case UX_DEVICE_INSERTION:

            // printf("hINS:%p,%p:%ld\n", cls, inst, dummy -> ux_host_class_dummy_interface -> ux_interface_descriptor.bInterfaceNumber);
            if (dummy == UX_NULL)
            {
                dummy = dummy_ptr;
                enum_done_count ++;
            }
            break;

        case UX_DEVICE_REMOVAL:

            // printf("hRMV:%p,%p:%ld\n", cls, inst, dummy -> ux_host_class_dummy_interface -> ux_interface_descriptor.bInterfaceNumber);
            if (dummy == dummy_ptr)
                dummy = UX_NULL;
            break;

        case UX_DEVICE_CONNECTION:
            if (device == UX_NULL)
                device = (UX_DEVICE *)inst;
            break;

        case UX_DEVICE_DISCONNECTION:
            if ((VOID *)device == inst)
                device = UX_NULL;
            break;

        default:
            break;
    }
    return 0;
}
