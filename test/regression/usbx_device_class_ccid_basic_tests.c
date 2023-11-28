/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_device_class_ccid.h"
#include "ux_device_stack.h"

#include "ux_host_class_dummy.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"

#include "ux_test_utility_sim.h"

/* Define constants.  */
#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024
#define                             UX_DEMO_BUFFER_SIZE (UX_SLAVE_REQUEST_DATA_MAX_LENGTH + 1)
#define                             UX_DEMO_XMIT_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE 512
#define                             UX_DEMO_FILE_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE 64
#define                             UX_DEMO_MEMORY_SIZE     (64*1024)
#define                             UX_DEMO_FILE_SIZE       (128 * 1024)
#define                             UX_RAM_DISK_MEMORY      (256 * 1024)

#define                             UX_DEMO_MAX_SLOT_INDEX     (2)
#define                             UX_DEMO_MAX_BUSY_SLOTS     (2)
#if UX_SLAVE_REQUEST_DATA_MAX_LENGTH >= 1024
#define                             UX_DEMO_MAX_MESSAGE_LENGTH (1024)
#else
#define                             UX_DEMO_MAX_MESSAGE_LENGTH (UX_SLAVE_REQUEST_DATA_MAX_LENGTH)
#endif
#define                             UX_DEMO_N_CLOCKS           (1)
#define                             UX_DEMO_N_DATA_RATES       (1)
#define                             UX_DEMO_BULK_OUT_EP         0x02
#define                             UX_DEMO_BULK_IN_EP          0x81
#define                             UX_DEMO_INTERRUPT_IN_EP     0x83
#define                             UX_DEMO_INTERRUPT_IN_SIZE   (8)

/* Define local/extern function prototypes.  */
static VOID                                test_thread_entry(ULONG);
static TX_THREAD                           tx_test_thread_host_simulation;
static TX_THREAD                           tx_test_thread_slave_simulation;
static VOID                                tx_test_thread_host_simulation_entry(ULONG);
static VOID                                tx_test_thread_slave_simulation_entry(ULONG);

