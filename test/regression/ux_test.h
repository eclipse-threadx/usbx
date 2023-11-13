/* USBX common things for testing. */

#ifndef _UX_TEST_H
#define _UX_TEST_H

#include "nx_api.h"
#include "ux_api.h"

#if 0
#define stepinfo printf
#else
#define stepinfo(...)
#endif


/* Compile options check.  */
/*
UX_MAX_HCD=1
UX_MAX_CLASS_DRIVER=1
UX_MAX_DEVICES=1
UX_DEVICE_INITIALIZE_FRAMEWORK_SCAN_DISABLE
UX_MAX_DEVICE_ENDPOINTS=2
UX_MAX_DEVICE_INTERFACES=1
UX_MAX_SLAVE_INTERFACES=1
UX_MAX_SLAVE_CLASS_DRIVER=1
UX_MAX_SLAVE_LUN=1
*/
#define UX_TEST_MULTI_HCD_ON        (UX_MAX_HCD > 1)
#define UX_TEST_MULTI_IFC_OVER(n)   (                           \
    (UX_MAX_SLAVE_INTERFACES > (n)) &&                          \
    (!defined(UX_DEVICE_INITIALIZE_FRAMEWORK_SCAN_DISABLE) ||   \
        (UX_MAX_DEVICE_INTERFACES > (n))) )
#define UX_TEST_MULTI_IFC_ON        UX_TEST_MULTI_IFC_OVER(1)
#define UX_TEST_MULTI_ALT_ON        (                           \
    !defined(UX_DEVICE_ALTERNATE_SETTING_SUPPORT_DISABLE))
#define UX_TEST_MULTI_CLS_OVER(n)   (                           \
    UX_MAX_CLASS_DRIVER > (n) &&                                \
    UX_MAX_SLAVE_CLASS_DRIVER > (n) )
#define UX_TEST_MULTI_CLS_ON        UX_TEST_MULTI_CLS_OVER(1)
#define UX_TEST_MULTI_DEV_ON        (UX_MAX_DEVICES > 1)
#define UX_TEST_MULTI_EP_OVER(n)    (                           \
    (UX_MAX_DEVICE_ENDPOINTS > (n)) ||                          \
    (!defined(UX_DEVICE_INITIALIZE_FRAMEWORK_SCAN_DISABLE)))

/* Define macros. */
#define ARRAY_COUNT(array)  (sizeof(array)/sizeof(array[0]))

extern char *ux_test_file_base_name(char *path, int n);
#define UX_TEST_FILE_BASE_NAME(s)                   (ux_test_file_base_name(s, sizeof(s)))
#define UX_TEST_FILE                                UX_TEST_FILE_BASE_NAME(__FILE__)

#define UX_TEST_ASSERT(expression)                  if (!(expression)) { printf("%s:%d Assert failed\n", UX_TEST_FILE, __LINE__); test_control_return(1); }
#define UX_TEST_ASSERT_MESSAGE(expression, ...)     if (!(expression)) { printf("%s:%d Assert failed; ", UX_TEST_FILE, __LINE__); printf(__VA_ARGS__); test_control_return(1); }
#define UX_TEST_CHECK_CODE(code, expression) \
    { \
        UINT __status__ = (expression); \
        if (__status__ != (code)) \
        { \
            printf("%s : %d, error: 0x%x\n", UX_TEST_FILE_BASE_NAME(__FILE__), __LINE__, __status__); \
            test_control_return(1); \
        } \
    }
#define UX_TEST_CHECK_NOT_CODE(code, expression) \
    { \
        UINT __status__ = (expression); \
        if (__status__ == (code)) \
        { \
            printf("%s:%d error: 0x%x\n", UX_TEST_FILE, __LINE__, __status__); \
            test_control_return(1); \
        } \
    }
#define UX_TEST_CHECK_SUCCESS(expression) UX_TEST_CHECK_CODE(UX_SUCCESS, expression)
#define UX_TEST_CHECK_NOT_SUCCESS(expression) UX_TEST_CHECK_NOT_CODE(UX_SUCCESS, expression)

typedef struct UX_TEST_ERROR_CALLBACK_ERROR_STRUCT
{
    UINT system_level;
    UINT system_context;
    UINT error_code;

    struct UX_TEST_ERROR_CALLBACK_ERROR_STRUCT *next;
} UX_TEST_ERROR_CALLBACK_ERROR;

