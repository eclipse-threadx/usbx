#include "ux_test.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_dcd_sim_slave.h"

#ifndef _ux_utility_time_elapsed
#define _ux_utility_time_elapsed(t0,t1) ((t1)>=(t0) ? ((t1)-(t0)) : (0xFFFFFFFF - (t0) + (t1)))
#endif

#define UX_TEST_TIMEOUT_MS 3000

static UX_TEST_ACTION ux_test_action_handler_check(UX_TEST_ACTION *action, UX_TEST_FUNCTION usbx_function, void *params, UCHAR advance);

UINT  _ux_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter);
UINT  _ux_dcd_sim_slave_function(UX_SLAVE_DCD *dcd, UINT function, VOID *parameter);
void test_control_return(UINT);
extern ULONG ux_test_port_status;

static UX_TEST_ACTION   *ux_test_hook_action_list; /* Link list, with .next NULL as end.  */
static UX_TEST_ACTION   *ux_test_main_action_list; /* Link list, with .next NULL as end.  */
static UX_TEST_ACTION   *ux_test_user_list_actions;/* Link list, with .created_list_next NULL as end.  */
static UCHAR            ux_test_expedient = 1;
static UCHAR            ignore_all_errors = 0;
static UCHAR            exit_on_errors = 0;
static ULONG            ux_test_memory_test_no_device_memory_free_amount;

static UCHAR            ux_test_assert_hit_hint_off = 0;
static UCHAR            ux_test_assert_hit_exit_off = 0;
static ULONG            ux_test_assert_hit_count = 0;
static UINT             ux_test_assert_hit_exit_code = 1;

VOID _ux_test_main_action_list_thread_update(TX_THREAD *old, TX_THREAD *new)
{
    UX_TEST_ACTION *list = ux_test_main_action_list;
    if (old != new)
    {
        while(list)
        {
            if (list->thread_ptr == old)
                list->thread_ptr == new;
            list = list->next;
        }
    }
}

VOID _ux_test_main_action_list_semaphore_update(TX_SEMAPHORE *old, TX_SEMAPHORE *new)
{
    UX_TEST_ACTION *list = ux_test_main_action_list;
    if (old != new)
    {
        while(list)
        {
            if (list->semaphore_ptr == old)
                list->semaphore_ptr == new;
            list = list->next;
        }
    }
}

VOID _ux_test_main_action_list_mutex_update(TX_MUTEX *old, TX_MUTEX *new)
{
    UX_TEST_ACTION *list = ux_test_main_action_list;
    if (old != new)
    {
        while(list)
        {
            if (list->mutex_ptr == old)
                list->mutex_ptr == new;
            list = list->next;
        }
    }
}


UCHAR _ux_test_check_action_function(UX_TEST_ACTION *new_actions)
{

UCHAR result;


#ifndef UX_TEST_RACE_CONDITION_TESTS_ON
    result = (new_actions->usbx_function > 0 && new_actions->usbx_function < (ULONG)UX_TEST_OVERRIDE_RACE_CONDITION_OVERRIDES);
#else
    result = (new_actions->_usbx_function > 0 && new_actions->_usbx_function < (ULONG)UX_TEST_NUMBER_OVERRIDES);
#endif

    return result;
}

VOID _ux_test_append_action(UX_TEST_ACTION *list, UX_TEST_ACTION *new_action)
{

UX_TEST_ACTION *tail;


    tail = list;
    while (tail->next != UX_NULL)
        tail = tail->next;
    tail->next = new_action;
}

UINT ux_test_list_action_compare(UX_TEST_ACTION *list_item, UX_TEST_ACTION *action)
{
UX_TEST_ACTION temp;
UINT           status;

    /* It's created one, check its contents.  */
    if (list_item->created_list_next == list_item)
    {
        /* Copy action contents to test.  */
        temp = *action;

        /* Set action next, created to match.  */
        temp.next = list_item->next;
        temp.created_list_next = list_item->created_list_next;

        /* Compare.  */
        status = ux_utility_memory_compare(list_item, &temp, sizeof(temp));

        /* Return success if equal.  */
        if (status == UX_SUCCESS)
            return status;
    }
    else
    {

        /* It's static one, check address.  */
        if (list_item == action)
            return UX_SUCCESS;
    }

    /* No, they do not match.  */
    return UX_NO_CLASS_MATCH;
}

VOID ux_test_remove_hook(UX_TEST_ACTION *action)
{
UX_TEST_ACTION *item;
UX_TEST_ACTION *previous;

    item = ux_test_hook_action_list;
    while(item)
    {
        if (ux_test_list_action_compare(item, action) != UX_SUCCESS)
        {
            previous = item;
            item = item->next;
            continue;
        }

        /* Remove action from head. */
        if (item == ux_test_hook_action_list)
        {
            ux_test_hook_action_list = item->next;
        }

        /* Remove action from list. */
        else
        {
            previous->next = action->next;
        }

        /* Free if it's created.  */
        if (action->created_list_next == action)
            free(action);

        /* Remove is done. */
        return;
    }
}

UINT ux_test_link_hook(UX_TEST_ACTION *action)
{
UX_TEST_ACTION  *tail;

    if (ux_test_hook_action_list == UX_NULL)
    {
        ux_test_hook_action_list = action;
    }
    else
    {
        tail = ux_test_hook_action_list;
        while(tail)
        {
            /* Check existing. */
            if (ux_test_list_action_compare(tail, action) == UX_SUCCESS)
                return UX_ERROR;

            /* Check next.  */
            if (tail->next == UX_NULL)
                break;

            tail = tail->next;
        }
        tail->next = action;
    }
    return UX_SUCCESS;
}

UINT ux_test_add_hook(UX_TEST_ACTION action)
{
UX_TEST_ACTION *created_action;
UINT           status;

    created_action = (UX_TEST_ACTION *)malloc(sizeof(UX_TEST_ACTION));
    UX_TEST_ASSERT(created_action);

    *created_action = action;

    created_action->next = UX_NULL;

    /* We use the memory pointer to indicate it's added by memory allocation.
     * On clean up or free we should free allocated memory.
     */
    created_action->created_list_next = created_action;

    status = ux_test_link_hook(created_action);
    if (status != UX_SUCCESS)
    {
        free(created_action);
    }
    return status;
}

VOID ux_test_remove_hooks_from_array(UX_TEST_ACTION *actions)
{
UX_TEST_ACTION *action;

    action = actions;
    while(action->usbx_function)
    {
        //printf("rm %p\n", action);
        UX_TEST_ASSERT(_ux_test_check_action_function(action));
        ux_test_remove_hook(action);
        action ++;
    }
}

VOID ux_test_remove_hooks_from_list(UX_TEST_ACTION *actions)
{
UX_TEST_ACTION *action;
UX_TEST_ACTION *next;

    action = actions;
    while(action)
    {
        UX_TEST_ASSERT(_ux_test_check_action_function(action));
        next = action->next;
        ux_test_remove_hook(action);
        action = next;
    }
}