static VOID                                ux_test_hcd_entry_should_not_be_called(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_disconnect(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);

static UINT                                ux_test_ccid_icc_power_on(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_icc_power_off(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_get_slot_status(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_xfr_block(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_get_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_reset_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_set_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_escape(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_icc_clock(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_t0_apdu(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_secure(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_mechanical(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_abort(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);
static UINT                                ux_test_ccid_set_data_rate_and_clock_frequency(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg);

/* Define global data structures.  */
static UCHAR                        usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

static UX_DEVICE                    *host_device = UX_NULL;
static UCHAR                        host_bulk_out[UX_DEMO_MAX_MESSAGE_LENGTH];
static UCHAR                        host_bulk_in[UX_DEMO_MAX_MESSAGE_LENGTH];
static ULONG                        host_bulk_in_length;
static UCHAR                        host_interrupt_in[UX_DEMO_INTERRUPT_IN_SIZE];
static ULONG                        host_interrupt_in_length;

static UX_HOST_CLASS_DUMMY          *host_ccid = UX_NULL;

static ULONG                        enum_counter;

static ULONG                        error_counter;
static ULONG                        error_callback_counter;

static ULONG                        set_cfg_counter;

static ULONG                        rsc_mem_alloc_cnt_on_set_cfg;
static ULONG                        rsc_mem_free_on_set_cfg;
static ULONG                        rsc_sem_on_set_cfg;
static ULONG                        rsc_sem_get_on_set_cfg;
static ULONG                        rsc_mutex_on_set_cfg;

static ULONG                        rsc_enum_sem_usage;
static ULONG                        rsc_enum_sem_get_count;
static ULONG                        rsc_enum_mutex_usage;
static ULONG                        rsc_enum_mem_usage;
static ULONG                        rsc_enum_mem_alloc_count;

static ULONG                        rsc_test_sem_usage;
static ULONG                        rsc_test_sem_get_count;
static ULONG                        rsc_test_mutex_usage;
static ULONG                        rsc_test_mem_alloc_count;

static UX_DEVICE_CLASS_CCID_HANDLES device_ccid_handles =
{
    ux_test_ccid_icc_power_on,
    ux_test_ccid_icc_power_off,
    ux_test_ccid_get_slot_status,
    ux_test_ccid_xfr_block,
    ux_test_ccid_get_parameters,
    ux_test_ccid_reset_parameters,
    ux_test_ccid_set_parameters,
    ux_test_ccid_escape,
    ux_test_ccid_icc_clock,
    ux_test_ccid_t0_apdu,
    ux_test_ccid_secure,
    ux_test_ccid_mechanical,
    ux_test_ccid_abort,
    ux_test_ccid_set_data_rate_and_clock_frequency,
};
static ULONG                                device_ccid_clocks[] =
{
    1200,
};
static ULONG                                device_ccid_data_rates[] =
{
    115200,
};
static UX_DEVICE_CLASS_CCID                 *device_ccid = UX_NULL;
static UX_DEVICE_CLASS_CCID_PARAMETER       device_ccid_parameter;
static UCHAR                                device_ccid_soft_reset_count = 0;

static struct _TEST_CCID_CALLBACK_LOG {
    UX_DEVICE_CLASS_CCID_HANDLE             handle;
    ULONG                                   buf_length;
    UCHAR                                   buf[UX_DEMO_MAX_MESSAGE_LENGTH + 4];
}                                           device_ccid_callback_log;
static inline void ux_test_callback_log(UX_DEVICE_CLASS_CCID_HANDLE handle, UCHAR *ccid_msg)
{
ULONG buf_length;
    device_ccid_callback_log.handle = handle;
    if (ccid_msg == UX_NULL)
    {
        device_ccid_callback_log.buf[0] = 0;
        device_ccid_callback_log.buf_length = 0;
        return;
    }
    buf_length = UX_DEVICE_CLASS_CCID_MESSAGE_LENGTH_GET(ccid_msg) + UX_DEVICE_CLASS_CCID_MESSAGE_HEADER_LENGTH;
    UX_TEST_ASSERT(buf_length <= UX_DEMO_MAX_MESSAGE_LENGTH);
    _ux_utility_memory_copy(device_ccid_callback_log.buf, ccid_msg, buf_length);
    device_ccid_callback_log.buf_length = buf_length;
}

/* Define device framework.  */

#define _W(w)   UX_W0(w),UX_W1(w)
#define _DW(dw) UX_DW0(dw),UX_DW1(dw),UX_DW2(dw),UX_DW3(dw)

#define _CONFIGURATION_DESCRIPTOR(total_len, n_ifc, cfg_val)                    \
    0x09, 0x02, UX_W0(total_len), UX_W1(total_len), (n_ifc), (cfg_val),         \
    0x00, 0xc0, 0x32,

#define _INTERFACE_DESCRIPTOR(ifc_n, alt, n_ep, cls, sub, protocol)             \
    0x09, 0x04, (ifc_n), (alt), (n_ep), (cls), (sub), (protocol), 0x00,

#define _SMART_CARD_DESCRIPTORS                                                 \
    0x36, /* bLength.  */                                                       \
    0x21, /* bDescriptorType.  */                                               \
    0x10, 0x01, /* bcdCCID.  */                                                 \
    UX_DEMO_MAX_SLOT_INDEX, /* bMaxSlotIndex.  */                               \
    0x01, /* bVoltageSupport (1-5V,2-3.0V,4-1.8V). */                           \
    0x00, 0x00, 0x00, 0x00, /* dwProtocols PPPP, RRRR.  */                      \
    _DW(14320), /* dwDefaultClock (KHz).  */                                    \
    _DW(14320), /* dwMaximumClock (KHz).  */                                    \
    UX_DEMO_N_CLOCKS, /* bNumClockSupported.  */                                \
    _DW(115200), /* dwDataRate (bps).  */                                       \
    _DW(115200), /* dwMaxDataRate (bps).  */                                    \
    UX_DEMO_N_DATA_RATES, /* bNumDataRatesSupported.  */                        \
    0x00, 0x00, 0x00, 0x00, /* dwMaxIFSD.  */                                   \
    0x00, 0x00, 0x00, 0x00, /* dwSynchProtocols PPPP, RRRR.  */                 \
    0x00, 0x00, 0x00, 0x00, /* dwMechanical (1-accept,2-eject,4-capture,8-lock/unlock).  */\
    0x00, 0x00, 0x00, 0x00, /* dwFeatures.  */                                  \
    _DW(1024), /* dwMaxCCIDMessageLength.  */                                   \
    0, /* bClassGetResponse.  */                                                \
    0, /* bClassEnvelope.  */                                                   \
    _W(0x0000), /* wLcdLayout, XXYY.  */                                        \
    0x3, /* bPINSupport, 1-Verification, 2-Modification.  */                    \
    UX_DEMO_MAX_BUSY_SLOTS, /* bMaxCCIDBusySlots.  */


#define _ENDPOINT_DESCRIPTOR(addr, attr, pktsize, interval)                     \
    0x07, 0x05, (addr), (attr), UX_W0(pktsize), UX_W1(pktsize), (interval),

#define _CFG_TOTAL_LEN (9+9+0x36+7+7+7)

#define             STRING_FRAMEWORK_LENGTH                 47
#define             LANGUAGE_ID_FRAMEWORK_LENGTH            2

static unsigned char device_framework_full_speed[] = {

    /* Device descriptor     18 bytes
       0xEF bDeviceClass:    Composite class code
       0x02 bDeviceSubclass: class sub code
       0x00 bDeviceProtocol: Device protocol
       idVendor & idProduct - http://www.linux-usb.org/usb.ids
    */
    0x12, 0x01, 0x10, 0x01,
    0x00, 0x00, 0x00,
    0x08,
    0x84, 0x84, 0x00, 0x00,
    0x00, 0x01,
    0x01, 0x02, 0x03,
    0x01,

    _CONFIGURATION_DESCRIPTOR(_CFG_TOTAL_LEN, 1, 1)
    _INTERFACE_DESCRIPTOR(0, 0, 3, 0x0B, 0x00, 0x00)
    _SMART_CARD_DESCRIPTORS
    _ENDPOINT_DESCRIPTOR(UX_DEMO_BULK_OUT_EP,       0x02, 64, 0x00)
    _ENDPOINT_DESCRIPTOR(UX_DEMO_BULK_IN_EP,        0x02, 64, 0x00)
    _ENDPOINT_DESCRIPTOR(UX_DEMO_INTERRUPT_IN_EP,   0x03, UX_DEMO_INTERRUPT_IN_SIZE, 0x04)
};

#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      sizeof(device_framework_full_speed)
#define             DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED      sizeof(device_framework_full_speed)
#define             device_framework_high_speed             device_framework_full_speed

static unsigned char string_framework[] = {

    /* Manufacturer string descriptor : Index 1 - "AzureRTOS" */
    0x09, 0x04, 0x01, 9,
        'A','z','u','r','e','R','T','O','S',

    /* Product string descriptor : Index 2 - "ccid device" */
    0x09, 0x04, 0x02, 11,
        'C','C','I','D',' ','d','e','v','i','c','e',

    /* Serial Number string descriptor : Index 3 - "0001" */
    0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31
};


    /* Multiple languages are supported on the device, to add
       a language besides english, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
static unsigned char language_id_framework[] = {

    /* English. */
        0x09, 0x04
};

static UX_TEST_SETUP _SetConfigure = UX_TEST_SETUP_SetConfigure;

/* Test interactions */

static UX_TEST_HCD_SIM_ACTION log_on_SetCfg[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_SetConfigure,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_SUCCESS, ux_test_hcd_entry_set_cfg,
        UX_TRUE}, /* Invoke callback & continue */
{   0   }
};

/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}

static UINT test_slave_change_function(ULONG change)
{
    return 0;
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{
    // printf("hCbChange: %lx %p %p\n", event, (VOID*)cls, inst);
    switch(event)
    {

    case UX_DEVICE_INSERTION:
        host_ccid = inst;
        break;

    case UX_DEVICE_REMOVAL:
        if (host_ccid == inst)
            host_ccid = UX_NULL;
        break;

    case UX_DEVICE_CONNECTION:
        host_device = (UX_DEVICE *)inst;
        break;

    case UX_DEVICE_DISCONNECTION:
        if ((VOID *)host_device == inst)
            host_device = UX_NULL;
        break;

#if defined(UX_HOST_STANDALONE)
    case UX_STANDALONE_WAIT_BACKGROUND_TASK:
        tx_thread_relinquish();
        break;
#endif

    default:
        break;
    }
    return 0;
}

static VOID    test_ccid_instance_activate(VOID *dummy_instance)
{
    if (device_ccid == UX_NULL)
        device_ccid = (UX_DEVICE_CLASS_CCID *)dummy_instance;
}
static VOID    test_ccid_instance_deactivate(VOID *dummy_instance)
{
    if ((VOID*)device_ccid == dummy_instance)
        device_ccid = UX_NULL;
}
static VOID    test_ccid_soft_reset(VOID *dummy_instance)
{
    device_ccid_soft_reset_count ++;
}

static VOID test_ux_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    error_callback_counter ++;
}

static VOID ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params)
{

    set_cfg_counter ++;

    rsc_mem_free_on_set_cfg = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    rsc_mem_alloc_cnt_on_set_cfg = ux_test_utility_sim_mem_alloc_count();

    rsc_sem_on_set_cfg = ux_test_utility_sim_sem_create_count();
    rsc_enum_sem_get_count = ux_test_utility_sim_sem_get_count();
    rsc_mutex_on_set_cfg = ux_test_utility_sim_mutex_create_count();
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_host_class_ccid_basic_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;


    printf("Running CCID Basic Functionality Test............................... ");
#if !UX_TEST_MULTI_EP_OVER(2)
    printf("Skip\n");
    test_control_return(0);
    return;
#endif

    /* Reset testing counts. */
    ux_test_utility_sim_mem_alloc_log_enable(UX_TRUE);
    ux_test_utility_sim_mem_alloc_count_reset();
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    /* Reset error generations */
    ux_test_utility_sim_sem_error_generation_stop();
    ux_test_utility_sim_mutex_error_generation_stop();
    ux_test_utility_sim_sem_get_error_generation_stop();

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 4);

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(test_ux_error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(test_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register Host DUMMY class.  */
    status =  ux_host_stack_class_register(_ux_host_class_dummy_name, _ux_host_class_dummy_entry);
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #3\n");
        test_control_return(1);
    }

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,
                                       test_slave_change_function);
    if(status!=UX_SUCCESS)
    {

        printf(" ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Set the parameters for callback when insertion/extraction of a ccid device.  */
    _ux_utility_memory_set(&device_ccid_parameter, 0, sizeof(device_ccid_parameter));
    device_ccid_parameter.ux_device_class_ccid_handles             = &device_ccid_handles;
    device_ccid_parameter.ux_device_class_ccid_instance_activate   = test_ccid_instance_activate;
    device_ccid_parameter.ux_device_class_ccid_instance_deactivate = test_ccid_instance_deactivate;
    device_ccid_parameter.ux_device_class_ccid_max_n_slots         = UX_DEMO_MAX_SLOT_INDEX + 1;
    device_ccid_parameter.ux_device_class_ccid_max_n_busy_slots    = UX_DEMO_MAX_BUSY_SLOTS;
    device_ccid_parameter.ux_device_class_ccid_max_transfer_length = UX_DEMO_MAX_MESSAGE_LENGTH;
    device_ccid_parameter.ux_device_class_ccid_n_clocks            = UX_DEMO_N_CLOCKS;
    device_ccid_parameter.ux_device_class_ccid_n_data_rates        = UX_DEMO_N_DATA_RATES;
    device_ccid_parameter.ux_device_class_ccid_clocks              = device_ccid_clocks;
    device_ccid_parameter.ux_device_class_ccid_data_rates          = device_ccid_data_rates;
    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status  = ux_device_stack_class_register(_ux_system_device_class_ccid_name,
                                             ux_device_class_ccid_entry,
                                             1, 0, &device_ccid_parameter);
    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf(" ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_test_thread_host_simulation, "tx test host simulation", tx_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf(" ERROR #9\n");
        test_control_return(1);
    }

    /* Create the main slave simulation  thread.  */
    stack_pointer += UX_DEMO_STACK_SIZE;
    status =  tx_thread_create(&tx_test_thread_slave_simulation, "tx test slave simulation", tx_test_thread_slave_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf(" ERROR #10\n");
        test_control_return(1);
    }
}

static UINT _test_check_host_connection_error(VOID)
{
    if (device_ccid && host_ccid)
        return(UX_SUCCESS);
    if (error_callback_counter >= 3)
        return(UX_SUCCESS);
    return(UX_ERROR);
}

static UINT _test_check_host_connection_success(VOID)
{
    if (device_ccid && host_ccid)
        return(UX_SUCCESS);
    return(UX_ERROR);
}

static UINT _test_check_host_disconnection_success(VOID)
{
    if (device_ccid == UX_NULL && host_ccid == UX_NULL)
        return(UX_SUCCESS);
    return(UX_ERROR);
}

static VOID _ccid_enumeration_test(VOID)
{
UINT                                        status;
ULONG                                       mem_free;
ULONG                                       test_n;

    stepinfo(">>>>>>>>>>>> Enumeration information collection\n");
    {

        /* Test disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Check connection. */
        status = ux_test_sleep_break_on_success(200, _test_check_host_disconnection_success);
        UX_TEST_ASSERT(status == UX_SUCCESS);

        /* Reset testing counts. */
        ux_test_utility_sim_mem_alloc_count_reset();
        ux_test_utility_sim_mutex_create_count_reset();
        ux_test_utility_sim_sem_create_count_reset();
        ux_test_utility_sim_sem_get_count_reset();
        ux_test_hcd_sim_host_set_actions(log_on_SetCfg);

        /* Save free memory usage. */
        mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);

        /* Check connection. */
        status = ux_test_sleep_break_on_success(100, _test_check_host_connection_success);
        UX_TEST_ASSERT(status == UX_SUCCESS);

        /* Log create counts for further tests. */
        rsc_enum_mutex_usage = rsc_mutex_on_set_cfg;
        rsc_enum_sem_usage = rsc_sem_on_set_cfg;
        rsc_enum_mem_alloc_count = rsc_mem_alloc_cnt_on_set_cfg;
        /* Log create counts when instances active for further tests. */
        rsc_test_mutex_usage = ux_test_utility_sim_mutex_create_count() - rsc_enum_mutex_usage;
        rsc_test_sem_usage = ux_test_utility_sim_sem_create_count() - rsc_enum_sem_usage;
        rsc_test_mem_alloc_count = ux_test_utility_sim_mem_alloc_count() - rsc_enum_mem_alloc_count;

        /* Lock log base for tests. */
        ux_test_utility_sim_mem_alloc_log_lock();

        stepinfo("enum mem: %ld\n", rsc_enum_mem_alloc_count);
        stepinfo("test mem : %ld\n", rsc_test_mem_alloc_count);
        stepinfo("mem free: %ld, %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available);
    }

    stepinfo(">>>>>>>>>>>> Enumeration test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < 3; test_n++)
    {
        stepinfo("%4ld / 2\n", test_n);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Check */
        status = ux_test_sleep_break_on_success(100, _test_check_host_disconnection_success);
        if (status != UX_SUCCESS)
        {

            printf("ERROR #%d.%ld: Disconnect fail\n", __LINE__, test_n);
            test_control_return(1);
        }

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #%d.%ld: Memory level different after re-enumerations %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            test_control_return(1);
        }

        /* Connect. */
        error_callback_counter = 0;
        ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);

        /* Wait and break on error. */
        error_callback_counter = 0;
        status = ux_test_sleep_break_on_success(100, _test_check_host_connection_error);

        /* Check */
        if (status != UX_SUCCESS)
        {

            printf("ERROR #%d.%ld: Enumeration fail\n", __LINE__, test_n);
            test_control_return(1);
        }
    }
    stepinfo("\n");

    if (rsc_test_mem_alloc_count) stepinfo(">>>>>>>>>>>> Memory errors enumeration test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < rsc_test_mem_alloc_count; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_test_mem_alloc_count - 1);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Check */
        status = ux_test_sleep_break_on_success(100, _test_check_host_disconnection_success);
        if (status != UX_SUCCESS)
        {

            stepinfo("ERROR #%d.%ld: Disconnect fail\n", __LINE__, test_n);
            test_control_return(1);
        }

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            stepinfo("ERROR #%d.%ld: Memory level different after re-enumerations %ld <> %ld\n", __LINE__, test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
            test_control_return(1);
        }

        /* Set memory error generation */
        ux_test_utility_sim_mem_alloc_error_generation_start(test_n + rsc_enum_mem_alloc_count);

        /* Connect. */
        error_callback_counter = 0;
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        /* Wait and break on errors. */
        status = ux_test_sleep_break_on_success(100, _test_check_host_connection_error);

        /* Check error */
        if (status != UX_SUCCESS)
        {

            /* Check error trap.  */
            if (error_callback_counter == 0)
            {
                stepinfo("ERROR #%d.%ld: device detected when there is memory error\n", __LINE__, test_n);
                test_control_return(1);
            }
        }
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_mem_alloc_error_generation_stop();
    if (rsc_test_mem_alloc_count) stepinfo("\n");

    /* If device disconnected, re-connect.  */
    if (_test_check_host_connection_success() != UX_SUCCESS)
    {
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();
        ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);

        /* Check */
        status = ux_test_sleep_break_on_success(100, _test_check_host_connection_success);
        if (status != UX_SUCCESS)
        {

            printf("ERROR #%d: Enumeration fail\n", __LINE__);
            test_control_return(1);
        }
    }
}

static UINT _ccid_message_bulk_out_in(UCHAR *out, ULONG out_length, UCHAR *in, ULONG *in_length)
{
ULONG           payload_size;
ULONG           total_length, actual_length;
UINT            status = UX_FUNCTION_NOT_SUPPORTED;
    if (out)
    {
        total_length = out_length;
        status = _ux_host_class_dummy_transfer(host_ccid, UX_DEMO_BULK_OUT_EP, 0, out, total_length, &actual_length);
        if (status != UX_SUCCESS)
        {
            printf("BulkOUT fail: 0x%x\n", status);
            return(status);
        }
    }
    if (in)
    {
        payload_size = _ux_host_class_dummy_get_max_payload_size(host_ccid, UX_DEMO_BULK_IN_EP, 0);
        while(1)
        {
            status = _ux_host_class_dummy_transfer(host_ccid, UX_DEMO_BULK_IN_EP, 0, in, payload_size, &actual_length);
            *in_length = actual_length;
            if (status != UX_SUCCESS)
            {
                printf("BulkIN fail: 0x%x\n", status);
                return(status);
            }
            /* Check time extension.  */
            if (((in[7] >> 6) & 0x3u) == 0x2)
            {
                /* Try to read another packet.  */
                // printf("TimeExtension\n");
                continue;
            }
            /* Check short packet.  */
            if (actual_length < payload_size)
                return(status);
            else
                break;
        }
        /* Read remaining message.  */
        total_length = UX_DEVICE_CLASS_CCID_MESSAGE_LENGTH_GET(in);
        total_length += UX_DEVICE_CLASS_CCID_MESSAGE_HEADER_LENGTH;
        //printf("FirstPacket, total %ld\n", total_length);
        in += payload_size;
        total_length -= payload_size;
        if (total_length)
        {
            status = _ux_host_class_dummy_transfer(host_ccid, UX_DEMO_BULK_IN_EP, 0, in, total_length, &actual_length);
            *in_length += actual_length;
        }
    }
    else
    {
        /* Let other threads run.  */
        tx_thread_sleep(1);
    }
    return(status);
}

static UINT _ccid_message_interrupt_in(UCHAR *in, ULONG *in_length)
{
ULONG           payload_size = _ux_host_class_dummy_get_max_payload_size(host_ccid, UX_DEMO_INTERRUPT_IN_EP, 0);
UINT            status;
    status = _ux_host_class_dummy_transfer(host_ccid, UX_DEMO_INTERRUPT_IN_EP, 0, in, payload_size, in_length);
    return(status);
}


#define _TEST_CCID_BULK_OUT_CHECK(h, l) \
    UX_TEST_ASSERT(status == UX_SUCCESS); \
    UX_TEST_ASSERT(device_ccid_callback_log.handle == (h)); \
    if ((h) != UX_NULL) { \
        UX_TEST_ASSERT(device_ccid_callback_log.buf_length == (l)); \
        UX_TEST_ASSERT(_ux_utility_memory_compare(device_ccid_callback_log.buf, host_bulk_out, (l)) == UX_SUCCESS); \
    }

#define _TEST_CCID_BULK_IN_HEADER_CHECK(type,len,slot,seq,stat,err) \
    UX_TEST_ASSERT(host_bulk_in[0] == (type));\
    UX_TEST_ASSERT((len) == _ux_utility_long_get(host_bulk_in + 1));\
    UX_TEST_ASSERT(host_bulk_in[5] == (slot));\
    UX_TEST_ASSERT(host_bulk_in[6] == (seq));\
    UX_TEST_ASSERT(host_bulk_in[7] == (stat)); /* bStatus: not present.  */\
    UX_TEST_ASSERT(host_bulk_in[8] == (err));

static VOID _buffer_dump(VOID *buf, ULONG len)
{
ULONG       i;
    for(i = 0; i < len; i ++)
        printf(" %02x", ((UCHAR *)buf)[i]);
    printf("\n");
}

static VOID _ccid_icc_insert_test(VOID)
{
UINT        status;

    stepinfo(">>>>>>>>>> Test Insert\n");
    ux_device_class_ccid_icc_insert(device_ccid, 0, 0);
    ux_device_class_ccid_icc_insert(device_ccid, 1, 0);
    status = _ccid_message_interrupt_in(host_interrupt_in, &host_interrupt_in_length);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(host_interrupt_in_length == 2);
    UX_TEST_ASSERT(host_interrupt_in[0] == 0x50);

#if UX_DEVICE_CLASS_CCID_MAX_N_SLOTS > 1
    if (host_interrupt_in[1] == 0x03)
    {
        status = _ccid_message_interrupt_in(host_interrupt_in, &host_interrupt_in_length);
        UX_TEST_ASSERT(host_interrupt_in_length == 2);
        UX_TEST_ASSERT(host_interrupt_in[0] == 0x50);
        UX_TEST_ASSERT(host_interrupt_in[1] == 0x0D); /* Slot0: exist, Slot1: insert.  */
    }
    else
        UX_TEST_ASSERT(host_interrupt_in[1] == 0x0F);
#else
    UX_TEST_ASSERT(host_interrupt_in[1] == 0x03); /* Slot0: exist.  */
#endif

#if UX_DEVICE_CLASS_CCID_MAX_N_SLOTS > 1
    #define _RM_SLOT   1
    #define _RM_CHECK  0x09 /* Slot0: exist, Slot1: remove.  */
#else
    #define _RM_SLOT   0
    #define _RM_CHECK  0x02 /* Slot0: remove, Slot1: remove.  */
#endif
    stepinfo(">>>>>>>>>> Test Remove\n");
    ux_device_class_ccid_icc_remove(device_ccid, _RM_SLOT);
    status = _ccid_message_interrupt_in(host_interrupt_in, &host_interrupt_in_length);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(host_interrupt_in_length == 2);
    UX_TEST_ASSERT(host_interrupt_in[0] == 0x50);
    UX_TEST_ASSERT(host_interrupt_in[1] == _RM_CHECK);

#if UX_DEVICE_CLASS_CCID_MAX_N_SLOTS > 1
    #define _INS_SLOT   1
    #define _INS_CHECK  0x0D /* Slot0: exist, Slot1: insert.  */
#else
    #define _INS_SLOT   0
    #define _INS_CHECK  0x03 /* Slot0: insert, Slot1: remove.  */
#endif
    stepinfo(">>>>>>>>>> Test Insert (auto)\n");
    ux_device_class_ccid_icc_insert(device_ccid, _INS_SLOT, 1);
    status = _ccid_message_interrupt_in(host_interrupt_in, &host_interrupt_in_length);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(host_interrupt_in_length == 2);
    UX_TEST_ASSERT(host_interrupt_in[0] == 0x50);
    UX_TEST_ASSERT(host_interrupt_in[1] == _INS_CHECK); /* Slot0: exist, Slot1: insert.  */
}

static VOID _ccid_icc_power_on_off_test(VOID)
{
UINT        status;
    /* PC_to_RDR_IccPowerOn, RDR_to_PC_DataBlock   */

    /* Message(IccPowerOn).  */
    stepinfo(">>>>>>>>>> Test IccPowerOn\n");
    _ux_utility_memory_set(host_bulk_out, 0, 10);
    host_bulk_out[0] = 0x62;
    host_bulk_out[5] = 0; /* bSlot.  */
    host_bulk_out[6] = 0; /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 0);
    host_bulk_out[7] = 2;
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_icc_power_on, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x80, 0, 0, 0, 0, 0)

#if UX_DEVICE_CLASS_CCID_MAX_N_SLOTS > 1

    /* Message(IccPowerOff).  */
    stepinfo(">>>>>>>>>> Test IccPowerOff\n");
    host_bulk_out[0] = 0x63;
    host_bulk_out[5] = 0; /* bSlot.  */
    host_bulk_out[6] = 1; /* bSeq.   */
    host_bulk_out[7] = 0;
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_icc_power_off, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x81, 0, 0, 1, 1, 0)

    /* Message(IccPowerOff) -> BUSY_WITH_AUTO_SEQUENCE.  */
    stepinfo(">>>>>>>>>> Test IccPowerOff -> BUSY_WITH_AUTO_SEQUENCE\n");
    host_bulk_out[0] = 0x63;
    host_bulk_out[5] = 1; /* bSlot.  */
    host_bulk_out[6] = 0; /* bSeq.   */
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(UX_NULL, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x81, 0, 1, 0, 0x41, 0xF2)

    /* Message(IccPowerOff).  */
    stepinfo(">>>>>>>>>> Test seq done - IccPowerOff\n");
    ux_device_class_ccid_auto_seq_done(device_ccid, 1, UX_DEVICE_CLASS_CCID_ICC_ACTIVE);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_icc_power_off, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x81, 0, 1, 0, 1, 0)
#else

    /* Message(IccPowerOff) -> BUSY_WITH_AUTO_SEQUENCE.  */
    stepinfo(">>>>>>>>>> Test IccPowerOff -> BUSY_WITH_AUTO_SEQUENCE\n");
    host_bulk_out[0] = 0x63;
    host_bulk_out[5] = 0; /* bSlot.  */
    host_bulk_out[6] = 1; /* bSeq.   */
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(UX_NULL, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x81, 0, 0, 1, 0x41, 0xF2)

    /* Message(IccPowerOff).  */
    stepinfo(">>>>>>>>>> Test seq done - IccPowerOff\n");
    ux_device_class_ccid_auto_seq_done(device_ccid, 0, UX_DEVICE_CLASS_CCID_ICC_ACTIVE);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_icc_power_off, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x81, 0, 0, 1, 1, 0)
#endif

    /* Message(IccPowerOn).  */
    host_bulk_out[0] = 0x62;
    host_bulk_out[5] = 0; /* bSlot.  */
    host_bulk_out[6] = 3; /* bSeq.   */
    host_bulk_out[7] = 1;
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_icc_power_on, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x80, 0, 0, 3, 0, 0)
}