typedef struct UX_TEST_ERROR_CALLBACK_ACTION_STRUCT
{
    UX_TEST_ERROR_CALLBACK_ERROR *error;
    VOID (*entry_func_error_callback_action)();
    UINT do_return;

    struct UX_TEST_ERROR_CALLBACK_ACTION *next;
} UX_TEST_ERROR_CALLBACK_ACTION;

/* General setup request */

typedef struct _UX_TEST_SETUP_STRUCT {
    UCHAR  ux_test_setup_type;
    UCHAR  ux_test_setup_request;
    USHORT ux_test_setup_value;
    USHORT ux_test_setup_index;
} UX_TEST_SETUP;

/* Interaction descriptor to insert action on specific entry function call. */
/* function, request to match, port action, port status, request action, request EP, request data, request actual length, request status, status, additional callback, no return, thread */

typedef struct UX_TEST_ACTION_STRUCT {

    /* Function code to match */
    ULONG function;
    /* Setup request to match */
    UX_TEST_SETUP *req_setup;

    /* Port action flags */
    ULONG port_action;
    ULONG port_status;

    /* Request action flags */
    ULONG req_action;
    ULONG req_ep_address;
    UCHAR *req_data;
    /* For comparing data, use this, not req_requested_len. */
    ULONG  req_actual_len;
    ULONG  req_status;

    /* Status to return */
    ULONG status;

    /* Additional callback */
    VOID (*action_func)(struct UX_TEST_ACTION_STRUCT *action, VOID *params);

    /* No return */
    ULONG no_return;

    ULONG req_requested_len;

    /* _ux_host_stack_class_instance_create */
    UCHAR   *class_name;
    UINT    class_name_length;

    /* _ux_device_stack_transfer_request */
    UCHAR   endpoint_address;
    ULONG   slave_length;
    ULONG   host_length;

    /* ux_host_stack_transfer_request_action */
    ULONG   requested_length;
    UINT    type;
    UINT    value;
    UINT    index;
    UCHAR   *data;
    /* For matching parts of a transfer.  */
    UCHAR   *req_data_match_mask;

    /* _ux_device_stack_transfer_abort (none) */

    /* _ux_host_class_storage_media_write */
    ULONG   sector_start;
    ULONG   sector_count;
    UCHAR   *data_pointer;

    /* _ux_host_stack_interface_set */
    ULONG   bInterfaceNumber;
    ULONG   bAlternateSetting;

    /* _ux_utility_memory_allocate */
    ULONG memory_alignment;
    ULONG memory_cache_flag;
    ULONG memory_size_requested;

    /* if there is name.  */
    CHAR *name_ptr;

    /* tx_semaphore_get */
    TX_SEMAPHORE    *semaphore_ptr;
    ULONG           wait_option;

    /* tx_semaphore_get */
    TX_MUTEX    *mutex_ptr;
    UINT         inherit;

    /* tx_semaphore_create */
    CHAR        *semaphore_name;

    /* ux_test_error_callback */
    UINT system_level;
    UINT system_context;
    UINT error_code;

    /* ux_slave_class_storage_media_read/write */
    VOID    *storage;
    ULONG   lun;
    USHORT  number_blocks;
    ULONG lba;
    ULONG *media_status;

    /* ux_slave_class_storage_media_status */
    ULONG media_id;

    /* tx_thread_preemption_change */
    TX_THREAD *thread_ptr;
    UINT new_threshold;

    /* _ux_host_stack_interface_set */
    UX_INTERFACE *interface;

    /* Thread to match. Necessary due to the following case: The storage class has a background thread that runs every 2
       seconds and performs a bulk transaction. What could happen is that we call ux_test_hcd_sim_host_set_actions and
       get preempted before actually starting the test. This could cause the action to match a transaction made by
       storage's background thread. */
    TX_THREAD *thread_to_match;

    /* Whether we invoke our callback before or after we call the real HCD. */
    UCHAR do_after;

    /* User data. */
    ALIGN_TYPE user_data;

    /* Additional check. */
    UCHAR (*check_func)();

    /* If set, the params are ignored. */
    UCHAR ignore_params;

    struct UX_TEST_ACTION_STRUCT *next;
    struct UX_TEST_ACTION_STRUCT *created_list_next;
    UCHAR matched;
    ULONG usbx_function;
} UX_TEST_ACTION;