VOID ux_test_link_hooks_from_array(UX_TEST_ACTION *actions)
{
UX_TEST_ACTION *action;

    action = actions;
    while(action->usbx_function)
    {
        UX_TEST_ASSERT(_ux_test_check_action_function(action));
        ux_test_link_hook(action);
        action ++;
    }
}

VOID ux_test_link_hooks_from_list(UX_TEST_ACTION *actions)
{
UX_TEST_ACTION *action;

    action = actions;
    while(action)
    {
        UX_TEST_ASSERT(_ux_test_check_action_function(action));
        ux_test_link_hook(action);
        action = action->next;
    }
}

VOID ux_test_add_hooks_from_array(UX_TEST_ACTION *actions)
{
UX_TEST_ACTION *action;

    action = actions;
    while(action->usbx_function)
    {
        UX_TEST_ASSERT(_ux_test_check_action_function(action));
        ux_test_add_hook(*action);
        action ++;
    }
}

VOID ux_test_add_hooks_from_list(UX_TEST_ACTION *actions)
{
UX_TEST_ACTION *action;

    action = actions;
    while(action)
    {
        UX_TEST_ASSERT(_ux_test_check_action_function(action));
        ux_test_add_hook(*action);
        action = action->next;
    }
}

ULONG ux_test_do_hooks_before(UX_TEST_FUNCTION usbx_function, VOID *params)
{
UX_TEST_ACTION *list;
UX_TEST_ACTION action;
ULONG          action_count = 0;
ULONG          action_return = 0;

    list = ux_test_hook_action_list;
    while(list)
    {
        action = ux_test_action_handler_check(list, usbx_function, params, UX_FALSE);
        if (action.matched  && !action.do_after)
        {
            action_count ++;
            if (!action.no_return)
                action_return = 0x80000000u;
        }
        ux_test_do_action_before(&action, params);
        list = list->next;
    }
    return (action_count | action_return);
}

VOID ux_test_do_hooks_after(UX_TEST_FUNCTION usbx_function, VOID *params)
{
UX_TEST_ACTION *list;
UX_TEST_ACTION action;
ULONG          action_count = 0;

    list = ux_test_hook_action_list;
    while(list)
    {
        action = ux_test_action_handler_check(list, usbx_function, params, UX_FALSE);
        if (action.matched)
            action_count ++;
        ux_test_do_action_after(&action, params);
        list = list->next;
    }
}

VOID ux_test_free_hook_actions()
{
UX_TEST_ACTION  *action;

    while(ux_test_hook_action_list)
    {
        action = ux_test_hook_action_list;
        ux_test_hook_action_list = ux_test_hook_action_list->next;

        /* If this action is added/created, free allocated memory.  */
        if (action->created_list_next == action)
        {
            free(action);
        }
    }
}

VOID _ux_test_add_action_to_list(UX_TEST_ACTION *list, UX_TEST_ACTION action)
{

UX_TEST_ACTION  *new_action = UX_NULL;
UX_TEST_ACTION  *tail;


    /* Do some idiot-proof checks.  */
    {
        /* Check the function. */
#ifndef UX_TEST_RACE_CONDITION_TESTS_ON
        UX_TEST_ASSERT((action.usbx_function > 0 && action.usbx_function < (ULONG)UX_TEST_OVERRIDE_RACE_CONDITION_OVERRIDES));
#else
        UX_TEST_ASSERT((action._usbx_function > 0 && action._usbx_function < (ULONG)UX_TEST_NUMBER_OVERRIDES));
#endif

        /* Make sure expedient is on when it should be. */
        if (action.no_return == 0 &&
            /* For user callbacks, it's okay. */
            action.usbx_function < UX_TEST_OVERRIDE_USER_CALLBACKS)
        {
            /* Allowed if flow stopped to avoid low level operations.  */
            #if 0
            UX_TEST_ASSERT(ux_test_is_expedient_on());
            #endif
        }
    }

    if (/* Is this the main list? */
        list == ux_test_main_action_list ||
        /* Is the head of user-list already an action? */
        list->usbx_function)
    {
        new_action = (UX_TEST_ACTION *)malloc(sizeof(UX_TEST_ACTION));
        *new_action = action;
        new_action->next = UX_NULL;
    }

    if (list == ux_test_main_action_list)
    {

        if (!ux_test_main_action_list)
            ux_test_main_action_list = new_action;
        else
            _ux_test_append_action(list, new_action);
    }
    else
    {

        if (!list->usbx_function)
            *list = action;
        else
        {

            /* Append to user created list. */
            if (!ux_test_user_list_actions)
                ux_test_user_list_actions = new_action;
            else
            {

                tail = ux_test_user_list_actions;
                while (tail->created_list_next)
                    tail = tail->created_list_next;
                tail->created_list_next = new_action;
            }

            _ux_test_append_action(list, new_action);
        }
    }
}

VOID ux_test_add_action_to_main_list(UX_TEST_ACTION new_action)
{

    _ux_test_add_action_to_list(ux_test_main_action_list, new_action);
}

VOID ux_test_add_action_to_main_list_multiple(UX_TEST_ACTION new_action, UINT num)
{

    while (num--)
        _ux_test_add_action_to_list(ux_test_main_action_list, new_action);
}

VOID ux_test_add_action_to_user_list(UX_TEST_ACTION *list, UX_TEST_ACTION new_action)
{

    _ux_test_add_action_to_list(list, new_action);
}

VOID ux_test_set_main_action_list_from_list(UX_TEST_ACTION *new_actions)
{

    UX_TEST_ASSERT(!ux_test_main_action_list);
    while (new_actions)
    {

        ux_test_add_action_to_main_list(*new_actions);
        new_actions = new_actions->next;
    }
}

VOID ux_test_set_main_action_list_from_array(UX_TEST_ACTION *new_actions)
{

    UX_TEST_ASSERT(!ux_test_main_action_list);
    while (new_actions->usbx_function)
    {

        ux_test_add_action_to_main_list(*new_actions);
        new_actions++;
    }
}

VOID ux_test_free_user_list_actions()
{

UX_TEST_ACTION *action;


    while (ux_test_user_list_actions)
    {

        action = ux_test_user_list_actions;
        ux_test_user_list_actions = ux_test_user_list_actions->created_list_next;
        free(action);
    }
}

static VOID _ux_test_advance_actions()
{

UX_TEST_ACTION *action;


    UX_TEST_ASSERT(ux_test_main_action_list != UX_NULL);
    action = ux_test_main_action_list;
    ux_test_main_action_list = ux_test_main_action_list->next;
    free(action);
}

VOID ux_test_clear_main_list_actions()
{

    while (ux_test_main_action_list)
        _ux_test_advance_actions();
}

VOID _ux_test_set_actions_and_set_function(UX_TEST_ACTION *new_actions, UX_TEST_FUNCTION function)
{

    UX_TEST_ASSERT(new_actions != UX_NULL);

    while (new_actions->function || new_actions->usbx_function)
    {

        if (new_actions->function && !new_actions->usbx_function)
            new_actions->usbx_function = function;
        ux_test_add_action_to_main_list(*new_actions);
        new_actions++;
    }
}