static VOID _ccid_get_slot_status_test(VOID)
{
UINT        status;
    /* PC_to_RDR_GetSlotStatus, RDR_to_PC_SlotStatus   */
    _ux_utility_memory_set(host_bulk_out, 0, 10);
    host_bulk_out[0] = 0x65;
    host_bulk_out[5] = 0; /* bSlot.  */
    host_bulk_out[6] = 0; /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 0);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_get_slot_status, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x81, 0, 0, 0, 0, 0)
}

static VOID _ccid_xfr_block_test(VOID)
{
UINT        status;
    /* PC_to_RDR_XfrBlock, RDR_to_PC_DataBlock.  */
    _ux_utility_memory_set(host_bulk_out, 0, 10);
    host_bulk_out[0] = 0x6F;
    host_bulk_out[5] = 0; /* bSlot.  */
    host_bulk_out[6] = 8; /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 0);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_xfr_block, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x80, 64-10, 0, 8, 0, 0)
}

static VOID _ccid_parameters_test(VOID)
{
UINT        status;
    /* PC_to_RDR_[Get/Reset/Set]Parameters, RDR_to_PC_Parameters.  */
    _ux_utility_memory_set(host_bulk_out, 0, 10);

    /* GetParameters.  */
    host_bulk_out[0] = 0x6C;
    host_bulk_out[5] = 0;  /* bSlot.  */
    host_bulk_out[6] = 9;  /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 0);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_get_parameters, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x82, 5, 0, 9, 0, 0)

    /* ResetParameters.  */
    host_bulk_out[0] = 0x6D;
    host_bulk_out[5] = 0;  /* bSlot.  */
    host_bulk_out[6] = 10; /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 0);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_reset_parameters, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x82, 5, 0, 10, 0, 0)

    /* SetParameters.  */
    host_bulk_out[0] = 0x61;
    host_bulk_out[5] = 0;  /* bSlot.  */
    host_bulk_out[6] = 11; /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 0);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_set_parameters, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x82, 5, 0, 11, 0, 0)

}

