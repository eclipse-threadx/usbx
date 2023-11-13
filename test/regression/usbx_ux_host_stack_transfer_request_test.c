/* This test is designed to test the ux_host_stack_transfer_request.  */

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

static UCHAR                           thread_1_state;

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

static VOID test_action_abort(UX_TEST_ACTION *action, VOID *params)
{

    ux_host_stack_transfer_request_abort(&dpump->ux_host_class_dpump_bulk_in_endpoint->ux_endpoint_transfer_request);
}

static UX_TEST_HCD_SIM_ACTION preempt_abort_on_abort[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_ABORT, NULL,
        UX_FALSE, 0,
        UX_TEST_MATCH_EP, 0x81, UX_NULL, 0, 0,
        UX_SUCCESS, test_action_abort,
        UX_TRUE},
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
void    usbx_ux_host_stack_transfer_request_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;
UX_SLAVE_CLASS_DPUMP_PARAMETER  parameter;

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_transfer_request Test......................... ERROR #1\n");
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX.  */
    status =  ux_host_stack_initialize(UX_NULL);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_transfer_request Test......................... ERROR #2\n");
        test_control_return(1);
    }

    /* Register all the host class drivers for this USBX implementation.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_dpump_name, test_ux_host_class_dpump_entry);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_transfer_request Test......................... ERROR #3\n");
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

        printf("Running ux_host_stack_transfer_request Test......................... ERROR #5\n");
        test_control_return(1);
    }

    /* Set the parameters for callback when insertion/extraction of a Data Pump device.  */
    parameter.ux_slave_class_dpump_instance_activate   =  UX_NULL;
    parameter.ux_slave_class_dpump_instance_deactivate =  UX_NULL;

    /* Initialize the device dpump class. The class is connected with interface 0 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_dpump_name, test_ux_device_class_dpump_entry,
                                              1, 0, &parameter);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_transfer_request Test......................... ERROR #6\n");
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_transfer_request Test......................... ERROR #7\n");
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running ux_host_stack_transfer_request Test......................... ERROR #4\n");
        test_control_return(1);
    }

    /* Create the main simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_simulation_0, "test simulation 0", ux_test_thread_simulation_0_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Running ux_host_stack_transfer_request Test......................... ERROR #8\n");
        test_control_return(1);
    }

    /* Create the test simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_simulation_1, "test simulation 1", ux_test_thread_simulation_1_entry, 0,
            stack_pointer + UX_TEST_STACK_SIZE, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_NO_ACTIVATE);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Running ux_host_stack_transfer_request Test......................... ERROR #8\n");
        test_control_return(1);
    }
}


static void  ux_test_thread_simulation_0_entry(ULONG arg)
{
UINT                                                status;
UX_DEVICE                                          *device;
UX_ENDPOINT                                        *control_endpoint;
UX_TRANSFER                                        *transfer_request;
INT                                                 i;

    /* Inform user.  */
    printf("Running ux_host_stack_transfer_request Test......................... ");

    /* Skip ISO EP create/delete. */
    ux_test_hcd_sim_host_set_actions(endpoint0x83_create_del_skip);

    /* Connect. */
    ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);
    ux_test_breakable_sleep(100, break_on_dpump_ready);

    if (dpump == UX_NULL || dpump -> ux_host_class_dpump_state != UX_HOST_CLASS_INSTANCE_LIVE)
    {

        printf("ERROR #%d: dpump not ready\n", __LINE__);
        error_counter ++;
    }

    status = ux_host_stack_device_get(0, &device);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: device_get fail\n", __LINE__);
        test_control_return(1);
    }
    control_endpoint = &device->ux_device_control_endpoint;
    transfer_request = &control_endpoint->ux_endpoint_transfer_request;

    /* Send transfer request when device is not addressed. */

    transfer_request -> ux_transfer_request_data_pointer =      buffer;
    transfer_request -> ux_transfer_request_requested_length =  64;
    transfer_request -> ux_transfer_request_index =             0;

    /* SetAddress(0). */
    transfer_request -> ux_transfer_request_function =          UX_SET_ADDRESS;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             0;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetAddress(0) code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    device->ux_device_state = UX_DEVICE_ATTACHED;

    /* GetDeviceDescriptor. */
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             UX_DEVICE_DESCRIPTOR_ITEM << 8;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: GetDeviceDescriptor() code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* SetAddress(x). */
    transfer_request -> ux_transfer_request_function =          UX_SET_ADDRESS;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             device->ux_device_address;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetAddress(x) code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    device->ux_device_state = UX_DEVICE_ADDRESSED;

    /* SetConfigure(1). */
    transfer_request -> ux_transfer_request_function =          UX_SET_CONFIGURATION;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             1;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d: SetConfigure(1) code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    device->ux_device_state = UX_DEVICE_CONFIGURED;

    /* Simulate semaphore error. */
    ux_test_utility_sim_sem_get_error_generation_start(0);
    /* GetDeviceDescriptor. */
    transfer_request -> ux_transfer_request_function =          UX_GET_DESCRIPTOR;
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_DEVICE;
    transfer_request -> ux_transfer_request_value =             UX_DEVICE_DESCRIPTOR_ITEM << 8;
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: GetDeviceDescriptor() code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    ux_test_utility_sim_sem_get_error_generation_stop();

    /* Start transfer in another thread and abort it.  */
    thread_1_state = 0;

    tx_thread_resume(&ux_test_thread_simulation_1);
    for (i = 0; i < 20; i ++)
    {
        if (thread_1_state > 0)
            break;
        tx_thread_sleep(1);
    }
    if (thread_1_state == 0)
    {
        printf("ERROR #%d: fail to resume thread\n", __LINE__);
        test_control_return(1);
    }

    /* Transfer started, check if it's pending.  */
    for (i = 0; i < 20; i ++)
    {
        if (thread_1_state > 1)
            break;
        tx_thread_sleep(1);
    }
    if (thread_1_state > 1)
    {
        printf("ERROR #%d: thread not pending\n", __LINE__);
        test_control_return(1);
    }
    if (dpump->ux_host_class_dpump_bulk_in_endpoint->ux_endpoint_transfer_request.ux_transfer_request_completion_code != UX_TRANSFER_STATUS_PENDING)
    {
        printf("ERROR #%d: transfer request status not UX_TRANSFER_STATUS_PENDING but 0x%x\n", __LINE__, dpump->ux_host_class_dpump_bulk_in_endpoint->ux_endpoint_transfer_request.ux_transfer_request_completion_code);
        error_counter ++;
    }

    /* Abort it.  */
    status = ux_host_stack_transfer_request_abort(&dpump->ux_host_class_dpump_bulk_in_endpoint->ux_endpoint_transfer_request);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: abort error\n", __LINE__);
        error_counter ++;
    }
    else
    {
        for (i = 0; i < 20; i ++)
        {
            if (thread_1_state > 1)
                break;
            tx_thread_sleep(1);
        }
        if (thread_1_state == 0)
        {
            printf("ERROR #%d: thread not progressing\n", __LINE__);
            error_counter ++;
        }
    }

    /* Disconnect on transfer abort.  */
    ux_test_hcd_sim_host_set_actions(preempt_abort_on_abort);
    thread_1_state = 0;
    tx_thread_resume(&ux_test_thread_simulation_1);
    for (i = 0; i < 20; i ++)
    {
        if (thread_1_state > 0)
            break;
        tx_thread_sleep(1);
    }
    status = ux_host_stack_transfer_request_abort(&dpump->ux_host_class_dpump_bulk_in_endpoint->ux_endpoint_transfer_request);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: abort error\n", __LINE__);
        error_counter ++;
    }

    /* Disconnect. */
    ux_test_hcd_sim_host_disconnect();
    ux_test_breakable_sleep(100, break_on_removal);

    /* Simulate transfer request when device is disconnected.  */
    status = ux_host_stack_transfer_request(transfer_request);
    if (status == UX_SUCCESS)
    {

        printf("ERROR #%d: GetDeviceDescriptor() should fail\n", __LINE__);
        test_control_return(1);
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

static void  ux_test_thread_simulation_1_entry(ULONG arg)
{
UINT  status;
ULONG actual_length;

    while(1)
    {
        thread_1_state ++;

        /* Start transfer. */
        status = ux_host_class_dpump_read(dpump, buffer, 128, &actual_length);

        thread_1_state ++;

        if (dpump->ux_host_class_dpump_bulk_in_endpoint->ux_endpoint_transfer_request.ux_transfer_request_completion_code != UX_TRANSFER_STATUS_ABORT)
        {
            printf("ERROR #%d: expect UX_TRANSFER_STATUS_ABORT but got 0x%x\n", __LINE__, dpump->ux_host_class_dpump_bulk_in_endpoint->ux_endpoint_transfer_request.ux_transfer_request_completion_code);
            error_counter ++;
        }

        tx_thread_suspend(&ux_test_thread_simulation_1);
    }
}