VOID ux_test_hcd_sim_host_set_actions(UX_TEST_ACTION *new_actions)
{

    ux_test_clear_main_list_actions();

    if (new_actions != UX_NULL)
        _ux_test_set_actions_and_set_function(new_actions, UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY);
}

VOID ux_test_dcd_sim_slave_set_actions(UX_TEST_ACTION *new_actions)
{

    ux_test_clear_main_list_actions();

    if (new_actions != UX_NULL)
        _ux_test_set_actions_and_set_function(new_actions, UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION);
}

/* Returns whether there are any actions in the list.  */
UCHAR ux_test_check_actions_empty()
{
    return(ux_test_main_action_list == UX_NULL ? UX_TRUE : UX_FALSE);
}

UINT ux_test_wait_for_empty_actions()
{
    return ux_test_wait_for_null((VOID **)&ux_test_main_action_list);
}

UINT ux_test_wait_for_empty_actions_wait_time(UINT wait_time_ms)
{
    return ux_test_wait_for_null_wait_time((VOID **)&ux_test_main_action_list, wait_time_ms);
}

UINT ux_test_get_num_actions_left()
{

UINT            num_actions_remaining = 0;
UX_TEST_ACTION  *action = ux_test_main_action_list;


    while (action)
    {
        num_actions_remaining++;
        action = action->next;
    }

    return(num_actions_remaining);
}

VOID _ux_host_class_storage_driver_read_write_notify(VOID *func);

VOID ux_test_cleanup_everything(VOID)
{

    /* Free main list actions. */
    ux_test_clear_main_list_actions();

    /* Free allocated user list actions.  */
    ux_test_free_user_list_actions();

    /* Free added hook actions.  */
    ux_test_free_hook_actions();

    /* Reset expedient value.  */
    ux_test_expedient = UX_TRUE;

    /* Reset free memory test amount. */
    ux_test_memory_test_no_device_memory_free_amount = 0;

    /* Assert related.  */
    ux_test_assert_hit_count = 0;
    ux_test_assert_hit_hint_off = 0;
    ux_test_assert_hit_exit_off = 0;
    ux_test_assert_hit_exit_code = 1;

#if defined(UX_HOST_CLASS_STORAGE_NO_FILEX)
    _ux_host_class_storage_driver_read_write_notify(UX_NULL);
#endif
}

VOID ux_test_do_action_before(UX_TEST_ACTION *action, VOID *params)
{

    if (action->matched && !action->do_after)
    {
        if (action->action_func)
        {
            action->action_func(action, params);
        }
    }
}

VOID ux_test_do_action_after(UX_TEST_ACTION *action, VOID *params)
{

    if (action->matched && action->do_after)
    {
        if (action->action_func)
        {
            action->action_func(action, params);
        }
    }
}

VOID ux_test_turn_on_expedient(UCHAR *original_expedient)
{

    if (original_expedient)
    {
        *original_expedient = ux_test_expedient;
    }
    ux_test_expedient = 1;
}

VOID ux_test_turn_off_expedient(UCHAR *original_expedient)
{

    if (original_expedient)
    {
        *original_expedient = ux_test_expedient;
    }
    ux_test_expedient = 0;
}

VOID ux_test_set_expedient(UCHAR value)
{

    ux_test_expedient = value;
}

UCHAR ux_test_is_expedient_on()
{

    return(ux_test_expedient);
}

UX_TEST_ACTION ux_test_action_handler(UX_TEST_FUNCTION usbx_function, void *params)
{
    return ux_test_action_handler_check(ux_test_main_action_list, usbx_function, params, UX_TRUE);
}