static VOID _ccid_escape_test(VOID)
{
UINT        status;
    /* PC_to_RDR_Escape, RDR_to_PC_Escape.  */
    _ux_utility_memory_set(host_bulk_out, 0, 10);

    /* GetParameters.  */
    host_bulk_out[0] = 0x6B;
    host_bulk_out[5] = 0;   /* bSlot.  */
    host_bulk_out[6] = 11;  /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 4);
    host_bulk_out[10] = 1;
    host_bulk_out[10] = 2;
    host_bulk_out[10] = 3;
    host_bulk_out[10] = 4;
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10+4, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_escape, 14)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x83, 128, 0, 11, 0, 0)
}

static VOID _ccid_icc_clock_test(VOID)
{
UINT        status;
    /* PC_to_RDR_IccClock, RDR_to_PC_SlotStatus.  */
    _ux_utility_memory_set(host_bulk_out, 0, 10);

    /* GetParameters.  */
    host_bulk_out[0] = 0x6E;
    host_bulk_out[5] = 0;   /* bSlot.  */
    host_bulk_out[6] = 88;  /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 0);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_icc_clock, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x81, 0, 0, 88, 0, 0)
}

static VOID _ccid_t0_apdu_test(VOID)
{
UINT        status;
    /* PC_to_RDR_T0APDU, RDR_to_PC_SlotStatus.  */
    _ux_utility_memory_set(host_bulk_out, 0, 10);

    /* GetParameters.  */
    host_bulk_out[0] = 0x6A;
    host_bulk_out[5] = 0;  /* bSlot.  */
    host_bulk_out[6] = 1;  /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 0);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_t0_apdu, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x81, 0, 0, 1, 0, 0)
}

