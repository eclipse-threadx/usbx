/* This test is designed to test the ux_utility_unicode_to_string.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_device_class_cdc_acm.h"
#include "ux_host_class_cdc_acm.h"

#include "ux_host_class_dpump.h"
#include "ux_device_class_dpump.h"

#include "ux_host_class_hid.h"
#include "ux_device_class_hid.h"

#include "ux_host_class_storage.h"
#include "ux_device_class_storage.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"


/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_RUN             1
#define UX_TEST_MEMORY_SIZE     (64*1024)

#define     LSB(x) ( (x) & 0x00ff)
#define     MSB(x) (((x) & 0xff00) >> 8)

/* Configuration descriptor 9 bytes */
#define CFG_DESC(wTotalLength, bNumInterfaces, bConfigurationValue)\
    /* Configuration 1 descriptor 9 bytes */\
    0x09, 0x02, LSB(wTotalLength), MSB(wTotalLength),\
    (bNumInterfaces), (bConfigurationValue), 0x00,\
    0x40, 0x00,
#define CFG_DESC_LEN 9

/* DPUMP interface descriptors. */
#define DPUMP_IFC_DESC(ifc, alt, nb_ep) \
    /* Interface descriptor */\
    0x09, 0x04, (ifc), (alt), (nb_ep), 0x99, 0x99, 0x99, 0x00,

#define DPUMP_IFC_EP_DESC(epaddr, eptype, epsize) \
    /* Endpoint descriptor */\
    0x07, 0x05, (epaddr), (eptype), LSB(epsize), MSB(epsize), 0x01,

#define DPUMP_IFC_DESC_ALL_LEN(nb_ep) (9 + (nb_ep) * 7)

#define CFG_DESC_ALL_LEN (CFG_DESC_LEN + DPUMP_IFC_DESC_ALL_LEN(4))

#define CFG_DESC_ALL \
    CFG_DESC(CFG_DESC_ALL_LEN, 1, 1)\
    DPUMP_IFC_DESC(0, 0, 4)\
    DPUMP_IFC_EP_DESC(0x81, 2, 64)\
    DPUMP_IFC_EP_DESC(0x02, 2, 64)\
    DPUMP_IFC_EP_DESC(0x83, 1, 64)\
    DPUMP_IFC_EP_DESC(0x84, 3, 64)\

/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           error_callback_ignore = UX_FALSE;
static ULONG                           error_callback_counter;

static UCHAR                           buffer[UX_TEST_BUFFER_SIZE];

/* Define USBX test global variables.  */

static UX_HOST_CLASS                   *class_driver;
static UX_HOST_CLASS_DPUMP             *dpump;
static UX_SLAVE_CLASS_DPUMP            *dpump_slave = UX_NULL;

static UCHAR device_framework_full_speed[] = {

    /* Device descriptor 18 bytes */
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
    0xec, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01,

    CFG_DESC_ALL
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

    CFG_DESC_ALL
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
    0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
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

static VOID                ux_test_instance_activate(VOID  *dpump_instance);
static VOID                ux_test_instance_deactivate(VOID *dpump_instance);

UINT                       _ux_host_class_dpump_entry(UX_HOST_CLASS_COMMAND *command);
UINT                       ux_hcd_sim_initialize(UX_HCD *hcd);
UINT                       _ux_host_class_dpump_write(UX_HOST_CLASS_DPUMP *dpump, UCHAR * data_pointer,
                                    ULONG requested_length, ULONG *actual_length);
UINT                       _ux_host_class_dpump_read (UX_HOST_CLASS_DPUMP *dpump, UCHAR *data_pointer,
                                    ULONG requested_length, ULONG *actual_length);

static TX_THREAD           ux_test_thread_simulation_0;
static TX_THREAD           ux_test_thread_simulation_1;
static void                ux_test_thread_simulation_0_entry(ULONG);
static void                ux_test_thread_simulation_1_entry(ULONG);


/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);

/* Simulator actions. */

static UX_TEST_HCD_SIM_ACTION endpoint0x83_create_del_skip[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_CREATE_ENDPOINT, NULL,
        UX_FALSE, 0,
        UX_TEST_MATCH_EP, 0x83, UX_NULL, 0, 0,
        UX_SUCCESS},
{   UX_HCD_CREATE_ENDPOINT, NULL,
        UX_FALSE, 0,
        UX_TEST_MATCH_EP, 0x83, UX_NULL, 0, 0,
        UX_SUCCESS},
{   0   }
};

/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}


static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        {
            /* Failed test.  */
            printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
            // test_control_return(1);
        }
    }
}