static UX_TEST_ACTION ux_test_action_handler_check(UX_TEST_ACTION *list, UX_TEST_FUNCTION usbx_function, void *params, UCHAR advance)
{

UINT                                                    i;
UINT                                                    min;
UX_TEST_ACTION                                          *this = list;
UX_TEST_ACTION                                          result = { 0 };
UCHAR                                                   act = 0;
UX_TRANSFER                                             *host_req;
UX_SLAVE_TRANSFER                                       *slave_req;
ULONG                                                   *slave_req_code = UX_NULL;
UINT                                                    *req_code = UX_NULL;
ULONG                                                   *req_actual_len = UX_NULL;
UX_ENDPOINT                                             *ep;
TX_THREAD                                               *this_thread;
UX_TEST_SETUP                                           req_setup;
UX_TEST_SIM_ENTRY_ACTION                                generic_transfer_parameter = { 0 };
UX_TEST_GENERIC_CD                                      *generic_cd_params;
UX_TEST_GENERIC_CD                                      _generic_cd_params;
UINT                                                    str_len0;
UINT                                                    str_len1;
UCHAR                                                   ignore_param_checks = 0;
UX_TEST_OVERRIDE_TX_SEMAPHORE_GET_PARAMS                *semaphore_get_params;
UX_TEST_OVERRIDE_TX_SEMAPHORE_CREATE_PARAMS             *semaphore_create_params;
UX_TEST_OVERRIDE_TX_THREAD_CREATE_PARAMS                *thread_create_params;
UX_TEST_ERROR_CALLBACK_PARAMS                           *error_callback_params;
UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ_WRITE_FLUSH_PARAMS*device_media_read_write_flush_params;
UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_STATUS_PARAMS          *device_media_status_params;
UX_TEST_OVERRIDE_UX_HOST_STACK_TRANSFER_REQUEST_PARAMS  *host_stack_transfer_request_params;
UX_TEST_OVERRIDE_TX_THREAD_PREEMPTION_CHANGE_PARAMS     *thread_preemption_change_params;
UX_TEST_OVERRIDE_UX_HOST_STACK_INTERFACE_SET_PARAMS     *host_stack_interface_set_params;
UX_TEST_OVERRIDE_TX_MUTEX_GET_PARAMS                    *mutex_get_params;
UX_TEST_OVERRIDE_TX_MUTEX_PUT_PARAMS                    *mutex_put_params;
UX_TEST_OVERRIDE_TX_MUTEX_CREATE_PARAMS                 *mutex_create_params;
UX_TEST_OVERRIDE_NX_PACKET_POOL_CREATE_PARAMS           *packet_pool_create_params;
UX_TEST_OVERRIDE_NX_PACKET_ALLOCATE_PARAMS              *packet_allocate_params;


    if (this)
    {

        if (this->usbx_function == usbx_function)
        {

            /* If appropriate, setup generic controller driver (CD) parameter. */
            if (this->usbx_function == UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY ||
                this->usbx_function == UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION ||
                this->usbx_function == UX_TEST_OVERRIDE_UX_HOST_STACK_TRANSFER_REQUEST)
            {
                /* Treat a transfer request action like an HCD action. */
                if (usbx_function == UX_TEST_OVERRIDE_UX_HOST_STACK_TRANSFER_REQUEST)
                {
                    host_stack_transfer_request_params = params;
                    generic_cd_params = &_generic_cd_params;
                    generic_cd_params->parameter = host_stack_transfer_request_params->transfer_request;
                    generic_cd_params->function = UX_HCD_TRANSFER_REQUEST;
                }
                else
                {
                    generic_cd_params = params;
                }
            }

            /* Should we ignore the param checking? */
            if (this->ignore_params)
            {
                if (this->usbx_function == UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY ||
                    this->usbx_function == UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION)
                {
                    /* Check the sub-function. */
                    if (this->function == generic_cd_params->function)
                    {
                        ignore_param_checks = 1;
                    }
                }
                else
                {
                    /* There is no sub-function. */
                    ignore_param_checks = 1;
                }
            }

            act = 1;
            if (!ignore_param_checks)
            {
                switch (usbx_function)
                {

                case UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY:
                case UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION:
                case UX_TEST_OVERRIDE_UX_HOST_STACK_TRANSFER_REQUEST:

                    act = 0;
                    if (this->function == generic_cd_params->function)
                    {

                        act = 1;
                        switch (generic_cd_params->function)
                        {

                            /* We have action on endpoint entries */
                        case UX_HCD_CREATE_ENDPOINT:
                        //case UX_DCD_CREATE_ENDPOINT:
                        case UX_HCD_DESTROY_ENDPOINT:
                        //case UX_DCD_DESTROY_ENDPOINT:
                        case UX_HCD_RESET_ENDPOINT:
                        //case UX_DCD_RESET_ENDPOINT:
                        case UX_DCD_ENDPOINT_STATUS:
                        case UX_DCD_STALL_ENDPOINT:

                            ep = generic_cd_params->parameter;

                            if ((this->req_action & UX_TEST_MATCH_EP) &&
                                this->req_ep_address != ep->ux_endpoint_descriptor.bEndpointAddress)
                                act = 0;

                            break;

                            /* We have action on transfer request abort */
                        case UX_HCD_TRANSFER_ABORT:
                        //case UX_DCD_TRANSFER_ABORT:

                            if (UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION != usbx_function)
                            {

                                host_req = (UX_TRANSFER *)generic_cd_params->parameter;
                                if ((this->req_action & UX_TEST_MATCH_EP) &&
                                    this->req_ep_address != host_req->ux_transfer_request_endpoint->ux_endpoint_descriptor.bEndpointAddress)
                                    act = 0;
                            }
                            else
                            {

                                slave_req = (UX_SLAVE_TRANSFER *)generic_cd_params->parameter;
                                if ((this->req_action & UX_TEST_MATCH_EP) &&
                                    this->req_ep_address != slave_req->ux_slave_transfer_request_endpoint->ux_slave_endpoint_descriptor.bEndpointAddress)
                                    act = 0;
                            }
                            break;

                            /* We have action on transfer request */
                        case UX_HCD_TRANSFER_REQUEST:
                        //case UX_DCD_TRANSFER_REQUEST:

                            generic_transfer_parameter.req_setup = &req_setup;

                            if (usbx_function == UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY ||
                                usbx_function == UX_TEST_OVERRIDE_UX_HOST_STACK_TRANSFER_REQUEST)
                            {
                                host_req = generic_cd_params->parameter;

                                generic_transfer_parameter.req_actual_len = host_req->ux_transfer_request_actual_length;
                                generic_transfer_parameter.req_data = host_req->ux_transfer_request_data_pointer;
                                generic_transfer_parameter.req_ep_address = host_req->ux_transfer_request_endpoint->ux_endpoint_descriptor.bEndpointAddress;
                                generic_transfer_parameter.req_requested_len = host_req->ux_transfer_request_requested_length;
                                generic_transfer_parameter.req_status = host_req->ux_transfer_request_status;
                                generic_transfer_parameter.req_setup->ux_test_setup_index = host_req->ux_transfer_request_index;
                                generic_transfer_parameter.req_setup->ux_test_setup_request = host_req->ux_transfer_request_function;
                                generic_transfer_parameter.req_setup->ux_test_setup_type = host_req->ux_transfer_request_type;
                                generic_transfer_parameter.req_setup->ux_test_setup_value = host_req->ux_transfer_request_value;

                                req_actual_len = &host_req->ux_transfer_request_actual_length;
                                req_code = &host_req->ux_transfer_request_completion_code;
                            }
                            else
                            {
                                slave_req = generic_cd_params->parameter;

                                generic_transfer_parameter.req_actual_len = slave_req->ux_slave_transfer_request_actual_length;
                                generic_transfer_parameter.req_data = slave_req->ux_slave_transfer_request_data_pointer;
                                generic_transfer_parameter.req_ep_address = slave_req->ux_slave_transfer_request_endpoint->ux_slave_endpoint_descriptor.bEndpointAddress;
                                generic_transfer_parameter.req_requested_len = slave_req->ux_slave_transfer_request_requested_length;
                                generic_transfer_parameter.req_status = slave_req->ux_slave_transfer_request_status;
                                generic_transfer_parameter.req_setup->ux_test_setup_type = (UCHAR)slave_req->ux_slave_transfer_request_type;

                                req_actual_len = &slave_req->ux_slave_transfer_request_actual_length;
                                slave_req_code = &slave_req->ux_slave_transfer_request_completion_code;
                            }

                            if (this->req_action & UX_TEST_MATCH_EP &&
                                this->req_ep_address != generic_transfer_parameter.req_ep_address)
                                act = 0;

                            /* We must confirm request setup is matching */
                            if (this->req_setup)
                            {

                                if ((this->req_action & UX_TEST_SETUP_MATCH_REQUEST) &&
                                    (generic_transfer_parameter.req_setup->ux_test_setup_type != this->req_setup->ux_test_setup_type))
                                    act = 0;

                                if ((this->req_action & UX_TEST_SETUP_MATCH_REQUEST) &&
                                    (generic_transfer_parameter.req_setup->ux_test_setup_request != this->req_setup->ux_test_setup_request))
                                    act = 0;

                                if ((this->req_action & UX_TEST_SETUP_MATCH_VALUE) &&
                                    (generic_transfer_parameter.req_setup->ux_test_setup_value != this->req_setup->ux_test_setup_value))
                                    act = 0;

                                if ((this->req_action & UX_TEST_SETUP_MATCH_INDEX) &&
                                    (generic_transfer_parameter.req_setup->ux_test_setup_index != this->req_setup->ux_test_setup_index))
                                    act = 0;
                            }

                            /* Compare data. We only do the comparison if this action is not meant for substituting data. */
                            if (!(this->req_action & UX_TEST_SIM_REQ_ANSWER) && this->req_data)
                            {
                                if (generic_transfer_parameter.req_data == UX_NULL ||
                                    /* Make sure we don't go off the end during the compare. */
                                    generic_transfer_parameter.req_requested_len < this->req_actual_len)
                                    act = 0;
                                else
                                {

                                    /* Is there no mask? */
                                    if (!this->req_data_match_mask)
                                    {
                                        if (ux_utility_memory_compare(generic_transfer_parameter.req_data, this->req_data, this->req_actual_len) == UX_ERROR)
                                        {
                                            act = 0;
                                        }
                                    }
                                    else
                                    {
                                        /* Compare with mask. */
                                        for (i = 0; i < this->req_actual_len; i++)
                                        {
                                            if (this->req_data_match_mask[i] &&
                                                this->req_data[i] != generic_transfer_parameter.req_data[i])
                                            {
                                                act = 0;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }

                            /* Compare requested lengths. */
                            if ((this->req_action & UX_TEST_MATCH_REQ_LEN) &&
                                (generic_transfer_parameter.req_requested_len != this->req_requested_len))
                                act = 0;

                            /* Do additional check. */
                            if (this->check_func && !this->check_func())
                                act = 0;

                            break;

                        case UX_DCD_CHANGE_STATE:
                        case UX_HCD_RESET_PORT:
                        case UX_HCD_ENABLE_PORT:
                        case UX_HCD_GET_PORT_STATUS:

                            /* AFAIK, there's nothing to check here. */

                            break;

                            /* The other entries are not handled now */
                        default:
                            UX_TEST_ASSERT(0);
                            break;
                        }
                    }
                    break;

    #if 0
                case UX_TEST_OVERRIDE_UX_UTILITY_MEMORY_ALLOCATE:

                    memory_allocate_params = params;
                    if (memory_allocate_params->memory_alignment != this->memory_alignment ||
                        memory_allocate_params->memory_cache_flag != this->memory_cache_flag ||
                        memory_allocate_params->memory_size_requested != this->memory_size_requested)
                    {
                        act = 0;
                    }
                    break;
    #endif

                case UX_TEST_OVERRIDE_TX_SEMAPHORE_GET:

                    semaphore_get_params = params;

                    if (this->semaphore_ptr != UX_NULL && semaphore_get_params->semaphore_ptr != this->semaphore_ptr ||
                        semaphore_get_params->wait_option != this->wait_option)
                    {
                        act = 0;
                    }
                    break;

                case UX_TEST_OVERRIDE_ERROR_CALLBACK:

                    error_callback_params = params;
                    if ((this->error_code && error_callback_params->error_code != this->error_code) ||
                        (this->system_context && error_callback_params->system_context != this->system_context) ||
                        (this->system_level && error_callback_params->system_level != this->system_level))
                    {
                        act = 0;
                    }
                    break;

                case UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_READ:

                    device_media_read_write_flush_params = params;
                    if ((device_media_read_write_flush_params->lba != this->lba ||
                        device_media_read_write_flush_params->lun != this->lun ||
                        device_media_read_write_flush_params->number_blocks != this->number_blocks))
                    {
                        act = 0;
                    }
                    break;

                case UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_WRITE:

                    device_media_read_write_flush_params = params;
                    if ((device_media_read_write_flush_params->lba != this->lba ||
                        device_media_read_write_flush_params->lun != this->lun ||
                        device_media_read_write_flush_params->number_blocks != this->number_blocks))
                    {
                        act = 0;
                    }
                    break;

                case UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_FLUSH:

                    device_media_read_write_flush_params = params;
                    if ((device_media_read_write_flush_params->lba != this->lba ||
                        device_media_read_write_flush_params->lun != this->lun ||
                        device_media_read_write_flush_params->number_blocks != this->number_blocks))
                    {
                        act = 0;
                    }
                    break;

                case UX_TEST_OVERRIDE_UX_DEVICE_MEDIA_STATUS:

                    device_media_status_params = params;
                    if ((device_media_status_params->lun != this->lun ||
                        device_media_status_params->media_id != this->media_id))
                    {
                        act = 0;
                    }
                    break;

                case UX_TEST_OVERRIDE_NX_PACKET_POOL_CREATE:

                    packet_pool_create_params = params;

                    /* We just compare names. */

                    UX_TEST_ASSERT(this->name_ptr != UX_NULL);

                    if (strcmp(packet_pool_create_params->name_ptr, this->name_ptr))
                    {
                        act = 0;
                    }

                    break;

                case UX_TEST_OVERRIDE_NX_PACKET_ALLOCATE:

                    packet_allocate_params = params;

                    /* We just compare names. */

                    UX_TEST_ASSERT(this->name_ptr != UX_NULL);

                    if (strcmp(packet_allocate_params->pool_ptr->nx_packet_pool_name, this->name_ptr))
                    {
                        act = 0;
                    }

                    break;

                case UX_TEST_OVERRIDE_TX_THREAD_CREATE:

                    thread_create_params = params;

                    /* We just compare names. */

                    UX_TEST_ASSERT(this->name_ptr != UX_NULL);

                    if (strcmp(thread_create_params->name_ptr, this->name_ptr))
                    {
                        act = 0;
                    }

                    break;

                case UX_TEST_OVERRIDE_TX_SEMAPHORE_CREATE:

                    semaphore_create_params = params;

                    /* We just compare names. */

                    UX_TEST_ASSERT(this->semaphore_name != UX_NULL);

                    if (strcmp(semaphore_create_params->semaphore_name, this->semaphore_name))
                    {
                        act = 0;
                    }

                    break;

                case UX_TEST_OVERRIDE_TX_THREAD_PREEMPTION_CHANGE:

                    thread_preemption_change_params = params;
                    if (this->thread_ptr != thread_preemption_change_params->thread_ptr ||
                        this->new_threshold != thread_preemption_change_params->new_threshold)
                    {
                        act = 0;
                    }
                    break;

                case UX_TEST_OVERRIDE_UX_HOST_STACK_INTERFACE_SET:

                    host_stack_interface_set_params = params;
                    if (this->interface->ux_interface_descriptor.bInterfaceNumber != host_stack_interface_set_params->interface->ux_interface_descriptor.bInterfaceNumber ||
                        this->interface->ux_interface_descriptor.bAlternateSetting != host_stack_interface_set_params->interface->ux_interface_descriptor.bAlternateSetting)
                    {
                        act = 0;
                    }
                    break;

                case UX_TEST_OVERRIDE_TX_MUTEX_GET:

                    mutex_get_params = params;
                    if (this->mutex_ptr &&
                        this->mutex_ptr != mutex_get_params->mutex_ptr)
                    {
                        act = 0;
                    }
                    if (mutex_get_params->wait_option != this->wait_option)
                    {
                        act = 0;
                    }
                    break;

                case UX_TEST_OVERRIDE_TX_MUTEX_CREATE:

                    mutex_create_params = params;
                    if (this->mutex_ptr &&
                        this->mutex_ptr != mutex_create_params->mutex_ptr)
                    {
                        act = 0;
                    }
                    if (mutex_create_params->inherit != this->inherit)
                    {
                        act = 0;
                    }
                    if (this->name_ptr != UX_NULL)
                    {
                        str_len0 = 0;
                        _ux_utility_string_length_check(this->name_ptr, &str_len0, 2048);
                        if (mutex_create_params->name_ptr != UX_NULL)
                        {
                            str_len1 = 0;
                            _ux_utility_string_length_check(mutex_create_params->name_ptr, &str_len1, 2048);
                        }
                        else
                        {
                            str_len1 = 0;
                        }
                        if (str_len0 != str_len1)
                        {
                            act = 0;
                        }
                        if (str_len0 == str_len1 &&
                            ux_utility_memory_compare(this->name_ptr, mutex_create_params->name_ptr, str_len0) != UX_SUCCESS)
                        {
                            act = 0;
                        }
                    }
                    break;

                case UX_TEST_OVERRIDE_TX_MUTEX_PUT:

                    mutex_put_params = params;
                    if (this->mutex_ptr != UX_NULL &&
                        this->mutex_ptr != mutex_put_params->mutex_ptr)
                    {
                        act = 0;
                    }
                    break;
                }
            }

            /* Non-param checks - these checks happen regardless of the "ignore_params"
               value. */

            if (this->thread_to_match)
            {
                this_thread = tx_thread_identify();
                if (this_thread != this->thread_to_match)
                    act = 0;
            }

            if (this->check_func && this->check_func(params) != UX_TRUE)
            {
                act = 0;
            }
        }
    }

    if (act)
    {

        //stepinfo("    action matched; usbx_function: %d\n", usbx_function);

        if (usbx_function == UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY ||
            usbx_function == UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION ||
            usbx_function == UX_TEST_OVERRIDE_UX_HOST_STACK_TRANSFER_REQUEST)
        {

            /* Apply to port status */
            if (this -> port_action)
            {

                UX_TEST_ASSERT(!this->no_return);

                ux_test_port_status = this -> port_status;
                // printf("Port status -> %lx\n", port_status);
                /* Signal change on the port (HCD0.RH.PORT0). */
                _ux_system_host -> ux_system_host_hcd_array -> ux_hcd_root_hub_signal[0] = 1;
                /* Signal detach to host enum thread. */
                _ux_host_semaphore_put(&_ux_system_host -> ux_system_host_enum_semaphore);
            }

            /* Apply to transfer */
            if (this -> req_action & UX_TEST_SIM_REQ_ANSWER)
            {

                /* We should always return, otherwise the data we copy will just
                   get overridden by the actual transfer request. */
                UX_TEST_ASSERT(!this->no_return);

                /* Is there data to copy? */
                if (this->req_data)
                {
                    /* Make sure we don't overflow. */
                    min = generic_transfer_parameter.req_requested_len < this->req_actual_len ? generic_transfer_parameter.req_requested_len : this->req_actual_len;

                    /* Copy the data. */
                    _ux_utility_memory_copy(generic_transfer_parameter.req_data, this->req_data, min);
                }

                /* Set actual length */
                if (this->req_actual_len == (~0))
                    *req_actual_len = generic_transfer_parameter.req_requested_len;
                else
                    *req_actual_len = this->req_actual_len;

                /* Set status code */
                if (usbx_function == UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY ||
                    usbx_function == UX_TEST_OVERRIDE_UX_HOST_STACK_TRANSFER_REQUEST)
                    *req_code = this->req_status;
                else
                    *slave_req_code = this->req_status;
            }
        }

        result = *this;
        result.matched = 1;

        if (advance)
            _ux_test_advance_actions();
    }

    return result;
}

#define SLEEP_STEP 1
UINT ux_test_breakable_sleep(ULONG tick, UINT (*sleep_break_check_callback)(VOID))
{

UINT  status;
ULONG t;

    while(tick) {

        /* Sleep for a while. */
        t = (tick > SLEEP_STEP) ? SLEEP_STEP : tick;
#if defined(UX_HOST_STANDALONE) || defined(UX_DEVICE_STANDALONE)
        ux_system_tasks_run();
#endif
        tx_thread_sleep(t);

        /* Check if we want to break. */
        if (sleep_break_check_callback)
        {

            status = sleep_break_check_callback();
            if (status != UX_SUCCESS)
            {

                /* We break sleep loop!
                   Status is returned for use to check. */
                return status;
            }
        }

        /* Update remaining ticks. */
        tick -= t;
    }

    /* Normal end. */
    return UX_SUCCESS;
}
UINT ux_test_sleep_break_if(ULONG tick, UINT (*check)(VOID),
                            UINT break_on_match_or_not,
                            UINT rc_to_check)
{
ULONG   t0, t1;
UINT    status;

    t0 = tx_time_get();
    while(1)
    {
#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
        tx_thread_relinquish();
#else
        tx_thread_sleep(1);
#endif
        if (check)
        {
            status = check();
            if (break_on_match_or_not)
            {
                /* If RC match expected, break.  */
                if (status == rc_to_check)
                    break;
            }
            else
            {
                /* If RC not match expected, break.  */
                if (status != rc_to_check)
                    break;
            }
        }
        t1 = tx_time_get();
        if (_ux_utility_time_elapsed(t0, t1) >= tick)
        {
            return(UX_TIMEOUT);
        }
    }
    return(UX_SUCCESS);
}

/* Error callback */

UX_TEST_ERROR_CALLBACK_ERROR ux_error_hcd_transfer_stalled = { UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_TRANSFER_STALLED };

VOID ux_test_ignore_all_errors()
{
    ignore_all_errors = UX_TRUE;
}

VOID ux_test_unignore_all_errors()
{
    ignore_all_errors = UX_FALSE;
}

void test_control_return(UINT status);
VOID ux_test_error_callback(UINT system_level, UINT system_context, UINT error_code)
{

UX_TEST_ERROR_CALLBACK_PARAMS   params = { system_level, system_context, error_code };
UX_TEST_ACTION                  action;


    if (ignore_all_errors == UX_TRUE)
    {
        return;
    }

    if (ux_test_do_hooks_before(UX_TEST_OVERRIDE_ERROR_CALLBACK, &params))
        return;

    action = ux_test_action_handler(UX_TEST_OVERRIDE_ERROR_CALLBACK, &params);
    if (action.matched)
    {
        return;
    }

    /* Failed test. Windows has some stupid printf bug where it stalls
       everything if other threads are printing as well. Doesn't matter anyways,
       since I use breakpoints for debugging this error. */
#ifndef WIN32
    printf("%s:%d Unexpected error: 0x%x, 0x%x, 0x%x\n", __FILE__, __LINE__, system_level, system_context, error_code);
#endif

    if (exit_on_errors)
        test_control_return(1);
}

/* Utility methods. */

static ULONG ux_test_memory_test_no_device_memory_free_amount;

void ux_test_memory_test_initialize()
{

UCHAR connected_when_started = 0;

    /* Are we connected? */
    if (_ux_system_host->ux_system_host_hcd_array[0].ux_hcd_nb_devices != 0)
    {
        /* Yes. */
        connected_when_started = 1;

        /* Then disconnect. */
        ux_test_disconnect_slave_and_host_wait_for_enum_completion();
    }

    /* Get normal amount of free memory when disconnected. This is to detect
       memory leaks. */
    ux_test_memory_test_no_device_memory_free_amount = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    ux_test_connect_slave_and_host_wait_for_enum_completion();

    /* With basic free memory amount, do basic memory test. */
    ux_test_disconnect_slave_and_host_wait_for_enum_completion();
    ux_test_connect_slave_and_host_wait_for_enum_completion();

    /* (Let's be nice to the user.) Hello user! :) */
    if (!connected_when_started)
    {
        ux_test_disconnect_slave_and_host_wait_for_enum_completion();
    }
}

void ux_test_memory_test_check()
{

    /* Has the memory check value not been initialized yet? */
    if (ux_test_memory_test_no_device_memory_free_amount == 0)
        return;
    UX_TEST_ASSERT(_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available == ux_test_memory_test_no_device_memory_free_amount);
}

#if defined(UX_HOST_STANDALONE)
static UINT _host_enum_is_pending(void)
{
UX_DEVICE           *enum_device = _ux_system_host -> ux_system_host_enum_device;
    /* Case 1 : there is nothing in enum list, nothing pending.  */
    if (enum_device == UX_NULL)
        return(UX_ERROR);
    /* Case 2 : enum list is not NULL, check enum flags each device.  */
    while(enum_device)
    {
        if (enum_device -> ux_device_flags & UX_DEVICE_FLAG_ENUM)
            return(UX_SUCCESS);
        enum_device = enum_device -> ux_device_enum_next;
    }
    return(UX_ERROR);
}
static UINT _host_rh_removal_is_pending(void)
{
UX_HCD  *hcd = &_ux_system_host -> ux_system_host_hcd_array[0];
    /* If device connected, pending.  */
    if (hcd -> ux_hcd_rh_device_connection)
        return(UX_SUCCESS);
    return(UX_ERROR);
}
#endif

/* Wait for the enum thread to finish whatever it's currently doing. Note that
   this implies it's actually _doing_ something when this is called.

   Specifics: We do this by simply waiting for the the enum thread's semaphore's
   suspended count to be non-zero. */
VOID ux_test_wait_for_enum_thread_completion()
{
#if !defined(UX_HOST_STANDALONE)
    /* Is it actually running? */
    if (_ux_system_host -> ux_system_host_enum_semaphore.tx_semaphore_suspended_count == 0)
    {

        /* Wait for enum thread to complete. */
        UX_TEST_CHECK_SUCCESS(ux_test_wait_for_value_uint(&_ux_system_host -> ux_system_host_enum_semaphore.tx_semaphore_suspended_count, 1));
    }
#else
    {
        UINT    status;
        if (ux_test_port_status & UX_PS_CCS)
        {
            /* Loop a while to confirm enumeration start.  */
            status = ux_test_sleep_break_on_success(10, _host_enum_is_pending);
            if (status == UX_SUCCESS)
            {
                /* Loop a while to confirm enumeration end.  */
                status = ux_test_sleep_break_on_error(100, _host_enum_is_pending);
                UX_ASSERT(status == UX_SUCCESS);
            }
        }
        else
        {
            /* Loop a while to confirm device removal.  */
            status = ux_test_sleep_break_on_error(100, _host_rh_removal_is_pending);
            UX_ASSERT(status == UX_SUCCESS);
        }
    }
#endif
}

VOID ux_test_disconnect_host_no_wait()
{

    ux_test_hcd_sim_host_disconnect_no_wait();
}

VOID ux_test_disconnect_host_wait_for_enum_completion()
{

    /* No wait because ux_test_wait_for_enum_completion() expects the enum thread
       to be running when it's called. */
    ux_test_hcd_sim_host_disconnect_no_wait();
    ux_test_wait_for_enum_thread_completion();
    ux_test_memory_test_check();
}

/* This function should not be called from inside USBX because we wait for the
   deactivation to finish. If we have a semaphore the deactivation routine requires,
   then we have deadlock. */
void ux_test_disconnect_slave_and_host_wait_for_enum_completion()
{

UX_HCD *hcd = &_ux_system_host->ux_system_host_hcd_array[0];

    ux_test_dcd_sim_slave_disconnect();
    /* No wait because below. */
    ux_test_disconnect_host_wait_for_enum_completion();
    ux_test_memory_test_check();
}

VOID ux_test_disconnect_slave()
{

    ux_test_dcd_sim_slave_disconnect();
}

VOID ux_test_connect_slave()
{

    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
}

VOID ux_test_connect_host_wait_for_enum_completion()
{

    /* No wait because ux_test_wait_for_enum_completion() expects the enum thread
       to be running when it's called. */
    ux_test_hcd_sim_host_connect_no_wait(UX_FULL_SPEED_DEVICE);
    ux_test_wait_for_enum_thread_completion();
}

VOID ux_test_connect_slave_and_host_wait_for_enum_completion()
{

    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_connect_host_wait_for_enum_completion();
}

/* This is supposed to only change these parameters. In other words, the same
   classes should still be registered after calling this. */
VOID ux_test_change_device_parameters(UCHAR * device_framework_high_speed, ULONG device_framework_length_high_speed,
                                      UCHAR * device_framework_full_speed, ULONG device_framework_length_full_speed,
                                      UCHAR * string_framework, ULONG string_framework_length,
                                      UCHAR * language_id_framework, ULONG language_id_framework_length,
                                      UINT(*ux_system_slave_change_function)(ULONG))
{

UX_SLAVE_CLASS *class;
UX_SLAVE_CLASS classes_copy[UX_MAX_SLAVE_CLASS_DRIVER];
ULONG          class_index;

    /* Disconnect device. */
    ux_test_dcd_sim_slave_disconnect();

    /* First, save the classes. */
    memcpy(classes_copy, _ux_system_slave->ux_system_slave_class_array, sizeof(classes_copy));

    /* Uninitialize classes. */
    for (class_index = 0; class_index < UX_SYSTEM_DEVICE_MAX_CLASS_GET(); class_index++)
    {

        class = &_ux_system_slave->ux_system_slave_class_array[class_index];
        if (class->ux_slave_class_status == UX_USED)
            UX_TEST_CHECK_SUCCESS(ux_device_stack_class_unregister((UCHAR *)class->ux_slave_class_name, class->ux_slave_class_entry_function));
    }

    /* Uninitialize the stack. */
    UX_TEST_CHECK_SUCCESS(ux_device_stack_uninitialize());

    /* Now re-initialize stack with correct stuff. */
    UX_TEST_CHECK_SUCCESS(ux_device_stack_initialize(device_framework_high_speed, device_framework_length_high_speed,
                                                     device_framework_full_speed, device_framework_length_full_speed,
                                                     string_framework, string_framework_length,
                                                     language_id_framework, language_id_framework_length,
                                                     ux_system_slave_change_function));

    /* Now re-register the classes. */
    for (class_index = 0; class_index < UX_SYSTEM_DEVICE_MAX_CLASS_GET(); class_index++)
    {

        class = &classes_copy[class_index];
        if (class->ux_slave_class_status == UX_USED)
            UX_TEST_CHECK_SUCCESS(ux_device_stack_class_register((UCHAR *)class->ux_slave_class_name,
                                                                 class->ux_slave_class_entry_function,
                                                                 class->ux_slave_class_configuration_number,
                                                                 class->ux_slave_class_interface_number,
                                                                 class->ux_slave_class_interface_parameter));
    }
}

UINT ux_test_host_stack_class_instance_get(UX_HOST_CLASS *host_class, UINT class_index, VOID **class_instance)
{

UINT status;
UINT timeout_ms = UX_TEST_TIMEOUT_MS;
#if defined(UX_HOST_STANDALONE)
ULONG t0, t, elapsed;
    t0 = ux_utility_time_get();
#endif

    do
    {

        status =  ux_host_stack_class_instance_get(host_class, class_index, class_instance);
#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
        tx_thread_relinquish();
        t = ux_utility_time_get();
        elapsed = ux_utility_time_elapsed(t0, t);
        if (elapsed >= UX_MS_TO_TICK_NON_ZERO(timeout_ms))
            return(UX_ERROR);
#else
        tx_thread_sleep(MS_TO_TICK(10));
        timeout_ms -= 10;
#endif

    } while (status != UX_SUCCESS && timeout_ms);

    if (timeout_ms == 0)
        return UX_ERROR;

    return UX_SUCCESS;
}

UINT ux_test_wait_for_value_uint(UINT *current_value_ptr, UINT desired_value)
{

UINT timeout_ms = UX_TEST_TIMEOUT_MS;
#if defined(UX_HOST_STANDALONE)
ULONG t0, t, elapsed;
    t0 = ux_utility_time_get();
#endif

    while (*current_value_ptr != desired_value && timeout_ms)
    {

#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
        tx_thread_relinquish();
        t = ux_utility_time_get();
        elapsed = ux_utility_time_elapsed(t0, t);
        if (elapsed >= UX_MS_TO_TICK_NON_ZERO(timeout_ms))
            return(UX_ERROR);
#else
        tx_thread_sleep(MS_TO_TICK(10));
        timeout_ms -= 10;
#endif
    }

    if (timeout_ms == 0)
        return UX_ERROR;

    return UX_SUCCESS;
}

UINT ux_test_wait_for_value_ulong(ULONG *current_value_ptr, ULONG desired_value)
{

UINT timeout_ms = UX_TEST_TIMEOUT_MS;
#if defined(UX_HOST_STANDALONE)
ULONG t0, t, elapsed;
    t0 = ux_utility_time_get();
#endif

    while (*current_value_ptr != desired_value && timeout_ms)
    {

#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
        tx_thread_relinquish();
        t = ux_utility_time_get();
        elapsed = ux_utility_time_elapsed(t0, t);
        if (elapsed >= UX_MS_TO_TICK_NON_ZERO(timeout_ms))
            return(UX_ERROR);
#else
        tx_thread_sleep(MS_TO_TICK(10));
        timeout_ms -= 10;
#endif
    }

    if (timeout_ms == 0)
        return UX_ERROR;

    return UX_SUCCESS;
}

UINT ux_test_wait_for_value_uchar(UCHAR *current_value_ptr, UCHAR desired_value)
{

UINT timeout_ms = UX_TEST_TIMEOUT_MS;
#if defined(UX_HOST_STANDALONE)
ULONG t0, t, elapsed;
    t0 = ux_utility_time_get();
#endif

    while (*current_value_ptr != desired_value && timeout_ms)
    {

#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
        tx_thread_relinquish();
        t = ux_utility_time_get();
        elapsed = ux_utility_time_elapsed(t0, t);
        if (elapsed >= UX_MS_TO_TICK_NON_ZERO(timeout_ms))
            return(UX_ERROR);
#else
        tx_thread_sleep(MS_TO_TICK(10));
        timeout_ms -= 10;
#endif
    }

    if (timeout_ms == 0)
        return UX_ERROR;

    return UX_SUCCESS;
}

UINT ux_test_wait_for_non_null(VOID **current_value_ptr)
{

UINT timeout_ms = UX_TEST_TIMEOUT_MS;
#if defined(UX_HOST_STANDALONE)
ULONG t0, t, elapsed;
    t0 = ux_utility_time_get();
#endif

    while (*current_value_ptr == UX_NULL && timeout_ms)
    {

#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
        tx_thread_relinquish();
        t = ux_utility_time_get();
        elapsed = ux_utility_time_elapsed(t0, t);
        if (elapsed >= UX_MS_TO_TICK_NON_ZERO(timeout_ms))
            return(UX_ERROR);
#else
        tx_thread_sleep(MS_TO_TICK(10));
        timeout_ms -= 10;
#endif
    }

    if (timeout_ms == 0)
        return UX_ERROR;

    return UX_SUCCESS;
}

UINT ux_test_wait_for_null_wait_time(VOID **current_value_ptr, UINT wait_time_ms)
{

UINT timeout_ms = wait_time_ms;
#if defined(UX_HOST_STANDALONE)
ULONG t0, t, elapsed;
    t0 = ux_utility_time_get();
#endif

    while (*current_value_ptr != UX_NULL && timeout_ms)
    {

#if defined(UX_HOST_STANDALONE)
        ux_system_tasks_run();
        tx_thread_relinquish();
        t = ux_utility_time_get();
        elapsed = ux_utility_time_elapsed(t0, t);
        if (elapsed >= UX_MS_TO_TICK_NON_ZERO(timeout_ms))
            return(UX_ERROR);
#else
        tx_thread_sleep(MS_TO_TICK(10));
        timeout_ms -= 10;
#endif
    }

    if (*current_value_ptr == UX_NULL)
        return(UX_SUCCESS);

        return UX_ERROR;
}

UINT ux_test_wait_for_null(VOID **current_value_ptr)
{
    return ux_test_wait_for_null_wait_time(current_value_ptr, UX_TEST_TIMEOUT_MS);
}

char *ux_test_file_base_name(char *path, int n)
{
    char *ptr = path, *slash = path;
    int i;
    for (i = 0; i < n; i ++)
    {
        if (*ptr == 0)
            break;
        if (*ptr == '\\' || *ptr == '/')
            slash = ptr + 1;
        ptr ++;
    }
    return(slash);
}

void ux_test_assert_hit_hint(UCHAR on_off)
{
    ux_test_assert_hit_hint_off = !on_off;
}
void ux_test_assert_hit_exit(UCHAR on_off)
{
    ux_test_assert_hit_exit_off = !on_off;
}
ULONG ux_test_assert_hit_count_get(void)
{
    return ux_test_assert_hit_count;
}
void ux_test_assert_hit_count_reset(void)
{
    ux_test_assert_hit_count = 0;
}
void ux_test_assert_hit(char* file, INT line)
{
    ux_test_assert_hit_count ++;
    if (!ux_test_assert_hit_hint_off)
        printf("%s:%d Assert HIT!\n", file, line);
    if (!ux_test_assert_hit_exit_off)
        test_control_return(ux_test_assert_hit_exit_code);
}

UINT ux_test_host_endpoint_write(UX_ENDPOINT *endpoint, UCHAR *buffer, ULONG length, ULONG *actual_length)
{
UINT            status;
UX_TRANSFER     *transfer = &endpoint->ux_endpoint_transfer_request;
    transfer->ux_transfer_request_data_pointer = buffer;
    transfer->ux_transfer_request_requested_length = length;
    status = ux_host_stack_transfer_request(transfer);
#if !defined(UX_HOST_STANDALONE)
    if (status == UX_SUCCESS)
    {
        status = tx_semaphore_get(&transfer->ux_transfer_request_semaphore,
                                  transfer->ux_transfer_request_timeout_value);
        if (status != TX_SUCCESS)
        {
            ux_host_stack_transfer_request_abort(transfer);
            status = UX_TRANSFER_TIMEOUT;
        }
    }
#endif
    if (actual_length)
        *actual_length = transfer->ux_transfer_request_actual_length;
    return(status);
}