typedef UX_TEST_ACTION UX_TEST_SIM_ENTRY_ACTION;

typedef enum UX_TEST_FUNCTIONS
{
    UX_TEST_NULL,
    UX_TEST_OVERRIDE_TX_SEMAPHORE_GET,
    UX_TEST_OVERRIDE_TX_SEMAPHORE_CREATE,
    UX_TEST_OVERRIDE_TX_THREAD_PREEMPTION_CHANGE,
    UX_TEST_OVERRIDE_TX_THREAD_CREATE,
    UX_TEST_OVERRIDE_NX_PACKET_POOL_CREATE,
    UX_TEST_OVERRIDE_NX_PACKET_ALLOCATE,
    UX_TEST_OVERRIDE_TX_MUTEX_CREATE,
    UX_TEST_OVERRIDE_TX_MUTEX_PUT,
    UX_TEST_OVERRIDE_TX_MUTEX_GET,
    UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
    UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION,
    UX_TEST_OVERRIDE_UX_UTILITY_MEMORY_ALLOCATE,

    /* Define functions where it's okay to return/change the logic. This should
       only include user callbacks.  */
    UX_TEST_OVERRIDE_USER_CALLBACKS,
    UX_TEST_OVERRIDE_ERROR_CALLBACK,
    UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ,
    UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_WRITE,
    UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_FLUSH,
    UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_STATUS,

    /* Only for race condition tests. */
    UX_TEST_OVERRIDE_RACE_CONDITION_OVERRIDES,
    UX_TEST_OVERRIDE_UX_HOST_STACK_TRANSFER_REQUEST,
    UX_TEST_OVERRIDE_UX_HOST_STACK_INTERFACE_SET,

    UX_TEST_NUMBER_OVERRIDES,
} UX_TEST_FUNCTION;

typedef struct UX_TEST_OVERRIDE_NX_PACKET_ALLOCATE_PARAMS
{
    NX_PACKET_POOL  *pool_ptr;
} UX_TEST_OVERRIDE_NX_PACKET_ALLOCATE_PARAMS;

typedef struct UX_TEST_OVERRIDE_NX_PACKET_POOL_CREATE_PARAMS
{
    CHAR *name_ptr;
} UX_TEST_OVERRIDE_NX_PACKET_POOL_CREATE_PARAMS;

typedef struct UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_PARAMS
{
    VOID *storage;
    ULONG lun;
    UCHAR *data_pointer;
    ULONG number_blocks;
    ULONG lba;
    ULONG *media_status;
} UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS;

typedef struct UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_STATUS_PARAMS
{
    VOID *storage;
    ULONG lun;
    ULONG media_id;
    ULONG *media_status;
} UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_STATUS_PARAMS;

typedef struct UX_TEST_OVERRIDE_UX_UTILITY_MEMORY_ALLOCATE_PARAMS
{
    ULONG memory_alignment;
    ULONG memory_cache_flag;
    ULONG memory_size_requested;
} UX_TEST_OVERRIDE_UX_UTILITY_MEMORY_ALLOCATE_PARAMS;

typedef struct UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS
{
    UX_SLAVE_DCD *dcd;
    UINT function;
    VOID *parameter;
} UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS;

typedef struct UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS
{
    UX_HCD *hcd;
    UINT function;
    VOID *parameter;
} UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS;

typedef struct UX_TEST_OVERRIDE_TX_SEMAPHORE_GET_PARAMS
{
    TX_SEMAPHORE *semaphore_ptr;
    ULONG wait_option;
} UX_TEST_OVERRIDE_TX_SEMAPHORE_GET_PARAMS;

typedef struct UX_TEST_OVERRIDE_TX_MUTEX_CREATE_PARAMS
{
    TX_MUTEX *mutex_ptr;
    CHAR     *name_ptr;
    UINT     inherit;
} UX_TEST_OVERRIDE_TX_MUTEX_CREATE_PARAMS;

typedef struct UX_TEST_OVERRIDE_TX_MUTEX_GET_PARAMS
{
    TX_MUTEX *mutex_ptr;
    ULONG wait_option;
} UX_TEST_OVERRIDE_TX_MUTEX_GET_PARAMS;

