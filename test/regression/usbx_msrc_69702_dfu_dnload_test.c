/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"

#include "fx_api.h"

#include "ux_device_class_dfu.h"
#include "ux_device_stack.h"
#include "ux_host_stack.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"


#define UX_DEMO_REQUEST_MAX_LENGTH                                                 \
    ((UX_HCD_SIM_HOST_MAX_PAYLOAD) > (UX_SLAVE_REQUEST_DATA_MAX_LENGTH) ?         \
     (UX_HCD_SIM_HOST_MAX_PAYLOAD) : (UX_SLAVE_REQUEST_DATA_MAX_LENGTH))


/* Define constants.  */

#define  UX_DEMO_MEMORY_SIZE             (128*1024)
#define  UX_DEMO_STACK_SIZE              (1024)


/* Define local/extern function prototypes.  */

static void        test_thread_entry(ULONG);
static TX_THREAD   tx_test_thread_host_simulation;
static TX_THREAD   tx_test_thread_slave_simulation;
static void        tx_test_thread_host_simulation_entry(ULONG);
static void        tx_test_thread_slave_simulation_entry(ULONG);

static VOID        demo_thread_dfu_activate(VOID *dfu);
static VOID        demo_thread_dfu_deactivate(VOID *dfu);
static UINT        demo_thread_dfu_read(VOID *dfu, ULONG block_number, UCHAR * data_pointer, ULONG length, ULONG *actual_length);
static UINT        demo_thread_dfu_write(VOID *dfu, ULONG block_number, UCHAR * data_pointer, ULONG length, ULONG *media_status);
static UINT        demo_thread_dfu_get_status(VOID *dfu, ULONG *media_status);
static UINT        demo_thread_dfu_notify(VOID *dfu, ULONG notification);
static UINT        demo_thread_dfu_custom_request(VOID *dfu, UX_SLAVE_TRANSFER *transfer);

/* Define global data structures.  */

static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

static ULONG                               error_counter;

static ULONG                               set_cfg_counter;

static ULONG                               rsc_mem_free_on_set_cfg;
static ULONG                               rsc_sem_on_set_cfg;
static ULONG                               rsc_sem_get_on_set_cfg;
static ULONG                               rsc_mutex_on_set_cfg;

static ULONG                               rsc_enum_sem_usage;
static ULONG                               rsc_enum_sem_get_count;
static ULONG                               rsc_enum_mutex_usage;
static ULONG                               rsc_enum_mem_usage;

static ULONG                               interaction_count;

static UCHAR                               error_callback_ignore = UX_TRUE;
static ULONG                               error_callback_counter;

static UX_SLAVE_CLASS_DFU_PARAMETER        dfu_parameter;

static UX_DEVICE                           *device;
static ULONG                               dfu_block;
static ULONG                               dfu_transfer_length;
static ULONG                               dfu_actual_length;
static UCHAR                               dfu_host_buffer[UX_DEMO_REQUEST_MAX_LENGTH];
static UCHAR                               dfu_device_buffer[UX_DEMO_REQUEST_MAX_LENGTH];


/* Define device framework.  */

/* DFU descriptor must be same for all frameworks!!!  */
#define DFU_FUNCTION_DESCRIPTOR                                                 \
    /* Functional descriptor for DFU.  */                                       \
    0x09, 0x21,                                                                 \
    0x0f,       /* bmAttributes: B3 bitWillDetach                   */          \
                /*               B2 bitManifestationTolerant        */          \
                /*               B1 bitCanUpload, B0 bitCanDnload   */          \
    0xE8, 0x03, /* wDetachTimeOut: 0x03E8 (1000) */                             \
    UX_W0(UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH),                                 \
    UX_W1(UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH), /* wTransferSize:   */          \
    0x00, 0x01, /* bcdDFUVersion:  0x0100  */

/* Interface descriptor for APP/DFU mode.  */
#define DFU_INTERFACE_DESCRIPTOR(bInterfaceNumber, bInterfaceProtocol)          \
    /* Interface descriptor for DFU.  */                                        \
    0x09, 0x04,                                                                 \
    (bInterfaceNumber), 0x00, 0x00,                                             \
    0xFE, 0x01, (bInterfaceProtocol),                                           \
    0x00,                                                                       \