static VOID _ccid_secure_test(VOID)
{
UINT        status;
    /* PC_to_RDR_Secure, RDR_to_PC_DataBlock.  */
    _ux_utility_memory_set(host_bulk_out, 0, 10);

    /* GetParameters.  */
    host_bulk_out[0] = 0x69;
    host_bulk_out[5] = 0;  /* bSlot.  */
    host_bulk_out[6] = 5;  /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 63-10);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 63, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_secure, 63)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x80, 128/8, 0, 5, 0, 0)
}

static VOID _ccid_mechanical_test(VOID)
{
UINT        status;
    /* PC_to_RDR_Mechanical, RDR_to_PC_SlotStatus.  */
    _ux_utility_memory_set(host_bulk_out, 0, 10);

    /* GetParameters.  */
    host_bulk_out[0] = 0x71;
    host_bulk_out[5] = 0;  /* bSlot.  */
    host_bulk_out[6] = 50;  /* bSeq.   */
    host_bulk_out[7] = 0x4; /* bFunction.   */
    _ux_utility_long_put(host_bulk_out+1, 0);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_mechanical, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x81, 0, 0, 50, 0, 0)
}

static VOID _ccid_abort_test(VOID)
{
UINT        status;
    /* PC_to_RDR_Abort, RDR_to_PC_SlotStatus.  */
    _ux_utility_memory_set(host_bulk_out, 0, 10);

    /* Abort.  */
    host_bulk_out[0] = 0x72;
    host_bulk_out[5] = 0;   /* bSlot.  */
    host_bulk_out[6] = 20;  /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 0);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 10, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_abort, 10)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x81, 0, 0, 20, 0, 0)
}