typedef struct UX_TEST_OVERRIDE_TX_MUTEX_PUT_PARAMS
{
    TX_MUTEX *mutex_ptr;
} UX_TEST_OVERRIDE_TX_MUTEX_PUT_PARAMS;

typedef struct UX_TEST_OVERRIDE_TX_SEMAPHORE_CREATE_PARAMS
{
    TX_SEMAPHORE *semaphore;
    CHAR *semaphore_name;
    UINT initial_count;
} UX_TEST_OVERRIDE_TX_SEMAPHORE_CREATE_PARAMS;

typedef struct UX_TEST_OVERRIDE_TX_THREAD_CREATE_PARAMS
{
    /* We only care about the name. */
    CHAR *name_ptr;
} UX_TEST_OVERRIDE_TX_THREAD_CREATE_PARAMS;

typedef struct UX_TEST_OVERRIDE_UX_HOST_STACK_TRANSFER_REQUEST_PARAMS
{
    UX_TRANSFER *transfer_request;
} UX_TEST_OVERRIDE_UX_HOST_STACK_TRANSFER_REQUEST_PARAMS;

typedef struct UX_TEST_OVERRIDE_TX_THREAD_PREEMPTION_CHANGE_PARAMS
{
    TX_THREAD *thread_ptr;
    UINT new_threshold;
} UX_TEST_OVERRIDE_TX_THREAD_PREEMPTION_CHANGE_PARAMS;

typedef struct UX_TEST_OVERRIDE_UX_HOST_STACK_INTERFACE_SET_PARAMS
{
    UX_INTERFACE *interface;
} UX_TEST_OVERRIDE_UX_HOST_STACK_INTERFACE_SET_PARAMS;

typedef struct UX_TEST_OVERRIDE_NX_PACKET_TRANSMIT_RELEASE_PARAMS
{
    NX_PACKET **packet_ptr_ptr;
} UX_TEST_OVERRIDE_NX_PACKET_TRANSMIT_RELEASE_PARAMS;

typedef struct UX_TEST_ERROR_CALLBACK_PARAMS
{
    UINT system_level;
    UINT system_context;
    UINT error_code;
} UX_TEST_ERROR_CALLBACK_PARAMS;

typedef struct UX_TEST_GENERIC_CD
{
    VOID *controller_driver;
    UINT function;
    VOID *parameter;
} UX_TEST_GENERIC_CD;

typedef struct UX_MEMORY_BLOCK_STRUCT
{
    struct UX_MEMORY_BLOCK_STRUCT *ux_memory_block_next;
    UX_MEMORY_BYTE_POOL           *ux_memory_byte_pool;
} UX_MEMORY_BLOCK;

UINT ux_test_list_action_compare(UX_TEST_ACTION *list_item, UX_TEST_ACTION *action);

VOID ux_test_remove_hook(UX_TEST_ACTION *action);
VOID ux_test_remove_hooks_from_array(UX_TEST_ACTION *actions);
VOID ux_test_remove_hooks_from_list(UX_TEST_ACTION *actions);

UINT ux_test_link_hook(UX_TEST_ACTION *action);
VOID ux_test_link_hooks_from_array(UX_TEST_ACTION *actions);
VOID ux_test_link_hooks_from_list(UX_TEST_ACTION *actions);

UINT ux_test_add_hook(UX_TEST_ACTION action);
VOID ux_test_add_hooks_from_array(UX_TEST_ACTION *actions);
VOID ux_test_add_hooks_from_list(UX_TEST_ACTION *actions);

ULONG ux_test_do_hooks_before(UX_TEST_FUNCTION usbx_function, VOID *params);
VOID ux_test_do_hooks_after(UX_TEST_FUNCTION usbx_function, VOID *params);

VOID ux_test_free_hook_actions();