#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)
static UCHAR device_framework_full_speed[] = { 

    /* Device descriptor */
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x40,
    0x99, 0x99, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
    0x03, 0x01,                                      

    /* Configuration descriptor */
    0x09, 0x02, 0x1b, 0x00, 0x01, 0x01, 0x00, 0xc0,
    0x32, 

    /* Interface descriptor for DFU (bInterfaceProtocol = 1).  */
    DFU_INTERFACE_DESCRIPTOR(0x00, 0x01)
    DFU_FUNCTION_DESCRIPTOR
};

#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)
static UCHAR device_framework_high_speed[] = { 

    /* Device descriptor */
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x99, 0x99, 0x00, 0x00, 0x01, 0x00, 0x01, 0x02,
    0x03, 0x01,                                      

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x01, 0x00,

    /* Configuration descriptor */
    0x09, 0x02, 0x1b, 0x00, 0x01, 0x01, 0x00, 0xc0,
    0x32, 

    /* Interface descriptor for DFU (bInterfaceProtocol = 1).  */
    DFU_INTERFACE_DESCRIPTOR(0x00, 0x01)
    DFU_FUNCTION_DESCRIPTOR
};

/* String Device Framework :
    Byte 0 and 1 : Word containing the language ID : 0x0904 for US
    Byte 2       : Byte containing the index of the descriptor
    Byte 3       : Byte containing the length of the descriptor string
*/
#define STRING_FRAMEWORK_LENGTH sizeof(string_framework)
static UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 - "Microsoft AzureRTOS" */
    0x09, 0x04, 0x01, 19,
    'M', 'i', 'c', 'r', 'o', 's', 'o', 'f',
    't', ' ', 'A', 'z', 'u', 'r', 'e', 'R',
    'T', 'O', 'S',

    /* Product string descriptor : Index 2 - "DFU Demo Device" */
    0x09, 0x04, 0x02, 15,
    'D', 'F', 'U', ' ', 'D', 'e', 'm', 'o',
    ' ', 'D', 'e', 'v', 'i', 'c', 'e',

    /* Serial Number string descriptor : Index 3 - "0000" */
    0x09, 0x04, 0x03, 0x04,
    '0', '0', '0', '0'
};

/* Multiple languages are supported on the device, to add
    a language besides english, the unicode language code must
    be appended to the language_id_framework array and the length
    adjusted accordingly. */
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(language_id_framework)
static UCHAR language_id_framework[] = {

    /* English. */
    0x09, 0x04
};


#define DEVICE_FRAMEWORK_LENGTH_DFU sizeof(device_framework_dfu)
static UCHAR device_framework_dfu[] = { 

    /* Device descriptor */
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x40,
    0x99, 0x99, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
    0x03, 0x01,                                      

    /* Configuration descriptor */
    0x09, 0x02, 0x1B, 0x00, 0x01, 0x01, 0x00, 0xc0,
    0x32, 

    /* Interface descriptor for DFU (bInterfaceProtocol = 2).  */
    DFU_INTERFACE_DESCRIPTOR(0x00, 0x02)
    DFU_FUNCTION_DESCRIPTOR
};


/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        {
            /* Failed test.  */
            printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
            test_control_return(1);
        }
    }
}


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}


