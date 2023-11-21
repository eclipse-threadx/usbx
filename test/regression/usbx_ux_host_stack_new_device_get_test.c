/* This test is designed to test the ux_host_stack_new_device_get.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_host_class_dpump.h"
#include "ux_device_class_dpump.h"


/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_RUN             1
#define UX_TEST_MEMORY_SIZE     (64*1024)


/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;


/* Define USBX test global variables.  */

static unsigned char                   host_out_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];
static unsigned char                   host_in_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];
static unsigned char                   slave_buffer[UX_HOST_CLASS_DPUMP_PACKET_SIZE];


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static VOID                ux_test_instance_activate(VOID  *dpump_instance);             
static VOID                ux_test_instance_deactivate(VOID *dpump_instance);


static TX_THREAD           ux_test_thread_host_simulation;
static TX_THREAD           ux_test_thread_slave_simulation;
static void                ux_test_thread_host_simulation_entry(ULONG);
static void                ux_test_thread_slave_simulation_entry(ULONG);

extern  UX_DEVICE  *_ux_host_stack_new_device_get(VOID);

/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_stack_new_device_get_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;


    /* Inform user.  */
    printf("Running ux_host_stack_new_device_get Test........................... ");

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);
    
    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #1\n");
        test_control_return(1);
    }

    /* The code below is required for installing the host portion of USBX.  */
    status =  ux_host_stack_initialize(UX_NULL);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #2\n");
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_host_simulation, "test host simulation", ux_test_thread_host_simulation_entry, 0,  
            stack_pointer, UX_TEST_STACK_SIZE, 
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #3\n");
        test_control_return(1);
    }


}


static void  ux_test_thread_host_simulation_entry(ULONG arg)
{

UX_DEVICE                       *new_device;
UINT                            i;


    /* Test ux_host_stack_new_device_get. */
    for (i = UX_SYSTEM_HOST_MAX_DEVICES_GET(); i != 0; i--)
    {
        new_device =  _ux_host_stack_new_device_get();
        if (new_device == UX_NULL)
        {
        
            printf("ERROR #4\n");
            test_control_return(1);
        }
    }

    /* There is no device entry available now. Try to get new device again.  */
    new_device =  _ux_host_stack_new_device_get();

    /* Check if UX_NULL is returned.  */
    if (new_device != UX_NULL)
    {

        printf("ERROR #5\n");
        test_control_return(1);
    }
    else
    {

        /* Successful test.  */
        printf("SUCCESS!\n");
        test_control_return(0);
    }

    /* Sleep for a tick to make sure everything is complete.  */
    tx_thread_sleep(1);
}