VOID ux_test_free_user_list_actions();
VOID ux_test_set_main_action_list_from_array(UX_TEST_ACTION *action);
VOID ux_test_set_main_action_list_from_list(UX_TEST_ACTION *new_actions);
VOID ux_test_add_action_to_user_list(UX_TEST_ACTION *list, UX_TEST_ACTION new_action);
VOID ux_test_add_action_to_main_list(UX_TEST_ACTION action);
VOID ux_test_add_action_to_main_list_multiple(UX_TEST_ACTION action, UINT);
VOID ux_test_hcd_sim_host_set_actions(UX_TEST_ACTION *action);
VOID ux_test_dcd_sim_slave_set_actions(UX_TEST_ACTION *action);
UX_TEST_ACTION ux_test_action_handler(UX_TEST_FUNCTION usbx_function, void *_params);
VOID ux_test_do_action_before(UX_TEST_ACTION *action, VOID *params);
VOID ux_test_do_action_after(UX_TEST_ACTION *action, VOID *params);
VOID ux_test_ignore_all_errors();
VOID ux_test_unignore_all_errors();
VOID ux_test_clear_main_list_actions();
VOID ux_test_cleanup_everything(VOID);
VOID ux_test_turn_off_expedient(UCHAR *);
VOID ux_test_turn_on_expedient(UCHAR *);
UCHAR ux_test_is_expedient_on();
VOID ux_test_set_expedient(UCHAR);
ULONG ux_test_calc_total_memory_allocated(ULONG memory_alignment, ULONG memory_cache_flag, ULONG memory_size_requested);
UCHAR ux_test_check_actions_empty();
UINT ux_test_wait_for_empty_actions();
UINT ux_test_get_num_actions_left();
extern UX_TEST_ERROR_CALLBACK_ERROR ux_error_hcd_transfer_stalled;
VOID ux_test_error_callback(UINT system_level, UINT system_context, UINT error_code);
void ux_test_disconnect_slave_and_host_wait_for_enum_completion();
VOID ux_test_connect_slave_and_host_wait_for_enum_completion();
void ux_test_memory_test_initialize();
void ux_test_memory_test_check();
void ux_test_disconnect_slave_and_host_wait_for_enum_completion();
VOID ux_test_wait_for_enum_thread_completion();
UINT ux_test_host_stack_class_instance_get(UX_HOST_CLASS *host_class, UINT class_index, VOID **class_instance);
UINT ux_test_wait_for_value_uint(UINT *current_value, UINT desired_value);
UINT ux_test_wait_for_value_ulong(ULONG *current_value, ULONG desired_value);
UINT ux_test_wait_for_value_uchar(UCHAR *current_value_ptr, UCHAR desired_value);
UINT ux_test_wait_for_non_null(VOID **current_value_ptr);
UINT ux_test_wait_for_null(VOID **current_value_ptr);
VOID ux_test_connect_host_wait_for_enum_completion();
VOID ux_test_disconnect_host_no_wait();
VOID ux_test_disconnect_slave();
VOID ux_test_connect_slave();
VOID ux_test_disconnect_host_wait_for_enum_completion();
VOID ux_test_change_device_parameters(UCHAR * device_framework_high_speed, ULONG device_framework_length_high_speed,
                                      UCHAR * device_framework_full_speed, ULONG device_framework_length_full_speed,
                                      UCHAR * string_framework, ULONG string_framework_length,
                                      UCHAR * language_id_framework, ULONG language_id_framework_length,
                                      UINT(*ux_system_slave_change_function)(ULONG));
UINT ux_test_wait_for_empty_actions_wait_time(UINT wait_time_ms);
UINT ux_test_wait_for_null_wait_time(VOID **current_value_ptr, UINT wait_time_ms);

VOID _ux_test_main_action_list_thread_update(TX_THREAD *old, TX_THREAD *new);
VOID _ux_test_main_action_list_semaphore_update(TX_SEMAPHORE *old, TX_SEMAPHORE *new);
VOID _ux_test_main_action_list_mutex_update(TX_MUTEX *old, TX_MUTEX *new);

UINT ux_test_host_endpoint_write(UX_ENDPOINT *endpoint, UCHAR *buffer, ULONG length, ULONG *actual_length);

/* Action flags */

#define UX_TEST_SIM_REQ_ANSWER 0x80

#define UX_TEST_MATCH_EP      0x01
#define UX_TEST_MATCH_REQ_LEN 0x40

#define UX_TEST_SETUP_MATCH_REQUEST 0x02
#define UX_TEST_SETUP_MATCH_VALUE   0x04
#define UX_TEST_SETUP_MATCH_INDEX   0x08

#define UX_TEST_SETUP_MATCH_REQ (UX_TEST_SETUP_MATCH_REQUEST)
#define UX_TEST_SETUP_MATCH_REQ_V (UX_TEST_SETUP_MATCH_REQ | UX_TEST_SETUP_MATCH_VALUE)
#define UX_TEST_SETUP_MATCH_REQ_V_I (UX_TEST_SETUP_MATCH_REQ_V | UX_TEST_SETUP_MATCH_INDEX)