static UINT demo_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{
    if (event == UX_DEVICE_CONNECTION)
    {
        device = (UX_DEVICE *)inst;
    }
    if (event == UX_DEVICE_DISCONNECTION)
    {
        if ((VOID *)device == inst)
            device = UX_NULL;
    }
}


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_device_dfu_basic_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;
ULONG                   test_n;

    /* Inform user.  */
    printf("Running MSRC 69702: Device DFU DNLOAD Test.......................... ");

    /* Reset testing counts. */
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    /* Reset error generations */
    ux_test_utility_sim_sem_error_generation_stop();
    ux_test_utility_sim_mutex_error_generation_stop();
    ux_test_utility_sim_sem_get_error_generation_stop();

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(demo_system_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* There is no host class for DFU now.  */

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Store the DFU parameters.   */
    dfu_parameter.ux_slave_class_dfu_parameter_instance_activate               =  demo_thread_dfu_activate;
    dfu_parameter.ux_slave_class_dfu_parameter_instance_deactivate             =  demo_thread_dfu_deactivate;
    dfu_parameter.ux_slave_class_dfu_parameter_read                            =  demo_thread_dfu_read;
    dfu_parameter.ux_slave_class_dfu_parameter_write                           =  demo_thread_dfu_write; 
    dfu_parameter.ux_slave_class_dfu_parameter_get_status                      =  demo_thread_dfu_get_status;
    dfu_parameter.ux_slave_class_dfu_parameter_notify                          =  demo_thread_dfu_notify;
#ifdef UX_DEVICE_CLASS_DFU_CUSTOM_REQUEST_ENABLE
    dfu_parameter.ux_device_class_dfu_parameter_custom_request                 =  demo_thread_dfu_custom_request;
#endif
    dfu_parameter.ux_slave_class_dfu_parameter_framework                       =  device_framework_dfu;
    dfu_parameter.ux_slave_class_dfu_parameter_framework_length                =  DEVICE_FRAMEWORK_LENGTH_DFU;

    /* Initilize the device dfu class. The class is connected with interface 1 on configuration 1. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_dfu_name, ux_device_class_dfu_entry, 
                                             1, 0, (VOID *)&dfu_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_test_thread_host_simulation, "tx demo host simulation", tx_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&tx_test_thread_slave_simulation, "tx demo slave simulation", tx_test_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}

static UINT _req_DFU_LOCK(UX_TRANSFER *control_transfer)
{
UINT        status;
#if defined(UX_HOST_STANDALONE)
    while(1)
    {
        ux_system_tasks_run();
        tx_thread_relinquish();

        UX_ENDPOINT *endpoint = control_transfer -> ux_transfer_request_endpoint;
        if (endpoint == UX_NULL || endpoint -> ux_endpoint_state != UX_ENDPOINT_RUNNING)
        {
            status = UX_ENDPOINT_HANDLE_UNKNOWN;
            break;
        }
        UX_DEVICE *device = endpoint -> ux_endpoint_device;
        if (device == UX_NULL || device -> ux_device_handle != (ULONG)(ALIGN_TYPE)(device))
        {
            status = UX_DEVICE_HANDLE_UNKNOWN;
            break;
        }
        if ((device -> ux_device_flags & UX_DEVICE_FLAG_LOCK) == 0)
        {
            device -> ux_device_flags |= UX_DEVICE_FLAG_LOCK;
            control_transfer -> ux_transfer_request_flags |= UX_TRANSFER_FLAG_AUTO_DEVICE_UNLOCK;
            control_transfer -> ux_transfer_request_timeout_value = UX_WAIT_FOREVER;
            status = UX_SUCCESS;
            break;
        }
    }
#else
    status = _ux_utility_semaphore_get(&control_transfer->ux_transfer_request_endpoint->ux_endpoint_device->ux_device_protection_semaphore, UX_WAIT_FOREVER);
#endif
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: %x\n", __LINE__, status);
        test_control_return(1);
    }
}
static UINT _req_DFU_GETSTATE(UX_TRANSFER *control_transfer)
{
    _req_DFU_LOCK(control_transfer);
    control_transfer->ux_transfer_request_type              = 0xA1;
    control_transfer->ux_transfer_request_function          = UX_SLAVE_CLASS_DFU_COMMAND_GET_STATE;
    control_transfer->ux_transfer_request_index             = 0;
    control_transfer->ux_transfer_request_requested_length  = 1;
    control_transfer->ux_transfer_request_data_pointer      = dfu_host_buffer;
    control_transfer->ux_transfer_request_value             = 0;
    return ux_host_stack_transfer_request(control_transfer);
}
static UINT _req_DFU_GETSTATUS(UX_TRANSFER *control_transfer)
{
    _req_DFU_LOCK(control_transfer);
    control_transfer->ux_transfer_request_type              = 0xA1;
    control_transfer->ux_transfer_request_function          = UX_SLAVE_CLASS_DFU_COMMAND_GET_STATUS;
    control_transfer->ux_transfer_request_index             = 0;
    control_transfer->ux_transfer_request_requested_length  = 6;
    control_transfer->ux_transfer_request_data_pointer      = dfu_host_buffer;
    control_transfer->ux_transfer_request_value             = 0;
    return ux_host_stack_transfer_request(control_transfer);
}
static UINT _req_DFU_DETACH(UX_TRANSFER *control_transfer)
{
    _req_DFU_LOCK(control_transfer);
    control_transfer->ux_transfer_request_type              = 0x21;
    control_transfer->ux_transfer_request_function          = UX_SLAVE_CLASS_DFU_COMMAND_DETACH;
    control_transfer->ux_transfer_request_index             = 0;
    control_transfer->ux_transfer_request_value             = 1000;
    control_transfer->ux_transfer_request_requested_length  = 0;
    return ux_host_stack_transfer_request(control_transfer);
}
static UINT _req_DFU_DNLOAD_IN(UX_TRANSFER *control_transfer, ULONG block, ULONG len)
{
    _req_DFU_LOCK(control_transfer);
    control_transfer->ux_transfer_request_type              = 0xA1;
    control_transfer->ux_transfer_request_function          = UX_SLAVE_CLASS_DFU_COMMAND_DOWNLOAD;
    control_transfer->ux_transfer_request_index             = 0;
    control_transfer->ux_transfer_request_data_pointer      = dfu_host_buffer;
    control_transfer->ux_transfer_request_requested_length  = len;
    control_transfer->ux_transfer_request_value             = block;
    return ux_host_stack_transfer_request(control_transfer);
}
static UINT _req_DFU_DNLOAD(UX_TRANSFER *control_transfer, ULONG block, ULONG len)
{
    _req_DFU_LOCK(control_transfer);
    control_transfer->ux_transfer_request_type              = 0x21;
    control_transfer->ux_transfer_request_function          = UX_SLAVE_CLASS_DFU_COMMAND_DOWNLOAD;
    control_transfer->ux_transfer_request_index             = 0;
    control_transfer->ux_transfer_request_data_pointer      = dfu_host_buffer;
    control_transfer->ux_transfer_request_requested_length  = len;
    control_transfer->ux_transfer_request_value             = block;
    return ux_host_stack_transfer_request(control_transfer);
}
static UINT _req_DFU_UPLOAD(UX_TRANSFER *control_transfer, ULONG block, ULONG len)
{
    _req_DFU_LOCK(control_transfer);
    control_transfer->ux_transfer_request_type              = 0xA1;
    control_transfer->ux_transfer_request_function          = UX_SLAVE_CLASS_DFU_COMMAND_UPLOAD;
    control_transfer->ux_transfer_request_index             = 0;
    control_transfer->ux_transfer_request_data_pointer      = dfu_host_buffer;
    control_transfer->ux_transfer_request_requested_length  = len;
    control_transfer->ux_transfer_request_value             = block;
    return ux_host_stack_transfer_request(control_transfer);
}
static UINT _req_DFU_CLRSTATUS(UX_TRANSFER *control_transfer)
{
    _req_DFU_LOCK(control_transfer);
    control_transfer->ux_transfer_request_type              = 0x21;
    control_transfer->ux_transfer_request_function          = UX_SLAVE_CLASS_DFU_COMMAND_CLEAR_STATUS;
    control_transfer->ux_transfer_request_index             = 0;
    control_transfer->ux_transfer_request_data_pointer      = UX_NULL;
    control_transfer->ux_transfer_request_requested_length  = 0;
    control_transfer->ux_transfer_request_value             = 0;
    return ux_host_stack_transfer_request(control_transfer);
}


static void  tx_test_thread_host_simulation_entry(ULONG arg)
{
UX_ENDPOINT             *control_endpoint;
UX_TRANSFER             *control_transfer;
ULONG                   len, trans_len, block;
INT                     i;
UINT                    status;

    stepinfo("\n");

    stepinfo(">>>>>>>>>>>> Test DFU connect\n");
    status = ux_test_wait_for_non_null((VOID **)&device);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    if (device -> ux_device_state == UX_DEVICE_CONFIGURED)
    {
        printf("ERROR #%d, device state 0x%lx\n", __LINE__, device->ux_device_state);
        test_control_return(1);
    }

    stepinfo(">>>>>>>>>>>> Test DFU set configure\n");
    status = ux_host_stack_device_configuration_select(device->ux_device_first_configuration);
    UX_TEST_ASSERT(status == UX_SUCCESS);

    /* Get endpoint and transfer request.  */
    control_endpoint = &device->ux_device_control_endpoint;
    control_transfer = &control_endpoint->ux_endpoint_transfer_request;

    stepinfo(">>>>>>>>>>>> Test DFU_GETSTATE\n");
    status = _req_DFU_GETSTATE(control_transfer);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: transfer status 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    UX_TEST_ASSERT(dfu_host_buffer[0] == UX_SYSTEM_DFU_STATE_APP_IDLE);

    stepinfo(">>>>>>>>>>>> Test DFU DETACH\n");

    /* Uses DFU framework after USB reset (re-connect).  */
    ux_test_dcd_sim_slave_connect_framework(device_framework_dfu, DEVICE_FRAMEWORK_LENGTH_DFU);
    status = _req_DFU_DETACH(control_transfer);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: transfer status 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    status = ux_test_wait_for_null((VOID **)&device);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    status = ux_test_wait_for_non_null((VOID **)&device);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    status = ux_host_stack_device_configuration_select(device->ux_device_first_configuration);
    UX_TEST_ASSERT(status == UX_SUCCESS);

    status = _req_DFU_GETSTATE(control_transfer);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: transfer status 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    UX_TEST_ASSERT(dfu_host_buffer[0] == UX_SYSTEM_DFU_STATE_DFU_IDLE);

    stepinfo(">>>>>>>>>>>> Test DFU DNLOAD direction error\n");
    status = _req_DFU_DNLOAD_IN(control_transfer, 0, 16);
    UX_TEST_CHECK_CODE(UX_TRANSFER_STALLED, status);
    status = _req_DFU_CLRSTATUS(control_transfer);
    UX_TEST_CHECK_SUCCESS(status);
    status = _req_DFU_GETSTATE(control_transfer);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: transfer status 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    UX_TEST_ASSERT(dfu_host_buffer[0] == UX_SYSTEM_DFU_STATE_DFU_IDLE);

    stepinfo(">>>>>>>>>>>> Test DFU DNLOAD length\n");
    status = _req_DFU_DNLOAD(control_transfer, 0, UX_DEMO_REQUEST_MAX_LENGTH);
    UX_TEST_CHECK_CODE(UX_TRANSFER_STALLED, status);
    status = _req_DFU_CLRSTATUS(control_transfer);
    UX_TEST_CHECK_SUCCESS(status);
    status = _req_DFU_GETSTATE(control_transfer);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: transfer status 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    UX_TEST_ASSERT(dfu_host_buffer[0] == UX_SYSTEM_DFU_STATE_DFU_IDLE);

    stepinfo(">>>>>>>>>>>> Test DFU DNLOAD direction error @ DNLOAD_IDLE\n");
    trans_len = 2; block = 0;
    for (i = 0; i < trans_len; i ++)
    {
        dfu_host_buffer[i] = (UCHAR)(block + i);
        dfu_device_buffer[i] = 0xFF;
    }
    status = _req_DFU_DNLOAD(control_transfer, block, trans_len);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d(%ld, %ld): DNLOAD status 0x%x\n", __LINE__, block, trans_len, status);
        test_control_return(1);
    }
    UX_TEST_ASSERT(dfu_transfer_length == trans_len);
    UX_TEST_ASSERT(dfu_block == block);
    if (ux_utility_memory_compare(dfu_host_buffer, dfu_device_buffer, trans_len) != UX_SUCCESS)
    {
        printf("ERROR #%d(%ld, %ld): data error\n", __LINE__, block, trans_len);
        test_control_return(1);
    }
    status = _req_DFU_GETSTATUS(control_transfer);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d(%ld, %ld): GETSTATUS status 0x%x\n", __LINE__, block, trans_len, status);
        test_control_return(1);
    }
    status = _req_DFU_DNLOAD_IN(control_transfer, 0, 16);
    UX_TEST_CHECK_CODE(UX_TRANSFER_STALLED, status);
    status = _req_DFU_CLRSTATUS(control_transfer);
    UX_TEST_CHECK_SUCCESS(status);
    status = _req_DFU_GETSTATE(control_transfer);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: transfer status 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    UX_TEST_ASSERT(dfu_host_buffer[0] == UX_SYSTEM_DFU_STATE_DFU_IDLE);

    stepinfo(">>>>>>>>>>>> Test DFU DNLOAD length error @ DNLOAD_IDLE\n");
    trans_len = 2; block = 0;
    for (i = 0; i < trans_len; i ++)
    {
        dfu_host_buffer[i] = (UCHAR)(block + i);
        dfu_device_buffer[i] = 0xFF;
    }
    status = _req_DFU_DNLOAD(control_transfer, block, trans_len);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d(%ld, %ld): DNLOAD status 0x%x\n", __LINE__, block, trans_len, status);
        test_control_return(1);
    }
    UX_TEST_ASSERT(dfu_transfer_length == trans_len);
    UX_TEST_ASSERT(dfu_block == block);
    if (ux_utility_memory_compare(dfu_host_buffer, dfu_device_buffer, trans_len) != UX_SUCCESS)
    {
        printf("ERROR #%d(%ld, %ld): data error\n", __LINE__, block, trans_len);
        test_control_return(1);
    }
    status = _req_DFU_GETSTATUS(control_transfer);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d(%ld, %ld): GETSTATUS status 0x%x\n", __LINE__, block, trans_len, status);
        test_control_return(1);
    }
    status = _req_DFU_DNLOAD(control_transfer, 0, UX_DEMO_REQUEST_MAX_LENGTH);
    UX_TEST_CHECK_CODE(UX_TRANSFER_STALLED, status);
    status = _req_DFU_CLRSTATUS(control_transfer);
    UX_TEST_CHECK_SUCCESS(status);
    status = _req_DFU_GETSTATE(control_transfer);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: transfer status 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    UX_TEST_ASSERT(dfu_host_buffer[0] == UX_SYSTEM_DFU_STATE_DFU_IDLE);

    stepinfo(">>>>>>>>>>>> All Done\n");

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}