static UINT break_on_dpump_ready(VOID)
{

UINT             status;
UX_HOST_CLASS   *class;

    /* Find the main data pump container.  */
    status =  ux_host_stack_class_get(_ux_system_host_class_dpump_name, &class);
    if (status != UX_SUCCESS)
        /* Do not break. */
        return UX_SUCCESS;

    /* Find the instance. */
    status =  ux_host_stack_class_instance_get(class, 0, (VOID **) &dpump);
    if (status != UX_SUCCESS)
        /* Do not break. */
        return UX_SUCCESS;

    if (dpump -> ux_host_class_dpump_state != UX_HOST_CLASS_INSTANCE_LIVE)
        /* Do not break. */
        return UX_SUCCESS;

    return 1;
}

static UINT break_on_removal(VOID)
{

UINT                     status;
UX_DEVICE               *device;

    status = ux_host_stack_device_get(0, &device);
    if (status == UX_SUCCESS)
        /* Do not break. */
        return UX_SUCCESS;

    return 1;
}


static UINT test_ux_device_class_dpump_entry(UX_SLAVE_CLASS_COMMAND *command)
{
    switch(command->ux_slave_class_command_request)
    {
        case UX_SLAVE_CLASS_COMMAND_INITIALIZE:
        case UX_SLAVE_CLASS_COMMAND_QUERY:
        case UX_SLAVE_CLASS_COMMAND_CHANGE:
            return UX_SUCCESS;

        default:
            return UX_NO_CLASS_MATCH;
    }
}

static UINT test_ux_host_class_dpump_entry(UX_HOST_CLASS_COMMAND *command)
{
    switch (command -> ux_host_class_command_request)
    {
        case UX_HOST_CLASS_COMMAND_QUERY:
        default:
            return _ux_host_class_dpump_entry(command);
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_utility_unicode_to_string_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running ux_utility_unicode_to_string Test (and string_to_unicode)... ");

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

    /* Create the simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_simulation_0, "test simulation", ux_test_thread_simulation_0_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}


static void  ux_test_thread_simulation_0_entry(ULONG arg)
{


#define     TEST_STR     "12345"
#define     TEST_STR_LEN (sizeof(TEST_STR)) /* Including null terminator */
UCHAR       test_str[] = TEST_STR;
UCHAR       s_buffer[16];
UCHAR       u_buffer[16 * 2 + 1];
INT         i, j;

    _ux_utility_memory_set(s_buffer, 0xFF, sizeof(s_buffer));
    _ux_utility_memory_set(u_buffer, 0xFF, sizeof(u_buffer));

    _ux_utility_string_to_unicode(test_str, (UCHAR *)u_buffer);
    if (u_buffer[0] != TEST_STR_LEN)
    {
        printf("ERROR #%d: expected 0x%x but got 0x%x\n", __LINE__, TEST_STR_LEN, u_buffer[0]);
    }
    for (i = 0, j = 1; i < TEST_STR_LEN; i ++, j += 2)
    {
        if (test_str[i] != u_buffer[j])
        {
            printf("ERROR #%d: expected 0x%x but got 0x%x\n", __LINE__, test_str[i], u_buffer[j]);
        }
        if (0x0 != u_buffer[j + 1])
        {
            printf("ERROR #%d: expected 0x%x but got 0x%x\n", __LINE__, 0, u_buffer[j + 1]);
        }
    }
    for (; j < sizeof(u_buffer); j ++)
    {
        if (0xFF != u_buffer[j])
        {
            printf("ERROR #%d: expected 0x%x but got 0x%x\n", __LINE__, 0xFF, s_buffer[j]);
        }
    }

    /* Modify unicode length to exclude null-terminator. */
    /* Here additional null terminator always pend to string end. */
    *u_buffer = (*u_buffer) - 1;
    _ux_utility_unicode_to_string((UCHAR *)u_buffer, s_buffer);
    for (i = 0; i < TEST_STR_LEN; i ++)
    {
        if (test_str[i] != s_buffer[i])
        {
            printf("ERROR #%d: expected 0x%x but got 0x%x\n", __LINE__, test_str[i], s_buffer[i]);
        }
    }
    for (; i < sizeof(s_buffer); i ++)
    {
        if (0xFF != s_buffer[i])
        {
            printf("ERROR #%d: expected 0x%x but got 0x%x\n", __LINE__, 0xFF, s_buffer[i]);
        }
    }

    /* Sleep for a tick to make sure everything is complete.  */
    tx_thread_sleep(1);

    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* Test error.  */
        printf("ERROR #%d: total %ld errors\n", __LINE__, error_counter);
        test_control_return(1);
    }
    else
    {

        /* Successful test.  */
        printf("SUCCESS!\n");
        test_control_return(0);
    }
}