#define UX_TEST_SETUP_MATCH_EP_REQ (UX_TEST_SETUP_MATCH_REQUEST | UX_TEST_MATCH_EP)
#define UX_TEST_SETUP_MATCH_EP_REQ_V (UX_TEST_SETUP_MATCH_EP_REQ | UX_TEST_SETUP_MATCH_VALUE)
#define UX_TEST_SETUP_MATCH_EP_REQ_V_I (UX_TEST_SETUP_MATCH_EP_REQ_V | UX_TEST_SETUP_MATCH_INDEX)

/* Expected Requests */

#define UX_TEST_SETUP_SetAddress   {0x00,0x05,0x0001,0x0000}
#define UX_TEST_SETUP_GetDevDescr  {0x80,0x06,0x0100,0x0000}
#define UX_TEST_SETUP_GetCfgDescr  {0x80,0x06,0x0200,0x0000}
#define UX_TEST_SETUP_SetConfigure {0x00,0x09,0x0001,0x0000}

#define UX_TEST_SETUP_CDCACM_GetLineCoding    {0xa1, 0x21, 0x0000, 0x0000}
#define UX_TEST_SETUP_CDCACM_SetLineCoding    {0x21, 0x20, 0x0000, 0x0000}
#define UX_TEST_SETUP_CDCACM_SetLineState     {0x21, 0x22, 0x0003, 0x0000}

#define UX_TEST_SETUP_STORAGE_GetMaxLun       {0xa1, 0xfe, 0x0000, 0x0001}
#define UX_TEST_SETUP_STORAGE_Reset           {0x21, 0xff, 0x0000, 0x0000}

#define UX_TEST_PORT_STATUS_DISC    0
#define UX_TEST_PORT_STATUS_LS_CONN UX_PS_CCS | UX_PS_DS_LS
#define UX_TEST_PORT_STATUS_FS_CONN UX_PS_CCS | UX_PS_DS_FS
#define UX_TEST_PORT_STATUS_HS_CONN UX_PS_CCS | UX_PS_DS_HS



UINT ux_test_breakable_sleep(ULONG tick, UINT (*sleep_break_check_callback)(VOID));
UINT ux_test_sleep_break_if(ULONG tick, UINT (*check)(VOID), UINT break_on_match_or_not, UINT rc_to_match);
#define ux_test_sleep(t) ux_test_sleep_break_if(t, UX_NULL, 1, UX_SUCCESS)
#define ux_test_sleep_break_on_success(t,c) ux_test_sleep_break_if(t, c, 1, UX_SUCCESS)
#define ux_test_sleep_break_on_error(t,c) ux_test_sleep_break_if(t, c, 0, UX_SUCCESS)

void  test_control_return(UINT status);

void  ux_test_assert_hit_hint(UCHAR on_off);
void  ux_test_assert_hit_exit(UCHAR on_off);
ULONG ux_test_assert_hit_count_get(void);
void  ux_test_assert_hit_count_reset(void);

static inline int ux_test_memory_is_freed(void *memory)
{
UCHAR               *work_ptr;
UCHAR               *temp_ptr;
ALIGN_TYPE          *free_ptr;
int                 is_free = 0;
    UX_TEST_ASSERT(memory);
    _ux_system_mutex_on(&_ux_system -> ux_system_mutex);
    work_ptr =  UX_VOID_TO_UCHAR_POINTER_CONVERT(memory);
    {
        work_ptr =  UX_UCHAR_POINTER_SUB(work_ptr, UX_MEMORY_BLOCK_HEADER_SIZE);
        temp_ptr =  UX_UCHAR_POINTER_ADD(work_ptr, (sizeof(UCHAR *)));
        free_ptr =  UX_UCHAR_TO_ALIGN_TYPE_POINTER_CONVERT(temp_ptr);
        if ((*free_ptr) == UX_BYTE_BLOCK_FREE)
            is_free = 1;
    }
    _ux_system_mutex_off(&_ux_system -> ux_system_mutex);
    return(is_free);
}
#define ux_test_regular_memory_free() _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available

#endif /* _UX_TEST_H */