static void  tx_test_thread_slave_simulation_entry(ULONG arg)
{
    while(1)
    {

#if defined(UX_DEVICE_STANDALONE)
        ux_system_tasks_run();
        tx_thread_relinquish();
#else

        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
#endif
    }
}

static UINT    demo_device_state_change(ULONG event)
{
    return(UX_SUCCESS);
}

static VOID    demo_thread_dfu_activate(VOID *dfu)
{
}

static VOID    demo_thread_dfu_deactivate(VOID *dfu)
{
}

static UINT    demo_thread_dfu_read(VOID *dfu, ULONG block_number, UCHAR * data_pointer, ULONG length, ULONG *actual_length)
{
ULONG       return_length;

    stepinfo("dfuRead %ld,%ld: %2x %2x %2x %2x ... -> %p\n", block_number, length,
        dfu_device_buffer[0], dfu_device_buffer[1], dfu_device_buffer[2], dfu_device_buffer[3],
        data_pointer);
    dfu_block = block_number;
    dfu_transfer_length = length;

    return_length = UX_MIN(length, sizeof(dfu_device_buffer));
    return_length = UX_MIN(return_length, dfu_actual_length);

    ux_utility_memory_copy(data_pointer, dfu_device_buffer, return_length);

    /* Here is where the data block is read from the firmware.  */
    /* Some code needs to be inserted specifically for a target platform.  */
    *actual_length =  return_length;
    return(UX_SUCCESS);
}

