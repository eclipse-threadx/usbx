/* This test is designed to test the ux_utility_descriptor_parse.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_test.h"

#include "ux_pictbridge.h"


/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_MEMORY_SIZE     (64*1024)


/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           error_callback_ignore = UX_FALSE;
static ULONG                           error_callback_counter;

static UCHAR                           element_buffer[UX_PICTBRIDGE_MAX_ELEMENT_SIZE + 8];
static ULONG                           hexa_buffer[UX_PICTBRIDGE_MAX_DEVINFO_ARRAY_SIZE + 8];


/* Define USBX test global variables.  */


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static VOID                ux_test_instance_activate(VOID  *dpump_instance);
static VOID                ux_test_instance_deactivate(VOID *dpump_instance);

static TX_THREAD           ux_test_thread_simulation_0;
static TX_THREAD           ux_test_thread_simulation_1;
static void                ux_test_thread_simulation_0_entry(ULONG);
static void                ux_test_thread_simulation_1_entry(ULONG);


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


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_msrc_81109_pictbridge_array_element_to_hexa_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running MSRC 81109 - Pictbridge array element to hexa Test.......... ");

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
UINT i;

    stepinfo(">>>>>>>>> Normal case\n");
    ux_utility_memory_set(element_buffer, 0, sizeof(element_buffer));
    ux_utility_memory_set(hexa_buffer, 0xFF, sizeof(hexa_buffer));
    strcat(element_buffer, "88 99 10");
    UX_TEST_CHECK_SUCCESS(_ux_pictbridge_array_element_to_array_hexa(element_buffer, hexa_buffer));
    UX_TEST_ASSERT(hexa_buffer[0] == 0x88);
    UX_TEST_ASSERT(hexa_buffer[1] == 0x99);
    UX_TEST_ASSERT(hexa_buffer[2] == 0x10);
    for(i = 3; i < UX_PICTBRIDGE_MAX_DEVINFO_ARRAY_SIZE; i ++) /* Remaining array items are 0.  */
        UX_TEST_ASSERT_MESSAGE(hexa_buffer[i] == 0, "%d:%lx", i, hexa_buffer[i]);
    for (    ; i < sizeof(hexa_buffer)/sizeof(ULONG); i ++)    /* Array outside is not touched.  */
        UX_TEST_ASSERT_MESSAGE(hexa_buffer[i] == 0xFFFFFFFF, "%d:%lx", i, hexa_buffer[i]);

    stepinfo(">>>>>>>>> No delimiter case (expect error)\n");
    ux_utility_memory_set(element_buffer, 0, sizeof(element_buffer));
    ux_utility_memory_set(hexa_buffer, 0x5A, sizeof(hexa_buffer));
    strcat(element_buffer, "111111112222222233333333444444445555555566666666777777778888888899999999aaaaaaaabbbbbbbbccccccccddddddddeeeeeeeeffffffff1111111100000000");
    UX_TEST_CHECK_NOT_SUCCESS(_ux_pictbridge_array_element_to_array_hexa(element_buffer, hexa_buffer));
    for (i = 0; i < sizeof(hexa_buffer)/sizeof(ULONG); i ++)
        UX_TEST_ASSERT_MESSAGE(hexa_buffer[i] == 0x5a5a5a5a, "%d:%lx", i, hexa_buffer[i]);

    stepinfo(">>>>>>>>> Invalid char case (expect error)\n");
    ux_utility_memory_set(element_buffer, 0, sizeof(element_buffer));
    ux_utility_memory_set(hexa_buffer, 0x5A, sizeof(hexa_buffer));
    strcat(element_buffer, "1111 ss");
    UX_TEST_CHECK_NOT_SUCCESS(_ux_pictbridge_array_element_to_array_hexa(element_buffer, hexa_buffer));
    UX_TEST_ASSERT(hexa_buffer[0] == 0x1111);
    UX_TEST_ASSERT(hexa_buffer[1] == 0);
    for (i = 2; i < sizeof(hexa_buffer)/sizeof(ULONG); i ++)
        UX_TEST_ASSERT_MESSAGE(hexa_buffer[i] == 0x5a5a5a5a, "%d:%lx", i, hexa_buffer[i]);

    stepinfo(">>>>>>>>> Too many elements (only first hexas are filled)\n");
    ux_utility_memory_set(element_buffer, 0, sizeof(element_buffer));
    ux_utility_memory_set(hexa_buffer, 0x5A, sizeof(hexa_buffer));
    strcat(element_buffer, "11111111 22222222 33 44 55 66 77 88 99 aaa bbb ccc ddd eeee ffff 00 11 22 33");
    UX_TEST_CHECK_SUCCESS(_ux_pictbridge_array_element_to_array_hexa(element_buffer, hexa_buffer));
    UX_TEST_ASSERT(hexa_buffer[0] == 0x11111111);
    UX_TEST_ASSERT(hexa_buffer[1] == 0x22222222);
    UX_TEST_ASSERT(hexa_buffer[2] == 0x33);
    UX_TEST_ASSERT(hexa_buffer[3] == 0x44);
    UX_TEST_ASSERT(hexa_buffer[4] == 0x55);
    UX_TEST_ASSERT(hexa_buffer[5] == 0x66);
    UX_TEST_ASSERT(hexa_buffer[6] == 0x77);
    UX_TEST_ASSERT(hexa_buffer[7] == 0x88);
    UX_TEST_ASSERT(hexa_buffer[8] == 0x99);
    UX_TEST_ASSERT(hexa_buffer[9] == 0xaaa);
    UX_TEST_ASSERT(hexa_buffer[10] == 0xbbb);
    UX_TEST_ASSERT(hexa_buffer[11] == 0xccc);
    UX_TEST_ASSERT(hexa_buffer[12] == 0xddd);
    UX_TEST_ASSERT(hexa_buffer[13] == 0xeeee);
    UX_TEST_ASSERT(hexa_buffer[14] == 0xffff);
    UX_TEST_ASSERT(hexa_buffer[15] == 0x00);
    for (i = UX_PICTBRIDGE_MAX_DEVINFO_ARRAY_SIZE; i < sizeof(hexa_buffer)/sizeof(ULONG); i ++)    /* Array outside is not touched.  */
        UX_TEST_ASSERT_MESSAGE(hexa_buffer[i] == 0x5a5a5a5a, "%d:%lx", i, hexa_buffer[i]);

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