static VOID _ccid_data_rate_and_clock_test(VOID)
{
UINT        status;
    /* PC_to_RDR_SetDatarateAndClockFrequency, RDR_to_PC_DataRateAndClockFrequency.  */
    _ux_utility_memory_set(host_bulk_out, 0, 10);

    /* GetParameters.  */
    host_bulk_out[0] = 0x73;
    host_bulk_out[5] = 0;   /* bSlot.  */
    host_bulk_out[6] = 40;  /* bSeq.   */
    _ux_utility_long_put(host_bulk_out+1, 8);
    device_ccid_callback_log.handle = UX_NULL;
    status = _ccid_message_bulk_out_in(host_bulk_out, 18, host_bulk_in, &host_bulk_in_length);
    _TEST_CCID_BULK_OUT_CHECK(ux_test_ccid_set_data_rate_and_clock_frequency, 18)
    _TEST_CCID_BULK_IN_HEADER_CHECK(0x84, 8, 0, 40, 0, 0)
}

static VOID _ccid_control_requests_test(VOID)
{
UX_ENDPOINT     *endpoint = _ux_host_class_dummy_get_endpoint(host_ccid, 0, 0);
UX_TRANSFER     *transfer = &endpoint->ux_endpoint_transfer_request;
UX_INTERFACE    *interface = host_ccid->ux_host_class_dummy_interface;
ULONG           buffer[8];
UINT            status;
    /* Issue GET_CLOCK_FREQUENCIES request.  */
    transfer->ux_transfer_request_type              = 0xA1;
    transfer->ux_transfer_request_function          = 0x02;
    transfer->ux_transfer_request_index             = interface->ux_interface_descriptor.bInterfaceNumber;
    transfer->ux_transfer_request_value             = 0;
    transfer->ux_transfer_request_requested_length  = sizeof(buffer);
    transfer->ux_transfer_request_data_pointer      = (UCHAR*)buffer;
    status = ux_host_stack_transfer_request(transfer);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(transfer->ux_transfer_request_actual_length == 4);
    UX_TEST_ASSERT(buffer[0] == device_ccid_clocks[0]);

    /* Issue GET_DATA_RATES request.  */
    transfer->ux_transfer_request_type              = 0xA1;
    transfer->ux_transfer_request_function          = 0x03;
    transfer->ux_transfer_request_index             = interface->ux_interface_descriptor.bInterfaceNumber;
    transfer->ux_transfer_request_value             = 0;
    transfer->ux_transfer_request_requested_length  = sizeof(buffer);
    transfer->ux_transfer_request_data_pointer      = (UCHAR*)buffer;
    status = ux_host_stack_transfer_request(transfer);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT(transfer->ux_transfer_request_actual_length == 4);
    UX_TEST_ASSERT(buffer[0] == device_ccid_data_rates[0]);
}