static UINT    demo_thread_dfu_write(VOID *dfu, ULONG block_number, UCHAR * data_pointer, ULONG length, ULONG *media_status)
{
    stepinfo("dfuWrite %ld,%ld\n", block_number, length);
    dfu_block = block_number;
    dfu_transfer_length = length;
    ux_utility_memory_copy(dfu_device_buffer, data_pointer, UX_MIN(length, sizeof(dfu_device_buffer)));

    /* Here is where the data block is coming to be written to the firmware.  */
    /* Some code needs to be inserted specifically for a target platform.  */
    /* Return media status ok.  */
    if (length > UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH)
        return(UX_ERROR);
    *media_status =  UX_SLAVE_CLASS_DFU_MEDIA_STATUS_OK    ;
    return(UX_SUCCESS);
}

static UINT    demo_thread_dfu_get_status(VOID *dfu, ULONG *media_status)
{

    /* Return media status ok.  */
    *media_status =  UX_SLAVE_CLASS_DFU_MEDIA_STATUS_OK    ;
    
    return(UX_SUCCESS);
}

static UINT    demo_thread_dfu_notify(VOID *dfu, ULONG notification)
{
    stepinfo("dfuNotify 0x%lx\n", notification);
    switch (notification)
    {


        case    UX_SLAVE_CLASS_DFU_NOTIFICATION_BEGIN_DOWNLOAD        :

            /* Begin of Download. */
            break;
            
        case    UX_SLAVE_CLASS_DFU_NOTIFICATION_END_DOWNLOAD        :

            /* Completion of Download. */
            break;
            
        case    UX_SLAVE_CLASS_DFU_NOTIFICATION_ABORT_DOWNLOAD        :

            /* Download was aborted. */
            break;

        case    UX_SLAVE_CLASS_DFU_NOTIFICATION_BEGIN_UPLOAD        :

            /* Begin of UPLOAD. */
            break;
            
        case    UX_SLAVE_CLASS_DFU_NOTIFICATION_END_UPLOAD        :

            /* Completion of UPLOAD. */
            break;
            
        case    UX_SLAVE_CLASS_DFU_NOTIFICATION_ABORT_UPLOAD        :

            /* Download was aborted. */
            break;
            
        default :

            /* Bad notification signal. Should never get here.  */
            break;

    }

    return(UX_SUCCESS);
}

