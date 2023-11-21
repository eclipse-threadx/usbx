/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_device_class_printer.h"

#include "ux_test.h"

/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_MEMORY_SIZE     (64*1024)


/* Define the counters used in the test application...  */

static ULONG                           error_counter = 0;

/* Define USBX test global variables.  */


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static TX_THREAD           ux_test_thread_simulation_0;
static void                ux_test_thread_simulation_0_entry(ULONG);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_uxe_device_printer_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_device_printer APIs Test.................................. ");
#if !defined(UX_DEVICE_CLASS_PRINTER_ENABLE_ERROR_CHECKING)
#warning Tests skipped due to compile option!
    printf("SKIP SUCCESS!\n");
    test_control_return(0);
    return;
#endif

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
UINT                                    status;
UX_DEVICE_CLASS_PRINTER                 dummy_printer_inst;
UX_DEVICE_CLASS_PRINTER                 *dummy_printer = &dummy_printer_inst;
UCHAR                                   dummy_buffer[32];
ULONG                                   dummy_actual_length;
UX_SLAVE_CLASS_COMMAND                  dummy_command;
UX_DEVICE_CLASS_PRINTER_PARAMETER       dummy_printer_parameter;

#if !defined(UX_DEVICE_STANDALONE)
/* Device printer device ID.  */
UCHAR printer_device_id[] =
 {
    "  "                                // Will be replaced by length (big endian)
    "MFG:Generic;"                      //   manufacturer (case sensitive)
    "MDL:Generic_/_Text_Only;"          //   model (case sensitive)
    "CMD:1284.4;"                       //   PDL command set
    "CLS:PRINTER;"                      //   class
    "DES:Generic text only printer;"    //   description
 };

    dummy_printer_parameter.ux_device_class_printer_device_id = printer_device_id;
    dummy_command.ux_slave_class_command_parameter = &dummy_printer_parameter;

    /* ux_device_class_printer_entry()  */
    dummy_command.ux_slave_class_command_request = UX_SLAVE_CLASS_COMMAND_INITIALIZE;

    printer_device_id[0] = 2;
    printer_device_id[1] = 1;
    status = ux_device_class_printer_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_printer_write()  */
    status = ux_device_class_printer_write(UX_NULL, dummy_buffer, 32, &dummy_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_printer_write(dummy_printer, UX_NULL, 32, &dummy_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_printer_write(dummy_printer, dummy_buffer, 32, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_printer_read()  */
    status = ux_device_class_printer_read(UX_NULL, dummy_buffer, 32, &dummy_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_printer_read(dummy_printer, UX_NULL, 32, &dummy_actual_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_printer_read(dummy_printer, dummy_buffer, 32, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_printer_ioctl()  */
    status = ux_device_class_printer_ioctl(UX_NULL, UX_DEVICE_CLASS_PRINTER_IOCTL_PORT_STATUS_SET, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
#else

   /* ux_device_class_printer_read_run()  */
    status = ux_device_class_printer_read_run(UX_NULL, dummy_buffer, 32, &dummy_actual_length);
    UX_TEST_CHECK_CODE(UX_STATE_ERROR, status);

    status = ux_device_class_printer_read_run(&dummy_printer_inst, UX_NULL, 32, &dummy_actual_length);
    UX_TEST_CHECK_CODE(UX_STATE_ERROR, status);

    status = ux_device_class_printer_read_run(&dummy_printer_inst, dummy_buffer, 32, UX_NULL);
    UX_TEST_CHECK_CODE(UX_STATE_ERROR, status);

    /* ux_device_class_cdc_acm_write_run()  */
    status = ux_device_class_printer_write_run(UX_NULL, dummy_buffer, 32, &dummy_actual_length);
    UX_TEST_CHECK_CODE(UX_STATE_ERROR, status);

    status = ux_device_class_printer_write_run(&dummy_printer_inst, UX_NULL, 32, &dummy_actual_length);
    UX_TEST_CHECK_CODE(UX_STATE_ERROR, status);

    status = ux_device_class_printer_write_run(&dummy_printer_inst, dummy_buffer, 32, UX_NULL);
    UX_TEST_CHECK_CODE(UX_STATE_ERROR, status);

#endif

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