void  tx_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
ULONG                                               test_n;
ULONG                                               mem_free;
ULONG                                               loop;
ULONG                                               parameter_u32[64/4];
USHORT                                              *parameter_u16 = (USHORT*)parameter_u32;
UCHAR                                               *parameter_u8 = (UCHAR*)parameter_u32;


    stepinfo("\n");
    stepinfo(">>>>>>>>>>>>>>>> Test connect\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    status = ux_test_sleep_break_on_success(100, _test_check_host_connection_success);
    UX_TEST_ASSERT(status == UX_SUCCESS);

    _ccid_enumeration_test();

    _ccid_icc_insert_test();
    _ccid_icc_power_on_off_test();
    _ccid_get_slot_status_test();
    _ccid_xfr_block_test();
    _ccid_parameters_test();
    _ccid_escape_test();
    _ccid_icc_clock_test();
    _ccid_t0_apdu_test();
    _ccid_secure_test();
    _ccid_mechanical_test();
    _ccid_abort_test();
    _ccid_data_rate_and_clock_test();

    _ccid_control_requests_test();

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    status = ux_test_sleep_break_on_success(100, _test_check_host_disconnection_success);
    UX_TEST_ASSERT(status == UX_SUCCESS);

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status  = ux_device_stack_class_unregister(_ux_system_device_class_ccid_name, _ux_device_class_ccid_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);

}

void  tx_test_thread_slave_simulation_entry(ULONG arg)
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