static UINT demo_thread_dfu_custom_request(VOID *dfu, UX_SLAVE_TRANSFER *transfer)
{
UCHAR      *setup;
UCHAR      *buffer;

    /* Check state and request to insert custom operation, before the standard
       handling process.
       If no standard handling process is needed, return UX_SUCCESS.
    */

   /* E.g., accept DNLOAD command with wLength 0 in dfuIDLE.  */
   if (ux_device_class_dfu_state_get((UX_SLAVE_CLASS_DFU *)dfu) == UX_SLAVE_CLASS_DFU_STATUS_STATE_DFU_IDLE)
   {
       setup = transfer -> ux_slave_transfer_request_setup;
       buffer = transfer -> ux_slave_transfer_request_data_pointer;

        if (setup[UX_SETUP_REQUEST] == UX_SLAVE_CLASS_DFU_COMMAND_DOWNLOAD &&
            setup[UX_SETUP_LENGTH] == 0 &&
            setup[UX_SETUP_LENGTH + 1] == 0)
        {

            /* Accept the case (by default it's stalled).  */
            stepinfo("dfuIDLE - accept dfuDNLOAD & wLength 0\n");

            /* Fill the status data payload.  First with status.  */
            *buffer = UX_SLAVE_CLASS_DFU_STATUS_OK;

            /* Poll time out value is set to 500ms.  */
            *(buffer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT) = UX_DW0(500);
            *(buffer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 1) = UX_DW1(500);
            *(buffer + UX_SLAVE_CLASS_DFU_GET_STATUS_POLL_TIMEOUT + 2) = UX_DW2(500);

            /* Next state: still dfuIDLE.  */
            *(buffer + UX_SLAVE_CLASS_DFU_GET_STATUS_STATE) = (UCHAR) UX_SYSTEM_DFU_STATE_DFU_IDLE;

            /* String index set to 0.  */
            *(buffer + UX_SLAVE_CLASS_DFU_GET_STATUS_STRING) = 0;

            /* We have a request to obtain the status of the DFU instance. */
            _ux_device_stack_transfer_request(transfer, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH, UX_SLAVE_CLASS_DFU_GET_STATUS_LENGTH);

            /* Inform stack it's taken.  */
            return(UX_SUCCESS);
        }
    }

    /* No custom request.  */
    return(UX_ERROR);
}