static UINT ux_test_ccid_icc_power_on(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_ICC_POWER_ON_HEADER          *icc_power_on;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_DATA_BLOCK_HEADER            *data_block;
    ux_test_callback_log(ux_test_ccid_icc_power_on, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    icc_power_on = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_ICC_POWER_ON_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    data_block = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_DATA_BLOCK_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    /* bPowerSelect.  */
    icc_power_on->bPowerSelect;
    /* Update bStatus,bError.  */
    data_block->bStatus = UX_DEVICE_CLASS_CCID_ICC_ACTIVE;
    // data_block->bError = 0;
    /* Update data length (reply message header only).  */
    io_msg->ux_device_class_ccid_messages_rdr_to_pc_length = 10;
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_icc_power_off(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_ICC_POWER_OFF_HEADER         *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER           *rsp;
    ux_test_callback_log(ux_test_ccid_icc_power_off, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_ICC_POWER_OFF_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    /* Update bStatus,bError.  */
    rsp->bStatus = UX_DEVICE_CLASS_CCID_ICC_INACTIVE;
    // rsp->bError = 0;
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_get_slot_status(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_GET_SLOT_STATUS_HEADER       *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER           *rsp;
    ux_test_callback_log(ux_test_ccid_get_slot_status, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_GET_SLOT_STATUS_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    /* Update bStatus,bError.  */
    // rsp->bStatus = 0;
    // rsp->bError = 0;
    // rsp->bClockStatus = 0;
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
#if defined(UX_DEVICE_STANDALONE)
static UINT _xfr_block_state = 0;
#endif
static UINT ux_test_ccid_xfr_block(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_XFR_BLOCK_HEADER             *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_DATA_BLOCK_HEADER            *rsp;
UCHAR                                                       *cmd_data;
UCHAR                                                       *rsp_data;
UCHAR                                                       i;
    ux_test_callback_log(ux_test_ccid_xfr_block, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_XFR_BLOCK_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_DATA_BLOCK_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    /* Access to data buffers.  */
    cmd_data = (UCHAR*)io_msg->ux_device_class_ccid_messages_pc_to_rdr + UX_DEVICE_CLASS_CCID_MESSAGE_HEADER_LENGTH;
    rsp_data = (UCHAR*)io_msg->ux_device_class_ccid_messages_rdr_to_pc + UX_DEVICE_CLASS_CCID_MESSAGE_HEADER_LENGTH;

#if defined(UX_DEVICE_STANDALONE)

    switch(_xfr_block_state)
    {
    case 0:
        /* Time extension.  */
        ux_device_class_ccid_time_extension(device_ccid, slot, 10);
        _xfr_block_state = 1;
        return(UX_STATE_WAIT);
    case 1:
        _xfr_block_state = 2;
        return(UX_STATE_WAIT);
    default:
        _xfr_block_state = 0;
    }
#else

    /* Time extension.  */
    ux_device_class_ccid_time_extension(device_ccid, slot, 10);
#endif

    /* Update bStatus,bError.  */
    // rsp->bStatus = 0;
    // rsp->bError = 0;

    /* Update data.  */
    for (i = 10; i < 64; i ++)
        rsp_data[i] = i;

    /* Update data length.  */
    UX_DEVICE_CLASS_CCID_MESSAGE_LENGTH_SET(rsp, 64-10);
    io_msg->ux_device_class_ccid_messages_rdr_to_pc_length = 64;

#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_get_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_GET_PARAMETERS_HEADER        *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_HEADER            *rsp;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T0                *t0;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T1                *t1;
    ux_test_callback_log(ux_test_ccid_get_parameters, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_GET_PARAMETERS_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    /* Parameters protocol.  */
    rsp->bProtocolNum = 0;
    if (rsp->bProtocolNum == 0)
    {
        t0 = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T0 *)rsp;
        t0->bmFindexDindex = 0;
        /* ... */
    }
    else if (rsp->bProtocolNum == 1)
    {
        t1 = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T1 *)rsp;
        t1->bmFindexDindex = 0;
        /* ... */
    }
    /* Update bStatus,bError.  */
    // rsp->bStatus = 0;
    // rsp->bError = 0;
    /* Update data length.  */
    UX_DEVICE_CLASS_CCID_MESSAGE_LENGTH_SET(rsp, 5);
    io_msg->ux_device_class_ccid_messages_rdr_to_pc_length = 5+10;
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_reset_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_RESET_PARAMETERS_HEADER      *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_HEADER            *rsp;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T0                *t0;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T1                *t1;
    ux_test_callback_log(ux_test_ccid_reset_parameters, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_RESET_PARAMETERS_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to response.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    /* Parameters protocol.  */
    rsp->bProtocolNum = 0;
    if (rsp->bProtocolNum == 0)
    {
        t0 = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T0 *)rsp;
        t0->bmFindexDindex = 0;
        /* ... */
    }
    else if (rsp->bProtocolNum == 1)
    {
        t1 = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T1 *)rsp;
        t1->bmFindexDindex = 0;
        /* ... */
    }
    /* Update bStatus,bError.  */
    // rsp->bStatus = 0;
    // rsp->bError = 0;
    /* Update data length.  */
    UX_DEVICE_CLASS_CCID_MESSAGE_LENGTH_SET(rsp, 5);
    io_msg->ux_device_class_ccid_messages_rdr_to_pc_length = 5+10;
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_set_parameters(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_SET_PARAMETERS_HEADER        *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_HEADER            *rsp;
UX_DEVICE_CLASS_CCID_PC_TO_RDR_SET_PARAMETERS_T0            *cmd_t0;
UX_DEVICE_CLASS_CCID_PC_TO_RDR_SET_PARAMETERS_T1            *cmd_t1;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T0                *rsp_t0;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T1                *rsp_t1;
    ux_test_callback_log(ux_test_ccid_set_parameters, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_SET_PARAMETERS_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to response.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    if (rsp->bProtocolNum == 0)
    {
        cmd_t0 = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_SET_PARAMETERS_T0 *)cmd;
        rsp_t0 = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T0 *)rsp;
        /* ... */
    }
    else
    {
        cmd_t1 = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_SET_PARAMETERS_T1 *)cmd;
        rsp_t1 = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_PARAMETERS_T1 *)rsp;
    }
    /* Update bStatus,bError.  */
    // rsp->bStatus = 0;
    // rsp->bError = 0;
    /* Update data length.  */
    UX_DEVICE_CLASS_CCID_MESSAGE_LENGTH_SET(rsp, 5);
    io_msg->ux_device_class_ccid_messages_rdr_to_pc_length = 5+10;
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_escape(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_ESCAPE_HEADER            *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_ESCAPE_HEADER            *rsp;
UCHAR                                                   *cmd_data;
UCHAR                                                   *rsp_data;
    ux_test_callback_log(ux_test_ccid_escape, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_ESCAPE_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_ESCAPE_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    cmd_data = io_msg->ux_device_class_ccid_messages_pc_to_rdr + UX_DEVICE_CLASS_CCID_MESSAGE_HEADER_LENGTH;
    rsp_data = io_msg->ux_device_class_ccid_messages_rdr_to_pc + UX_DEVICE_CLASS_CCID_MESSAGE_HEADER_LENGTH;
    /* Update bStatus,bError.  */
    // rsp->bStatus = 0;
    // rsp->bError = 0;
    /* Update data length.  */
    UX_DEVICE_CLASS_CCID_MESSAGE_LENGTH_SET(rsp, 128);
    io_msg->ux_device_class_ccid_messages_rdr_to_pc_length = 128+10;
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_icc_clock(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_ICC_CLOCK_HEADER             *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER           *rsp;
    ux_test_callback_log(ux_test_ccid_icc_clock, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_ICC_CLOCK_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    /* Clock command.  */
    cmd->bClockCommand;
    /* Update bStatus,bError.  */
    // rsp->bStatus = 0;
    // rsp->bError = 0;
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_t0_apdu(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_T0_APDU_HEADER               *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER           *rsp;
    ux_test_callback_log(ux_test_ccid_t0_apdu, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_T0_APDU_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    /* Changes.  */
    if (cmd->bmChanges & UX_DEVICE_CLASS_CCID_CHANGE_CLASS_ENVELOPE)
    {
        cmd->bClassEnvelope;
    }
    if (cmd->bmChanges & UX_DEVICE_CLASS_CCID_CHANGE_CLASS_GET_RESPONSE)
    {
        cmd->bClassGetResponse;
    }
    /* Update bStatus,bError.  */
    rsp->bStatus = 0;
    rsp->bError = 0;

#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_secure(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_SECURE_HEADER            *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_DATA_BLOCK_HEADER        *rsp;
UCHAR                                                   *cmd_data;
UCHAR                                                   *rsp_data;
    ux_test_callback_log(ux_test_ccid_secure, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_SECURE_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_DATA_BLOCK_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;

    cmd->wLevelParameter;
    rsp->bChainParameter;

    cmd_data = io_msg->ux_device_class_ccid_messages_pc_to_rdr + UX_DEVICE_CLASS_CCID_MESSAGE_HEADER_LENGTH;
    rsp_data = io_msg->ux_device_class_ccid_messages_rdr_to_pc + UX_DEVICE_CLASS_CCID_MESSAGE_HEADER_LENGTH;

    /* Update bStatus,bError.  */
    // rsp->bStatus = 0;
    // rsp->bError = 0;
    /* Update data length.  */
    UX_DEVICE_CLASS_CCID_MESSAGE_LENGTH_SET(rsp, 128/8);
    io_msg->ux_device_class_ccid_messages_rdr_to_pc_length = 128/8+10;
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_mechanical(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_MECHANICAL_HEADER            *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER           *rsp;
    ux_test_callback_log(ux_test_ccid_mechanical, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_MECHANICAL_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    /* Update bStatus,bError.  */
    // rsp->bStatus = 0;
    // rsp->bError = 0;
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_abort(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_ABORT_HEADER                 *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER           *rsp;
    /* Control request Abort.  */
    if (io_msg == UX_NULL)
    {
        ux_test_callback_log(ux_test_ccid_abort, UX_NULL);
        return(UX_SUCCESS);
    }
    /* Bulk OUT Abort.  */
    ux_test_callback_log(ux_test_ccid_abort, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_ABORT_HEADER*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_SLOT_STATUS_HEADER*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    /* Update bStatus,bError.  */
    // rsp->bStatus = 0;
    // rsp->bError = 0;
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
static UINT ux_test_ccid_set_data_rate_and_clock_frequency(ULONG slot, UX_DEVICE_CLASS_CCID_MESSAGES*io_msg)
{
UX_DEVICE_CLASS_CCID_PC_TO_RDR_SET_DATA_RATE_AND_CLOCK_FREQUENCY        *cmd;
UX_DEVICE_CLASS_CCID_RDR_TO_PC_DATA_RATE_AND_CLOCK_FREQUENCY            *rsp;
ULONG                                                                   data_rate;
ULONG                                                                   clock;
    ux_test_callback_log(ux_test_ccid_set_data_rate_and_clock_frequency, io_msg->ux_device_class_ccid_messages_pc_to_rdr);
    /* Access to command.  */
    cmd = (UX_DEVICE_CLASS_CCID_PC_TO_RDR_SET_DATA_RATE_AND_CLOCK_FREQUENCY*)io_msg->ux_device_class_ccid_messages_pc_to_rdr;
    /* Access to data.  */
    rsp = (UX_DEVICE_CLASS_CCID_RDR_TO_PC_DATA_RATE_AND_CLOCK_FREQUENCY*)io_msg->ux_device_class_ccid_messages_rdr_to_pc;
    data_rate = UX_DEVICE_CLASS_CCID_PC_TO_RDR_CLOCK_FREQUENCY_GET(cmd);
    clock = UX_DEVICE_CLASS_CCID_PC_TO_RDR_DATA_RATE_GET(cmd);
    /* Update bStatus,bError.  */
    // rsp->bStatus = 0;
    // rsp->bError = 0;
    /* Update response data rate and clock.  */
    UX_DEVICE_CLASS_CCID_RDR_TO_PC_CLOCK_FREQUENCY_SET(rsp, clock);
    UX_DEVICE_CLASS_CCID_RDR_TO_PC_DATA_RATE_SET(rsp, data_rate);
#if defined(UX_DEVICE_STANDALONE)
    return(UX_STATE_NEXT);
#else
    return(UX_SUCCESS);
#endif
}
