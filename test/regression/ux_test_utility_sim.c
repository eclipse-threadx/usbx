/* This test simulator is designed to simulate ux_utility_ APIs for test.  */

#include <stdio.h>

#define TX_SOURCE_CODE
#include "tx_api.h"
#include "tx_thread.h"
#include "tx_trace.h"
#include "tx_mutex.h"
#include "tx_semaphore.h"
#include "tx_event_flags.h"
#include "tx_initialize.h"


#define NX_SOURCE_CODE
#include "nx_api.h"
#include "nx_packet.h"


#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_dcd_sim_slave.h"
#include "ux_device_stack.h"
#include "ux_test_utility_sim.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_dcd_sim_slave.h"
#include "ux_host_class_storage.h"

#include "ux_test.h"

#define FAIL_DISABLE ((ULONG)~0x00)
#define SYSTEM_MUTEX_ALLOC_LOG_SIZE 1024

typedef struct UX_TEST_UTILITY_SIM_SEM_EXCEPT_STRUCT
{

    struct UX_TEST_UTILITY_SIM_SEM_EXCEPT_STRUCT *next;

    TX_SEMAPHORE *semaphore; /* UX_NULL to match any */
    ULONG semaphore_signal;
} UX_TEST_UTILITY_SIM_SEM_EXCEPT;

typedef struct UX_TEST_UTILITY_SIM_MEMD_STRUCT
{

    struct UX_TEST_UTILITY_SIM_MEMD_STRUCT *next;
    VOID                                   *mem;
} UX_TEST_UTILITY_SIM_MEMD;

typedef struct UX_TEST_UTILITY_SYSTEM_MUTEX_ALLOC_LOG_STRUCT
{

    ULONG first_count;
    ULONG last_count;
} UX_TEST_UTILITY_SYSTEM_MUTEX_ALLOC_LOG;

static ULONG sem_create_count = 0;
static ULONG sem_create_fail_after = FAIL_DISABLE;

static ULONG sem_get_count = 0;
static ULONG sem_get_fail_after = FAIL_DISABLE;

static UX_TEST_UTILITY_SIM_SEM_EXCEPT *excepts = UX_NULL;

static ULONG mutex_create_count = 0;
static ULONG mutex_fail_after = FAIL_DISABLE;

static ULONG mutex_on_count = 0;
static ULONG mutex_on_fail_after = FAIL_DISABLE;

static ULONG event_create_count = 0;
static ULONG event_fail_after = FAIL_DISABLE;

static void ux_system_mutex_create_callback(UX_TEST_ACTION *action, VOID *params);
static void ux_system_mutex_get_callback(UX_TEST_ACTION *action, VOID *params);
static void ux_system_mutex_put_callback(UX_TEST_ACTION *action, VOID *params);

/* Create - 0, get - 1, put - 2 */
static UX_TEST_ACTION ux_system_mutex_hooks[4] = {
    {
        .usbx_function = UX_TEST_OVERRIDE_TX_MUTEX_CREATE,
        .name_ptr = "ux_system_mutex",
        .mutex_ptr = UX_NULL, /* Don't care. */
        .inherit = TX_NO_INHERIT,
        .do_after = UX_TRUE,
        .action_func = ux_system_mutex_create_callback,
    },
    {
        .usbx_function = UX_TEST_OVERRIDE_TX_MUTEX_GET,
        .mutex_ptr = UX_NULL, /* Replaced on creation callback. */
        .wait_option = TX_WAIT_FOREVER,
        .do_after = UX_FALSE,
        .action_func = ux_system_mutex_get_callback,
    },
    {
        .usbx_function = UX_TEST_OVERRIDE_TX_MUTEX_PUT,
        .mutex_ptr = UX_NULL, /* Replaced on creation callback. */
        .do_after = UX_TRUE,
        .action_func = ux_system_mutex_put_callback,
    },
{ 0 },
};

static ULONG ux_system_mutex_on_count      = 0;
static ULONG ux_system_mutex_off_count     = 0;
UCHAR ux_system_mutex_callback_skip = UX_FALSE;
static ULONG rmem_free = 0;
static ULONG cmem_free = 0;
static UX_TEST_UTILITY_SYSTEM_MUTEX_ALLOC_LOG ux_system_mutex_alloc_logs[SYSTEM_MUTEX_ALLOC_LOG_SIZE];
static ULONG ux_system_mutex_alloc_logs_count = 0;
static ULONG ux_system_mutex_alloc_logs_match = 0;
static UCHAR ux_system_mutex_alloc_logs_area  = UX_FALSE;
static UCHAR ux_system_mutex_alloc_logs_lock  = UX_FALSE;

static ULONG thread_create_count = 0;
static ULONG thread_create_fail_after = FAIL_DISABLE;

static UX_TEST_UTILITY_SIM_MEMD  first_allocate[2];
static UX_TEST_UTILITY_SIM_MEMD *sim_allocates [2] = {UX_NULL, UX_NULL};
static VOID                     *last_allocate [2] = {UX_NULL, UX_NULL};

static ULONG mem_alloc_count = 0;
static ULONG mem_alloc_fail_after = FAIL_DISABLE;
ULONG mem_alloc_do_fail = UX_FALSE;

VOID ux_test_utility_sim_cleanup(VOID)
{

    sem_create_count = 0;
    sem_create_fail_after = FAIL_DISABLE;

    sem_get_count = 0;
    sem_get_fail_after = FAIL_DISABLE;

    excepts = UX_NULL;

    mutex_create_count = 0;
    mutex_fail_after = FAIL_DISABLE;

    mutex_on_count = 0;
    mutex_on_fail_after = FAIL_DISABLE;

    event_create_count = 0;
    event_fail_after = FAIL_DISABLE;

    thread_create_count = 0;
    thread_create_fail_after = FAIL_DISABLE;

    sim_allocates[0] = UX_NULL;
    sim_allocates[1] = UX_NULL;
    last_allocate[0] = UX_NULL;
    last_allocate[1] = UX_NULL;

    ux_test_remove_hooks_from_array(ux_system_mutex_hooks);

    mem_alloc_count = 0;
    mem_alloc_fail_after = FAIL_DISABLE;
    mem_alloc_do_fail = UX_FALSE;

    ux_system_mutex_on_count  = 0;
    ux_system_mutex_off_count = 0;
    ux_system_mutex_callback_skip      = UX_FALSE;
    ux_system_mutex_alloc_logs_count = 0;
    ux_system_mutex_alloc_logs_match = 0;
    ux_system_mutex_alloc_logs_area  = UX_FALSE;
    ux_system_mutex_alloc_logs_lock  = UX_FALSE;
    _ux_utility_memory_set(ux_system_mutex_alloc_logs, 0, sizeof(ux_system_mutex_alloc_logs));
}

/* Semaphore handling simulation */

VOID ux_test_utility_sim_sem_create_count_reset(VOID)
{

    sem_create_count = 0;
}

ULONG ux_test_utility_sim_sem_create_count(VOID)
{

    return sem_create_count;
}

VOID ux_test_utility_sim_sem_error_generation_start(ULONG fail_after)
{

    sem_create_count = 0;
    sem_create_fail_after = fail_after;
}

VOID ux_test_utility_sim_sem_error_generation_stop(VOID)
{

    sem_create_fail_after = FAIL_DISABLE;
    sem_create_count = 0;
}

UINT  _tx_semaphore_create(TX_SEMAPHORE *semaphore_ptr, CHAR *name_ptr, ULONG initial_count)
{

TX_INTERRUPT_SAVE_AREA

TX_SEMAPHORE    *next_semaphore;
TX_SEMAPHORE    *previous_semaphore;
UX_TEST_OVERRIDE_TX_SEMAPHORE_CREATE_PARAMS     action_params = { semaphore_ptr, name_ptr, initial_count };
UX_TEST_ACTION                                  action;


    if (sem_create_fail_after != FAIL_DISABLE)
    {

        if (sem_create_count >= sem_create_fail_after)
        {

            /* Return testing error instead of actual creation. */
            return UX_MUTEX_ERROR;
        }
    }

    /* Perform action.  */
    action = ux_test_action_handler(UX_TEST_OVERRIDE_TX_SEMAPHORE_CREATE, &action_params);
    ux_test_do_action_before(&action, &action_params);
    if (ux_test_is_expedient_on())
    {
        if (action.matched && !action.do_after)
        {
            if (!action.no_return)
            {
                return action.status;
            }
        }
    }

    /* Do actual creating. */
    sem_create_count ++;

    /* Initialize semaphore control block to all zeros.  */
    TX_MEMSET(semaphore_ptr, 0, (sizeof(TX_SEMAPHORE)));

    /* Setup the basic semaphore fields.  */
    semaphore_ptr -> tx_semaphore_name =             name_ptr;
    semaphore_ptr -> tx_semaphore_count =            initial_count;
    
    /* Disable interrupts to place the semaphore on the created list.  */
    TX_DISABLE

    /* Setup the semaphore ID to make it valid.  */
    semaphore_ptr -> tx_semaphore_id =  TX_SEMAPHORE_ID;

    /* Place the semaphore on the list of created semaphores.  First,
       check for an empty list.  */
    if (_tx_semaphore_created_count == TX_EMPTY)
    {

        /* The created semaphore list is empty.  Add semaphore to empty list.  */
        _tx_semaphore_created_ptr =                       semaphore_ptr;
        semaphore_ptr -> tx_semaphore_created_next =      semaphore_ptr;
        semaphore_ptr -> tx_semaphore_created_previous =  semaphore_ptr;
    }
    else
    {

        /* This list is not NULL, add to the end of the list.  */
        next_semaphore =      _tx_semaphore_created_ptr;
        previous_semaphore =  next_semaphore -> tx_semaphore_created_previous;

        /* Place the new semaphore in the list.  */
        next_semaphore -> tx_semaphore_created_previous =  semaphore_ptr;
        previous_semaphore -> tx_semaphore_created_next =  semaphore_ptr;

        /* Setup this semaphore's next and previous created links.  */
        semaphore_ptr -> tx_semaphore_created_previous =  previous_semaphore;
        semaphore_ptr -> tx_semaphore_created_next =      next_semaphore;    
    }
    
    /* Increment the created count.  */
    _tx_semaphore_created_count++;

    /* Optional semaphore create extended processing.  */
    TX_SEMAPHORE_CREATE_EXTENSION(semaphore_ptr)

    /* If trace is enabled, register this object.  */
    TX_TRACE_OBJECT_REGISTER(TX_TRACE_OBJECT_TYPE_SEMAPHORE, semaphore_ptr, name_ptr, initial_count, 0)

    /* If trace is enabled, insert this event into the trace buffer.  */
    TX_TRACE_IN_LINE_INSERT(TX_TRACE_SEMAPHORE_CREATE, semaphore_ptr, initial_count, TX_POINTER_TO_ULONG_CONVERT(&next_semaphore), 0, TX_TRACE_SEMAPHORE_EVENTS)

    /* Log this kernel call.  */
    TX_EL_SEMAPHORE_CREATE_INSERT

    /* Restore interrupts.  */
    TX_RESTORE

    /* Return TX_SUCCESS.  */
    return(TX_SUCCESS);
}

VOID  ux_test_utility_sim_sem_get_count_reset    (VOID)
{

    sem_get_count = 0;
}

ULONG ux_test_utility_sim_sem_get_count          (VOID)
{

    return sem_get_count;
}

VOID  ux_test_utility_sim_sem_get_error_generation_start(ULONG fail_after)
{

    sem_get_count = 0;
    sem_get_fail_after = fail_after;
}

VOID  ux_test_utility_sim_sem_get_error_generation_stop (VOID)
{

    sem_get_fail_after = FAIL_DISABLE;
    sem_get_count = 0;
}

VOID  ux_test_utility_sim_sem_get_error_exception_reset(VOID)
{

    excepts = UX_NULL;
}

VOID  ux_test_utility_sim_sem_get_error_exception_add(TX_SEMAPHORE *semaphore, ULONG semaphore_signal)
{
UX_TEST_UTILITY_SIM_SEM_EXCEPT* except;

    if (_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start == UX_NULL)
        return;

    except = (UX_TEST_UTILITY_SIM_SEM_EXCEPT *)ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_TEST_UTILITY_SIM_SEM_EXCEPT));
    if (except == UX_NULL)
        return;

    /* Save exception */
    except->semaphore = semaphore;
    except->semaphore_signal = semaphore_signal;

    /* Link to head */
    except->next = excepts;
    excepts = except;
}

static UCHAR ux_test_utility_sim_sem_in_exception_list(TX_SEMAPHORE *semaphore, ULONG semaphore_signal)
{
UX_TEST_UTILITY_SIM_SEM_EXCEPT* except;

    except = excepts;
    while(except)
    {

        if (except->semaphore == UX_NULL && semaphore_signal == except->semaphore_signal)
            return UX_TRUE;

        if (except->semaphore == semaphore && semaphore_signal == except->semaphore_signal)
            return UX_TRUE;

        except = except->next;
    }
    return UX_FALSE;
}

UINT  _tx_semaphore_get(TX_SEMAPHORE *semaphore_ptr, ULONG wait_option)
{

TX_INTERRUPT_SAVE_AREA
            
TX_THREAD       *thread_ptr;            
TX_THREAD       *next_thread;
TX_THREAD       *previous_thread;
UINT            status;
UX_TEST_OVERRIDE_TX_SEMAPHORE_GET_PARAMS    params = { semaphore_ptr, wait_option };
UX_TEST_ACTION                              action;


    /* Perform hooked callbacks.  */
    ux_test_do_hooks_before(UX_TEST_OVERRIDE_TX_SEMAPHORE_GET, &params);

    action = ux_test_action_handler(UX_TEST_OVERRIDE_TX_SEMAPHORE_GET, &params);
    ux_test_do_action_before(&action, &params);

    if (ux_test_is_expedient_on())
    {

        if (sem_get_fail_after != FAIL_DISABLE)

            if (sem_get_count >= sem_get_fail_after)

                /* Return testing error instead of actual creation. */
                if (!ux_test_utility_sim_sem_in_exception_list(semaphore_ptr, wait_option))

                    return UX_SEMAPHORE_ERROR;
    }

    /* Default the status to TX_SUCCESS.  */
    status =  TX_SUCCESS;

    /* Disable interrupts to get an instance from the semaphore.  */
    TX_DISABLE

#ifdef TX_SEMAPHORE_ENABLE_PERFORMANCE_INFO

    /* Increment the total semaphore get counter.  */
    _tx_semaphore_performance_get_count++;

    /* Increment the number of attempts to get this semaphore.  */
    semaphore_ptr -> tx_semaphore_performance_get_count++;
#endif

    /* If trace is enabled, insert this event into the trace buffer.  */
    TX_TRACE_IN_LINE_INSERT(TX_TRACE_SEMAPHORE_GET, semaphore_ptr, wait_option, semaphore_ptr -> tx_semaphore_count, TX_POINTER_TO_ULONG_CONVERT(&thread_ptr), TX_TRACE_SEMAPHORE_EVENTS)

    /* Log this kernel call.  */
    TX_EL_SEMAPHORE_GET_INSERT

    /* Determine if there is an instance of the semaphore.  */
    if (semaphore_ptr -> tx_semaphore_count != ((ULONG) 0))
    {

        /* Decrement the semaphore count.  */
        semaphore_ptr -> tx_semaphore_count--;

        /* Restore interrupts.  */
        TX_RESTORE
    }

    /* Determine if the request specifies suspension.  */
    else if (wait_option != TX_NO_WAIT)
    {

        /* Prepare for suspension of this thread.  */

#ifdef TX_SEMAPHORE_ENABLE_PERFORMANCE_INFO

        /* Increment the total semaphore suspensions counter.  */
        _tx_semaphore_performance_suspension_count++;

        /* Increment the number of suspensions on this semaphore.  */
        semaphore_ptr -> tx_semaphore_performance_suspension_count++;
#endif
            
        /* Pickup thread pointer.  */
        TX_THREAD_GET_CURRENT(thread_ptr)

        /* Setup cleanup routine pointer.  */
        thread_ptr -> tx_thread_suspend_cleanup =  &(_tx_semaphore_cleanup);

        /* Setup cleanup information, i.e. this semaphore control
           block.  */
        thread_ptr -> tx_thread_suspend_control_block =  (VOID *) semaphore_ptr;

        /* Setup suspension list.  */
        if (semaphore_ptr -> tx_semaphore_suspended_count == TX_NO_SUSPENSIONS)
        {

            /* No other threads are suspended.  Setup the head pointer and
               just setup this threads pointers to itself.  */
            semaphore_ptr -> tx_semaphore_suspension_list =         thread_ptr;
            thread_ptr -> tx_thread_suspended_next =                thread_ptr;
            thread_ptr -> tx_thread_suspended_previous =            thread_ptr;
        }
        else
        {

            /* This list is not NULL, add current thread to the end. */
            next_thread =                                   semaphore_ptr -> tx_semaphore_suspension_list;
            thread_ptr -> tx_thread_suspended_next =        next_thread;
            previous_thread =                               next_thread -> tx_thread_suspended_previous;
            thread_ptr -> tx_thread_suspended_previous =    previous_thread;
            previous_thread -> tx_thread_suspended_next =   thread_ptr;
            next_thread -> tx_thread_suspended_previous =   thread_ptr;
        }

        /* Increment the number of suspensions.  */
        semaphore_ptr -> tx_semaphore_suspended_count++;

        /* Set the state to suspended.  */
        thread_ptr -> tx_thread_state =    TX_SEMAPHORE_SUSP;

#ifdef TX_NOT_INTERRUPTABLE

        /* Call actual non-interruptable thread suspension routine.  */
        _tx_thread_system_ni_suspend(thread_ptr, wait_option);

        /* Restore interrupts.  */
        TX_RESTORE
#else

        /* Set the suspending flag.  */
        thread_ptr -> tx_thread_suspending =  TX_TRUE;

        /* Setup the timeout period.  */
        thread_ptr -> tx_thread_timer.tx_timer_internal_remaining_ticks =  wait_option;

        /* Temporarily disable preemption.  */
        _tx_thread_preempt_disable++;

        /* Restore interrupts.  */
        TX_RESTORE

        /* Call actual thread suspension routine.  */
        _tx_thread_system_suspend(thread_ptr);
#endif

        /* Return the completion status.  */
        status =  thread_ptr -> tx_thread_suspend_status;
    }
    else
    {

        /* Restore interrupts.  */
        TX_RESTORE

        /* Immediate return, return error completion.  */
        status =  TX_NO_INSTANCE;
    }

    ux_test_do_action_after(&action, &params);

    /* Perform hooked callbacks.  */
    ux_test_do_hooks_after(UX_TEST_OVERRIDE_TX_SEMAPHORE_GET, &params);

    /* Return completion status.  */
    return(status);
}

/* Mutex handling simulation */

VOID ux_test_utility_sim_mutex_create_count_reset(VOID)
{

    mutex_create_count = 0;
}

ULONG ux_test_utility_sim_mutex_create_count(VOID)
{

    return mutex_create_count;
}

VOID ux_test_utility_sim_mutex_error_generation_start(ULONG fail_after)
{

    mutex_create_count = 0;
    mutex_fail_after = fail_after;
}

VOID ux_test_utility_sim_mutex_error_generation_stop(VOID)
{

    mutex_fail_after = FAIL_DISABLE;
    mutex_create_count = 0;
}

UINT  _tx_mutex_create(TX_MUTEX *mutex_ptr, CHAR *name_ptr, UINT inherit)
{

TX_INTERRUPT_SAVE_AREA

TX_MUTEX        *next_mutex;
TX_MUTEX        *previous_mutex;

UX_TEST_OVERRIDE_TX_MUTEX_CREATE_PARAMS action_params = { mutex_ptr, name_ptr, inherit };
UX_TEST_ACTION                          action;

    /* Perform hooked callbacks.  */
    ux_test_do_hooks_before(UX_TEST_OVERRIDE_TX_MUTEX_CREATE, &action_params);

    /* Perform action.  */
    action = ux_test_action_handler(UX_TEST_OVERRIDE_TX_MUTEX_CREATE, &action_params);
    ux_test_do_action_before(&action, &action_params);

    if (mutex_fail_after != FAIL_DISABLE)
    {

        if (mutex_create_count >= mutex_fail_after)
        {

            /* Return testing error instead of actual creation. */
            return UX_MUTEX_ERROR;
        }
    }

    /* Do actual creating. */
    mutex_create_count ++;

    /* Initialize mutex control block to all zeros.  */
    TX_MEMSET(mutex_ptr, 0, (sizeof(TX_MUTEX)));

    /* Setup the basic mutex fields.  */
    mutex_ptr -> tx_mutex_name =             name_ptr;
    mutex_ptr -> tx_mutex_inherit =          inherit;
    
    /* Disable interrupts to place the mutex on the created list.  */
    TX_DISABLE

    /* Setup the mutex ID to make it valid.  */
    mutex_ptr -> tx_mutex_id =  TX_MUTEX_ID;

    /* Setup the thread mutex release function pointer.  */
    _tx_thread_mutex_release =  &(_tx_mutex_thread_release);

    /* Place the mutex on the list of created mutexes.  First,
       check for an empty list.  */
    if (_tx_mutex_created_count == TX_EMPTY)
    {

        /* The created mutex list is empty.  Add mutex to empty list.  */
        _tx_mutex_created_ptr =                   mutex_ptr;
        mutex_ptr -> tx_mutex_created_next =      mutex_ptr;
        mutex_ptr -> tx_mutex_created_previous =  mutex_ptr;
    }
    else
    {

        /* This list is not NULL, add to the end of the list.  */
        next_mutex =      _tx_mutex_created_ptr;
        previous_mutex =  next_mutex -> tx_mutex_created_previous;

        /* Place the new mutex in the list.  */
        next_mutex -> tx_mutex_created_previous =  mutex_ptr;
        previous_mutex -> tx_mutex_created_next =  mutex_ptr;

        /* Setup this mutex's next and previous created links.  */
        mutex_ptr -> tx_mutex_created_previous =  previous_mutex;
        mutex_ptr -> tx_mutex_created_next =      next_mutex;    
    }

    /* Increment the ownership count.  */
    _tx_mutex_created_count++;
    
    /* Optional mutex create extended processing.  */
    TX_MUTEX_CREATE_EXTENSION(mutex_ptr)

    /* If trace is enabled, register this object.  */
    TX_TRACE_OBJECT_REGISTER(TX_TRACE_OBJECT_TYPE_MUTEX, mutex_ptr, name_ptr, inherit, 0)

    /* If trace is enabled, insert this event into the trace buffer.  */
    TX_TRACE_IN_LINE_INSERT(TX_TRACE_MUTEX_CREATE, mutex_ptr, inherit, TX_POINTER_TO_ULONG_CONVERT(&next_mutex), 0, TX_TRACE_MUTEX_EVENTS)

    /* Log this kernel call.  */
    TX_EL_MUTEX_CREATE_INSERT

    /* Restore interrupts.  */
    TX_RESTORE

    ux_test_do_action_after(&action, &action_params);

    /* Perform hooked callbacks.  */
    ux_test_do_hooks_after(UX_TEST_OVERRIDE_TX_MUTEX_CREATE, &action_params);

    /* Return TX_SUCCESS.  */
    return(TX_SUCCESS);
}

VOID  ux_test_utility_sim_mutex_on_count_reset    (VOID)
{

    mutex_on_count = 0;
}
ULONG ux_test_utility_sim_mutex_on_count          (VOID)
{

    return mutex_on_count;
}

VOID  ux_test_utility_sim_mutex_on_error_generation_start(ULONG fail_after)
{

    mutex_on_count = 0;
    mutex_on_fail_after = fail_after;
}
VOID  ux_test_utility_sim_mutex_on_error_generation_stop (VOID)
{

    mutex_on_fail_after = FAIL_DISABLE;
    mutex_on_count = 0;
}

/* Thread handling simulation */

VOID ux_test_utility_sim_thread_create_count_reset(VOID)
{

    thread_create_count = 0;
}

ULONG ux_test_utility_sim_thread_create_count(VOID)
{

    return thread_create_count;
}

VOID ux_test_utility_sim_thread_error_generation_start(ULONG fail_after)
{

    thread_create_count = 0;
    thread_create_fail_after = fail_after;
}

VOID ux_test_utility_sim_thread_error_generation_stop(VOID)
{

    thread_create_fail_after = FAIL_DISABLE;
    thread_create_count = 0;
}

UINT  _tx_thread_create(TX_THREAD *thread_ptr, CHAR *name_ptr, VOID (*entry_function)(ULONG id), ULONG entry_input,
                            VOID *stack_start, ULONG stack_size, UINT priority, UINT preempt_threshold,
                            ULONG time_slice, UINT auto_start)
{

TX_INTERRUPT_SAVE_AREA

TX_THREAD               *next_thread;
TX_THREAD               *previous_thread;
TX_THREAD               *saved_thread_ptr;
UINT                    saved_threshold =  ((UINT) 0);
UCHAR                   *temp_ptr;
UX_TEST_OVERRIDE_TX_THREAD_CREATE_PARAMS        action_params = { name_ptr };
UX_TEST_ACTION                                  action;

#ifdef TX_ENABLE_STACK_CHECKING
ULONG                   new_stack_start;
ULONG                   updated_stack_start;
#endif


    if (thread_create_fail_after != FAIL_DISABLE)
    {

        if (thread_create_count >= thread_create_fail_after)
        {

            /* Return testing error instead of actual creation. */
            return UX_MUTEX_ERROR;
        }
    }

    /* Perform action.  */
    action = ux_test_action_handler(UX_TEST_OVERRIDE_TX_THREAD_CREATE, &action_params);
    ux_test_do_action_before(&action, &action_params);
    if (ux_test_is_expedient_on())
    {
        if (action.matched && !action.do_after)
        {
            if (!action.no_return)
            {
                return action.status;
            }
        }
    }

    /* Do actual creating. */
    thread_create_count ++;

#ifndef TX_DISABLE_STACK_FILLING

    /* Set the thread stack to a pattern prior to creating the initial
       stack frame.  This pattern is used by the stack checking routines
       to see how much has been used.  */
    TX_MEMSET(stack_start, ((UCHAR) TX_STACK_FILL), stack_size);
#endif

#ifdef TX_ENABLE_STACK_CHECKING

    /* Ensure that there are two ULONG of 0xEF patterns at the top and 
       bottom of the thread's stack. This will be used to check for stack
       overflow conditions during run-time.  */
    stack_size =  ((stack_size/(sizeof(ULONG))) * (sizeof(ULONG))) - (sizeof(ULONG));

    /* Ensure the starting stack address is evenly aligned.  */
    new_stack_start =  TX_POINTER_TO_ULONG_CONVERT(stack_start);
    updated_stack_start =  ((((ULONG) new_stack_start) + ((sizeof(ULONG)) - ((ULONG) 1)) ) & (~((sizeof(ULONG)) - ((ULONG) 1))));

    /* Determine if the starting stack address is different.  */
    if (new_stack_start != updated_stack_start)
    {
    
        /* Yes, subtract another ULONG from the size to avoid going past the stack area.  */
        stack_size =  stack_size - (sizeof(ULONG));
    }

    /* Update the starting stack pointer.  */
    stack_start =  TX_ULONG_TO_POINTER_CONVERT(updated_stack_start);
#endif

    /* Prepare the thread control block prior to placing it on the created
       list.  */

    /* Initialize thread control block to all zeros.  */
    TX_MEMSET(thread_ptr, 0, (sizeof(TX_THREAD)));

    /* Place the supplied parameters into the thread's control block.  */
    thread_ptr -> tx_thread_name =              name_ptr;
    thread_ptr -> tx_thread_entry =             entry_function;
    thread_ptr -> tx_thread_entry_parameter =   entry_input;
    thread_ptr -> tx_thread_stack_start =       stack_start;
    thread_ptr -> tx_thread_stack_size =        stack_size;
    thread_ptr -> tx_thread_stack_end =         (VOID *) (TX_UCHAR_POINTER_ADD(stack_start, (stack_size - ((ULONG) 1))));
    thread_ptr -> tx_thread_priority =          priority;
    thread_ptr -> tx_thread_user_priority =     priority;
    thread_ptr -> tx_thread_time_slice =        time_slice;
    thread_ptr -> tx_thread_new_time_slice =    time_slice;
    thread_ptr -> tx_thread_inherit_priority =  ((UINT) TX_MAX_PRIORITIES);

    /* Calculate the end of the thread's stack area.  */
    temp_ptr =  TX_VOID_TO_UCHAR_POINTER_CONVERT(stack_start);
    temp_ptr =  (TX_UCHAR_POINTER_ADD(temp_ptr, (stack_size - ((ULONG) 1))));
    thread_ptr -> tx_thread_stack_end =         TX_UCHAR_TO_VOID_POINTER_CONVERT(temp_ptr);

#ifndef TX_DISABLE_PREEMPTION_THRESHOLD

    /* Preemption-threshold is enabled, setup accordingly.  */
    thread_ptr -> tx_thread_preempt_threshold =       preempt_threshold;
    thread_ptr -> tx_thread_user_preempt_threshold =  preempt_threshold;
#else

    /* Preemption-threshold is disabled, determine if preemption-threshold was required.  */
    if (priority != preempt_threshold)
    {

        /* Preemption-threshold specified. Since specific preemption-threshold is not supported,
           disable all preemption.  */
        thread_ptr -> tx_thread_preempt_threshold =       ((UINT) 0);
        thread_ptr -> tx_thread_user_preempt_threshold =  ((UINT) 0);
    } 
    else
    {

        /* Preemption-threshold is not specified, just setup with the priority.  */
        thread_ptr -> tx_thread_preempt_threshold =       priority;
        thread_ptr -> tx_thread_user_preempt_threshold =  priority;
    }
#endif

    /* Now fill in the values that are required for thread initialization.  */
    thread_ptr -> tx_thread_state =  TX_SUSPENDED;

    /* Setup the necessary fields in the thread timer block.  */
    TX_THREAD_CREATE_TIMEOUT_SETUP(thread_ptr)

    /* Perform any additional thread setup activities for tool or user purpose.  */
    TX_THREAD_CREATE_INTERNAL_EXTENSION(thread_ptr) 

    /* Call the target specific stack frame building routine to build the 
       thread's initial stack and to setup the actual stack pointer in the
       control block.  */
    _tx_thread_stack_build(thread_ptr, _tx_thread_shell_entry);

#ifdef TX_ENABLE_STACK_CHECKING

    /* Setup the highest usage stack pointer.  */
    thread_ptr -> tx_thread_stack_highest_ptr =  thread_ptr -> tx_thread_stack_ptr;
#endif

    /* Prepare to make this thread a member of the created thread list.  */
    TX_DISABLE

    /* Load the thread ID field in the thread control block.  */
    thread_ptr -> tx_thread_id =  TX_THREAD_ID;

    /* Place the thread on the list of created threads.  First,
       check for an empty list.  */
    if (_tx_thread_created_count == TX_EMPTY)
    {

        /* The created thread list is empty.  Add thread to empty list.  */
        _tx_thread_created_ptr =                    thread_ptr;
        thread_ptr -> tx_thread_created_next =      thread_ptr;
        thread_ptr -> tx_thread_created_previous =  thread_ptr;
    }
    else
    {

        /* This list is not NULL, add to the end of the list.  */
        next_thread =  _tx_thread_created_ptr;
        previous_thread =  next_thread -> tx_thread_created_previous;

        /* Place the new thread in the list.  */
        next_thread -> tx_thread_created_previous =  thread_ptr;
        previous_thread -> tx_thread_created_next =  thread_ptr;

        /* Setup this thread's created links.  */
        thread_ptr -> tx_thread_created_previous =  previous_thread;
        thread_ptr -> tx_thread_created_next =      next_thread;    
    }
    
    /* Increment the thread created count.  */
    _tx_thread_created_count++;

    /* If trace is enabled, register this object.  */
    TX_TRACE_OBJECT_REGISTER(TX_TRACE_OBJECT_TYPE_THREAD, thread_ptr, name_ptr, TX_POINTER_TO_ULONG_CONVERT(stack_start), stack_size)

    /* If trace is enabled, insert this event into the trace buffer.  */
    TX_TRACE_IN_LINE_INSERT(TX_TRACE_THREAD_CREATE, thread_ptr, priority, TX_POINTER_TO_ULONG_CONVERT(stack_start), stack_size, TX_TRACE_THREAD_EVENTS)

    /* Register thread in the thread array structure.  */
    TX_EL_THREAD_REGISTER(thread_ptr)

    /* Log this kernel call.  */
    TX_EL_THREAD_CREATE_INSERT

#ifndef TX_NOT_INTERRUPTABLE

    /* Temporarily disable preemption.  */
    _tx_thread_preempt_disable++;
#endif

    /* Determine if an automatic start was requested.  If so, call the resume
       thread function and then check for a preemption condition.  */
    if (auto_start == TX_AUTO_START)
    {

        /* Determine if the create call is being called from initialization.  */
        if (TX_THREAD_GET_SYSTEM_STATE() >= TX_INITIALIZE_IN_PROGRESS)
        {

            /* Yes, this create call was made from initialization.  */

            /* Pickup the current thread execute pointer, which corresponds to the
               highest priority thread ready to execute.  Interrupt lockout is 
               not required, since interrupts are assumed to be disabled during 
               initialization.  */
            saved_thread_ptr =  _tx_thread_execute_ptr;

            /* Determine if there is thread ready for execution.  */
            if (saved_thread_ptr != TX_NULL)
            {
                
                /* Yes, a thread is ready for execution when initialization completes.  */

                /* Save the current preemption-threshold.  */
                saved_threshold =  saved_thread_ptr -> tx_thread_preempt_threshold;

                /* For initialization, temporarily set the preemption-threshold to the 
                   priority level to make sure the highest-priority thread runs once 
                   initialization is complete.  */
                saved_thread_ptr -> tx_thread_preempt_threshold =  saved_thread_ptr -> tx_thread_priority;
            }
        } 
        else
        {

            /* Simply set the saved thread pointer to NULL.  */
            saved_thread_ptr =  TX_NULL;
        }

#ifdef TX_NOT_INTERRUPTABLE

        /* Perform any additional activities for tool or user purpose.  */
        TX_THREAD_CREATE_EXTENSION(thread_ptr)

        /* Resume the thread!  */
        _tx_thread_system_ni_resume(thread_ptr);

        /* Restore previous interrupt posture.  */
        TX_RESTORE
#else

        /* Restore previous interrupt posture.  */
        TX_RESTORE

        /* Perform any additional activities for tool or user purpose.  */
        TX_THREAD_CREATE_EXTENSION(thread_ptr)

        /* Call the resume thread function to make this thread ready.  */ 
        _tx_thread_system_resume(thread_ptr);
#endif
 
        /* Determine if the thread's preemption-threshold needs to be restored.  */
        if (saved_thread_ptr != TX_NULL)
        {

            /* Yes, restore the previous highest-priority thread's preemption-threshold. This
               can only happen if this routine is called from initialization.  */
            saved_thread_ptr -> tx_thread_preempt_threshold =  saved_threshold;
        } 
    }
    else
    {

#ifdef TX_NOT_INTERRUPTABLE

        /* Perform any additional activities for tool or user purpose.  */
        TX_THREAD_CREATE_EXTENSION(thread_ptr)

        /* Restore interrupts.  */
        TX_RESTORE
#else

        /* Restore interrupts.  */
        TX_RESTORE

        /* Perform any additional activities for tool or user purpose.  */
        TX_THREAD_CREATE_EXTENSION(thread_ptr)

        /* Disable interrupts.  */
        TX_DISABLE

        /* Re-enable preemption.  */
        _tx_thread_preempt_disable--;

        /* Restore interrupts.  */
        TX_RESTORE

        /* Check for preemption.  */
        _tx_thread_system_preempt_check();
#endif
    }

    /* Always return a success.  */
    return(TX_SUCCESS);
}

UINT  _tx_mutex_get(TX_MUTEX *mutex_ptr, ULONG wait_option)
{

TX_INTERRUPT_SAVE_AREA

TX_THREAD       *thread_ptr;            
TX_MUTEX        *next_mutex;
TX_MUTEX        *previous_mutex;
TX_THREAD       *mutex_owner;
TX_THREAD       *next_thread;
TX_THREAD       *previous_thread;
UINT            status;
UX_TEST_OVERRIDE_TX_MUTEX_GET_PARAMS    action_params = { mutex_ptr, wait_option };
UX_TEST_ACTION                          action;

    /* Perform hooked callbacks.  */
    ux_test_do_hooks_before(UX_TEST_OVERRIDE_TX_MUTEX_GET, &action_params);

    /* Perform action.  */
    action = ux_test_action_handler(UX_TEST_OVERRIDE_TX_MUTEX_GET, &action_params);
    ux_test_do_action_before(&action, &action_params);

    /* Disable interrupts to get an instance from the mutex.  */
    TX_DISABLE

#ifdef TX_MUTEX_ENABLE_PERFORMANCE_INFO

    /* Increment the total mutex get counter.  */
    _tx_mutex_performance_get_count++;

    /* Increment the number of attempts to get this mutex.  */
    mutex_ptr -> tx_mutex_performance_get_count++;
#endif

    /* If trace is enabled, insert this event into the trace buffer.  */
    TX_TRACE_IN_LINE_INSERT(TX_TRACE_MUTEX_GET, mutex_ptr, wait_option, TX_POINTER_TO_ULONG_CONVERT(mutex_ptr -> tx_mutex_owner), mutex_ptr -> tx_mutex_ownership_count, TX_TRACE_MUTEX_EVENTS)

    /* Log this kernel call.  */
    TX_EL_MUTEX_GET_INSERT

    /* Pickup thread pointer.  */
    TX_THREAD_GET_CURRENT(thread_ptr)

    /* Determine if this mutex is available.  */
    if (mutex_ptr -> tx_mutex_ownership_count == ((UINT) 0))
    {

        /* Set the ownership count to 1.  */
        mutex_ptr -> tx_mutex_ownership_count =  ((UINT) 1);

        /* Remember that the calling thread owns the mutex.  */
        mutex_ptr -> tx_mutex_owner =  thread_ptr;

        /* Determine if the thread pointer is valid.  */
        if (thread_ptr != TX_NULL)
        {

            /* Determine if priority inheritance is required.  */
            if (mutex_ptr -> tx_mutex_inherit == TX_TRUE)
            {
         
                /* Remember the current priority of thread.  */
                mutex_ptr -> tx_mutex_original_priority =   thread_ptr -> tx_thread_priority;

                /* Setup the highest priority waiting thread.  */
                mutex_ptr -> tx_mutex_highest_priority_waiting =  ((UINT) TX_MAX_PRIORITIES);
            }

            /* Pickup next mutex pointer, which is the head of the list.  */
            next_mutex =  thread_ptr -> tx_thread_owned_mutex_list;

            /* Determine if this thread owns any other mutexes that have priority inheritance.  */
            if (next_mutex != TX_NULL)
            {

                /* Non-empty list. Link up the mutex.  */

                /* Pickup the next and previous mutex pointer.  */
                previous_mutex =  next_mutex -> tx_mutex_owned_previous;

                /* Place the owned mutex in the list.  */
                next_mutex -> tx_mutex_owned_previous =  mutex_ptr;
                previous_mutex -> tx_mutex_owned_next =  mutex_ptr;

                /* Setup this mutex's next and previous created links.  */
                mutex_ptr -> tx_mutex_owned_previous =  previous_mutex;
                mutex_ptr -> tx_mutex_owned_next =      next_mutex;
            }
            else
            {

                /* The owned mutex list is empty.  Add mutex to empty list.  */
                thread_ptr -> tx_thread_owned_mutex_list =     mutex_ptr;
                mutex_ptr -> tx_mutex_owned_next =             mutex_ptr;
                mutex_ptr -> tx_mutex_owned_previous =         mutex_ptr;
            }

            /* Increment the number of mutexes owned counter.  */
            thread_ptr -> tx_thread_owned_mutex_count++;
        }

        /* Restore interrupts.  */
        TX_RESTORE

        /* Return success.  */
        status =  TX_SUCCESS;
    }

    /* Otherwise, see if the owning thread is trying to obtain the same mutex.  */
    else if (mutex_ptr -> tx_mutex_owner == thread_ptr)
    {

        /* The owning thread is requesting the mutex again, just 
           increment the ownership count.  */
        mutex_ptr -> tx_mutex_ownership_count++;

        /* Restore interrupts.  */
        TX_RESTORE

        /* Return success.  */
        status =  TX_SUCCESS;
    }
    else
    {

        /* Determine if the request specifies suspension.  */
        if (wait_option != TX_NO_WAIT)
        {

            /* Prepare for suspension of this thread.  */

            /* Pickup the mutex owner.  */
            mutex_owner =  mutex_ptr -> tx_mutex_owner;

#ifdef TX_MUTEX_ENABLE_PERFORMANCE_INFO

            /* Increment the total mutex suspension counter.  */
            _tx_mutex_performance_suspension_count++;

            /* Increment the number of suspensions on this mutex.  */
            mutex_ptr -> tx_mutex_performance_suspension_count++;

            /* Determine if a priority inversion is present.  */
            if (thread_ptr -> tx_thread_priority < mutex_owner -> tx_thread_priority)
            {

                /* Yes, priority inversion is present!  */

                /* Increment the total mutex priority inversions counter.  */
                _tx_mutex_performance_priority_inversion_count++;

                /* Increment the number of priority inversions on this mutex.  */
                mutex_ptr -> tx_mutex_performance_priority_inversion_count++;

#ifdef TX_THREAD_ENABLE_PERFORMANCE_INFO

                /* Increment the number of total thread priority inversions.  */
                _tx_thread_performance_priority_inversion_count++;

                /* Increment the number of priority inversions for this thread.  */
                thread_ptr -> tx_thread_performance_priority_inversion_count++;
#endif
            }
#endif

            /* Setup cleanup routine pointer.  */
            thread_ptr -> tx_thread_suspend_cleanup =  &(_tx_mutex_cleanup);

            /* Setup cleanup information, i.e. this mutex control
               block.  */
            thread_ptr -> tx_thread_suspend_control_block =  (VOID *) mutex_ptr;

            /* Setup suspension list.  */
            if (mutex_ptr -> tx_mutex_suspended_count == TX_NO_SUSPENSIONS)
            {

                /* No other threads are suspended.  Setup the head pointer and
                   just setup this threads pointers to itself.  */
                mutex_ptr -> tx_mutex_suspension_list =         thread_ptr;
                thread_ptr -> tx_thread_suspended_next =        thread_ptr;
                thread_ptr -> tx_thread_suspended_previous =    thread_ptr;
            }
            else
            {

                /* This list is not NULL, add current thread to the end. */
                next_thread =                                   mutex_ptr -> tx_mutex_suspension_list;
                thread_ptr -> tx_thread_suspended_next =        next_thread;
                previous_thread =                               next_thread -> tx_thread_suspended_previous;
                thread_ptr -> tx_thread_suspended_previous =    previous_thread;
                previous_thread -> tx_thread_suspended_next =   thread_ptr;
                next_thread -> tx_thread_suspended_previous =   thread_ptr;
            }
            
            /* Increment the suspension count.  */
            mutex_ptr -> tx_mutex_suspended_count++;

            /* Set the state to suspended.  */
            thread_ptr -> tx_thread_state =    TX_MUTEX_SUSP;

#ifdef TX_NOT_INTERRUPTABLE

            /* Determine if we need to raise the priority of the thread 
               owning the mutex.  */
            if (mutex_ptr -> tx_mutex_inherit == TX_TRUE)
            {

                /* Determine if this is the highest priority to raise for this mutex.  */
                if (mutex_ptr -> tx_mutex_highest_priority_waiting > thread_ptr -> tx_thread_priority)
                {

                    /* Remember this priority.  */
                    mutex_ptr -> tx_mutex_highest_priority_waiting =  thread_ptr -> tx_thread_priority;
                }

                /* Priority inheritance is requested, check to see if the thread that owns the mutex is lower priority.  */
                if (mutex_owner -> tx_thread_priority > thread_ptr -> tx_thread_priority)
                {

                    /* Yes, raise the suspended, owning thread's priority to that
                       of the current thread.  */
                    _tx_mutex_priority_change(mutex_owner, thread_ptr -> tx_thread_priority);

#ifdef TX_MUTEX_ENABLE_PERFORMANCE_INFO

                    /* Increment the total mutex priority inheritance counter.  */
                    _tx_mutex_performance__priority_inheritance_count++;

                    /* Increment the number of priority inheritance situations on this mutex.  */
                    mutex_ptr -> tx_mutex_performance__priority_inheritance_count++;
#endif
                }
            }

            /* Call actual non-interruptable thread suspension routine.  */
            _tx_thread_system_ni_suspend(thread_ptr, wait_option);

            /* Restore interrupts.  */
            TX_RESTORE
#else

            /* Set the suspending flag.  */
            thread_ptr -> tx_thread_suspending =  TX_TRUE;

            /* Setup the timeout period.  */
            thread_ptr -> tx_thread_timer.tx_timer_internal_remaining_ticks =  wait_option;

            /* Temporarily disable preemption.  */
            _tx_thread_preempt_disable++;

            /* Restore interrupts.  */
            TX_RESTORE

            /* Determine if we need to raise the priority of the thread 
               owning the mutex.  */
            if (mutex_ptr -> tx_mutex_inherit == TX_TRUE)
            {

                /* Determine if this is the highest priority to raise for this mutex.  */
                if (mutex_ptr -> tx_mutex_highest_priority_waiting > thread_ptr -> tx_thread_priority)
                {

                    /* Remember this priority.  */
                    mutex_ptr -> tx_mutex_highest_priority_waiting =  thread_ptr -> tx_thread_priority;
                }

                /* Priority inheritance is requested, check to see if the thread that owns the mutex is lower priority.  */
                if (mutex_owner -> tx_thread_priority > thread_ptr -> tx_thread_priority)
                {

                    /* Yes, raise the suspended, owning thread's priority to that
                       of the current thread.  */
                    _tx_mutex_priority_change(mutex_owner, thread_ptr -> tx_thread_priority);

#ifdef TX_MUTEX_ENABLE_PERFORMANCE_INFO

                    /* Increment the total mutex priority inheritance counter.  */
                    _tx_mutex_performance__priority_inheritance_count++;

                    /* Increment the number of priority inheritance situations on this mutex.  */
                    mutex_ptr -> tx_mutex_performance__priority_inheritance_count++;
#endif
                }
            }

            /* Call actual thread suspension routine.  */
            _tx_thread_system_suspend(thread_ptr);
#endif
            /* Return the completion status.  */
            status =  thread_ptr -> tx_thread_suspend_status;
        }
        else
        {

            /* Restore interrupts.  */
            TX_RESTORE 

            /* Immediate return, return error completion.  */
            status =  TX_NOT_AVAILABLE;
        }
    }

    ux_test_do_action_after(&action, &action_params);

    /* Perform hooked callbacks.  */
    ux_test_do_hooks_after(UX_TEST_OVERRIDE_TX_MUTEX_GET, &action_params);

    /* Return completion status.  */
    return(status);
}

/* Re-target the _ux_utility_memory_allocate for testing */

VOID  ux_test_utility_sim_mem_alloc_log_enable(UCHAR enable_disable)
{
    if (enable_disable)
    {
        mem_alloc_fail_after = FAIL_DISABLE;
        mem_alloc_count = 0;
        mem_alloc_do_fail = UX_FALSE;

        ux_system_mutex_on_count = 0;
        ux_system_mutex_off_count = 0;
        ux_system_mutex_alloc_logs_count = 0;
        ux_system_mutex_alloc_logs_match = 0;
        ux_system_mutex_alloc_logs_area = UX_FALSE;
        ux_system_mutex_alloc_logs_lock  = UX_FALSE;
        _ux_utility_memory_set(ux_system_mutex_alloc_logs, 0, sizeof(ux_system_mutex_alloc_logs));

        ux_test_link_hooks_from_array(ux_system_mutex_hooks);
    }
    else
    {
        ux_test_remove_hooks_from_array(ux_system_mutex_hooks);
    }
}

VOID ux_test_utility_sim_mem_alloc_log_lock(VOID)
{
    ux_system_mutex_alloc_logs_lock = UX_TRUE;
#if 0 /* TODO: Dump mem alloc log map.  */
    printf("Lock mem log map, %ld area:\n", ux_system_mutex_alloc_logs_count + 1);
    for (int i = 0; i <= ux_system_mutex_alloc_logs_count; i ++)
    {
        printf(" : %6ld ~ %6ld\n", ux_system_mutex_alloc_logs[i].first_count, ux_system_mutex_alloc_logs[i].last_count);
    }
#endif
}

ULONG ux_test_utility_sim_mem_alloc_count(VOID)
{
    return mem_alloc_count;
}

VOID ux_test_utility_sim_mem_alloc_count_reset(VOID)
{
    mem_alloc_fail_after = FAIL_DISABLE;
    mem_alloc_do_fail = UX_FALSE;
    mem_alloc_count = 0;

    ux_system_mutex_off_count = 0;
    ux_system_mutex_on_count = 0;

    ux_system_mutex_alloc_logs_count = 0;
    ux_system_mutex_alloc_logs_area = UX_FALSE;
    ux_system_mutex_alloc_logs_lock  = UX_FALSE;
    _ux_utility_memory_set(ux_system_mutex_alloc_logs, 0, sizeof(ux_system_mutex_alloc_logs));
}

VOID ux_test_utility_sim_mem_alloc_error_generation_start(ULONG fail_after)
{

    mem_alloc_count = 0;
    mem_alloc_do_fail = UX_FALSE;

    ux_system_mutex_off_count = 0;
    ux_system_mutex_on_count = 0;

    ux_system_mutex_alloc_logs_match = 0;

    mem_alloc_fail_after = fail_after;
}

VOID ux_test_utility_sim_mem_alloc_error_generation_stop(VOID)
{

    mem_alloc_fail_after = FAIL_DISABLE;
    mem_alloc_count = 0;

    ux_system_mutex_off_count = 0;
    ux_system_mutex_on_count = 0;
}

UINT ux_test_utility_sim_mem_alloc_error_generation_active(VOID)
{
    if (mem_alloc_fail_after == FAIL_DISABLE)
        return UX_ERROR;
    if (mem_alloc_count >= mem_alloc_fail_after)
        return UX_SUCCESS;
    return UX_ERROR;
}

/* Override. */
UINT  _tx_thread_preemption_change(TX_THREAD *thread_ptr, UINT new_threshold, UINT *old_threshold)
{

TX_INTERRUPT_SAVE_AREA

#ifndef TX_DISABLE_PREEMPTION_THRESHOLD
ULONG                                   priority_bit;
#if TX_MAX_PRIORITIES > 32
UINT                                    map_index;
#endif
#endif
UINT                                    status;
UX_TEST_OVERRIDE_TX_THREAD_PREEMPTION_CHANGE_PARAMS params = { thread_ptr, new_threshold };
UX_TEST_ACTION                          action;


    action = ux_test_action_handler(UX_TEST_OVERRIDE_TX_THREAD_PREEMPTION_CHANGE, &params);
    ux_test_do_action_before(&action, &params);

    /* Default status to success.  */
    status =  TX_SUCCESS;

#ifdef TX_DISABLE_PREEMPTION_THRESHOLD

    /* Only allow 0 (disable all preemption) and returning preemption-threshold to the 
       current thread priority if preemption-threshold is disabled. All other threshold
       values are converted to 0.  */
    if (thread_ptr -> tx_thread_user_priority != new_threshold)
    {
    
        /* Is the new threshold zero?  */
        if (new_threshold != ((UINT) 0))
        {
        
            /* Convert the new threshold to disable all preemption, since preemption-threshold is
               not supported.  */
            new_threshold =  ((UINT) 0);
        }
    }
#endif

    /* Lockout interrupts while the thread is being resumed.  */
    TX_DISABLE

    /* If trace is enabled, insert this event into the trace buffer.  */
    TX_TRACE_IN_LINE_INSERT(TX_TRACE_THREAD_PREEMPTION_CHANGE, thread_ptr, new_threshold, thread_ptr -> tx_thread_preempt_threshold, thread_ptr -> tx_thread_state, TX_TRACE_THREAD_EVENTS)

    /* Log this kernel call.  */
    TX_EL_THREAD_PREEMPTION_CHANGE_INSERT

    /* Determine if the new threshold is greater than the current user priority.  */
    if (new_threshold > thread_ptr -> tx_thread_user_priority)
    {
            
        /* Return error.  */
        status =  TX_THRESH_ERROR;
    }
    else
    {

#ifndef TX_DISABLE_PREEMPTION_THRESHOLD

        /* Determine if the new threshold is the same as the priority.  */
        if (thread_ptr -> tx_thread_user_priority == new_threshold)
        {

            /* Determine if this thread is at the head of the list.  */
            if (_tx_thread_priority_list[thread_ptr -> tx_thread_priority] == thread_ptr)
            {

#if TX_MAX_PRIORITIES > 32

                /* Calculate the index into the bit map array.  */
                map_index =  (thread_ptr -> tx_thread_priority)/((UINT) 32);
#endif

                /* Yes, this thread is at the front of the list.  Make sure
                   the preempted bit is cleared for this thread.  */
                TX_MOD32_BIT_SET(thread_ptr -> tx_thread_priority, priority_bit)
                _tx_thread_preempted_maps[MAP_INDEX] =  _tx_thread_preempted_maps[MAP_INDEX] & (~(priority_bit));

#if TX_MAX_PRIORITIES > 32

                /* Determine if there are any other bits set in this preempt map.  */
                if (_tx_thread_preempted_maps[MAP_INDEX] == ((ULONG) 0))
                {

                    /* No, clear the active bit to signify this preempt map has nothing set.  */
                    TX_DIV32_BIT_SET(thread_ptr -> tx_thread_priority, priority_bit)
                    _tx_thread_preempted_map_active =  _tx_thread_preempted_map_active & (~(priority_bit));
                }
#endif
            }
        }
#endif

        /* Return the user's preemption-threshold.   */
        *old_threshold =  thread_ptr -> tx_thread_user_preempt_threshold;

        /* Setup the new threshold.  */
        thread_ptr -> tx_thread_user_preempt_threshold =  new_threshold;

        /* Determine if the new threshold represents a higher priority than the priority inheritance threshold.  */
        if (new_threshold < thread_ptr -> tx_thread_inherit_priority)
        {
    
            /* Update the actual preemption-threshold with the new threshold.  */
            thread_ptr -> tx_thread_preempt_threshold =  new_threshold;
        }
        else
        {
    
            /* Update the actual preemption-threshold with the priority inheritance.  */
            thread_ptr -> tx_thread_preempt_threshold =  thread_ptr -> tx_thread_inherit_priority;
        }

        /* Is the thread priority less than the current highest priority?  If not, no preemption is required.  */
        if (_tx_thread_highest_priority < thread_ptr -> tx_thread_priority)
        {

            /* Is the new thread preemption-threshold less than the current highest priority?  If not, no preemption is required.  */
            if (_tx_thread_highest_priority < new_threshold)
            {

                /* If the current execute pointer is the same at this thread, preemption needs to take place.  */
                if (_tx_thread_execute_ptr == thread_ptr) 
                {

                    /* Preemption needs to take place.  */

#ifndef TX_DISABLE_PREEMPTION_THRESHOLD

                    /* Determine if this thread has preemption threshold set.  */
                    if (thread_ptr -> tx_thread_preempt_threshold != thread_ptr -> tx_thread_priority)
                    {

#if TX_MAX_PRIORITIES > 32

                        /* Calculate the index into the bit map array.  */
                        map_index =  (thread_ptr -> tx_thread_priority)/((UINT) 32);

                        /* Set the active bit to remember that the preempt map has something set.  */
                        TX_DIV32_BIT_SET(thread_ptr -> tx_thread_priority, priority_bit)
                        _tx_thread_preempted_map_active =  _tx_thread_preempted_map_active | priority_bit;
#endif

                        /* Remember that this thread was preempted by a thread above the thread's threshold.  */
                        TX_MOD32_BIT_SET(thread_ptr -> tx_thread_priority, priority_bit)
                        _tx_thread_preempted_maps[MAP_INDEX] =  _tx_thread_preempted_maps[MAP_INDEX] | priority_bit;
                    }
#endif

#ifdef TX_THREAD_ENABLE_PERFORMANCE_INFO

                    /* Determine if the caller is an interrupt or from a thread.  */
                    if (TX_THREAD_GET_SYSTEM_STATE() == ((ULONG) 0))
                    {

                        /* Caller is a thread, so this is a solicited preemption.  */
                        _tx_thread_performance_solicited_preemption_count++;

                        /* Increment the thread's solicited preemption counter.  */
                        thread_ptr -> tx_thread_performance_solicited_preemption_count++;
                    }

                    /* Remember the thread that preempted this thread.  */
                    thread_ptr -> tx_thread_performance_last_preempting_thread =  _tx_thread_priority_list[_tx_thread_highest_priority];

                    /* Is the execute pointer different?  */
                    if (_tx_thread_performance_execute_log[_tx_thread_performance__execute_log_index] != _tx_thread_execute_ptr)
                    {
                     
                        /* Move to next entry.  */
                        _tx_thread_performance__execute_log_index++;
            
                        /* Check for wrap condition.  */
                        if (_tx_thread_performance__execute_log_index >= TX_THREAD_EXECUTE_LOG_SIZE)
                        {
          
                            /* Set the index to the beginning.  */                  
                            _tx_thread_performance__execute_log_index =  ((UINT) 0);
                        }
            
                        /* Log the new execute pointer.  */
                        _tx_thread_performance_execute_log[_tx_thread_performance__execute_log_index] =  _tx_thread_execute_ptr;
                    }
#endif

                    /* Setup the highest priority thread to execute.  */
                    _tx_thread_execute_ptr =  _tx_thread_priority_list[_tx_thread_highest_priority];

                    /* Restore interrupts.  */
                    TX_RESTORE

                    /* Check for preemption.  */
                    _tx_thread_system_preempt_check();
                    
                    /* Disable interrupts.  */
                    TX_DISABLE
                }
            }
        }
    }

    /* Restore interrupts.  */
    TX_RESTORE

    ux_test_do_action_after(&action, &params);
    
    /* Return completion status.  */
    return(status);
}

static void ux_system_mutex_create_callback(UX_TEST_ACTION *action, VOID *params)
{
UX_TEST_OVERRIDE_TX_MUTEX_CREATE_PARAMS *mutex_create_param = params;

    /* Log mutex pointer.  */
    ux_system_mutex_hooks[1].mutex_ptr = mutex_create_param->mutex_ptr;
    ux_system_mutex_hooks[2].mutex_ptr = mutex_create_param->mutex_ptr;

    ux_system_mutex_off_count = 0;
    ux_system_mutex_on_count = 0;

    mem_alloc_count = 0;
}

static void ux_system_mutex_get_callback(UX_TEST_ACTION *action, VOID *params)
{
UX_TEST_OVERRIDE_TX_MUTEX_GET_PARAMS *mutex_create_param = params;
ULONG                                this_count = ux_system_mutex_on_count;

    if (ux_system_mutex_callback_skip)
        return;

    ux_system_mutex_on_count ++;

    rmem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    cmem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available;

    /* Just return if error generation is disabled. */
    if (mem_alloc_fail_after == FAIL_DISABLE)
        return;

    /* Always fail if we are short of logs. */
    /* When failing started, never stop it. */
    if (ux_system_mutex_alloc_logs_match > ux_system_mutex_alloc_logs_count || mem_alloc_do_fail == UX_TRUE)
        mem_alloc_do_fail = UX_TRUE;

    /* Check log baseline to increase memory allocate count. */
    else if (this_count >= ux_system_mutex_alloc_logs[ux_system_mutex_alloc_logs_match].first_count &&
             this_count <= ux_system_mutex_alloc_logs[ux_system_mutex_alloc_logs_match].last_count)
    {
        /* Consume all memory if we will generate memory allocation error. */
        if (mem_alloc_count >= mem_alloc_fail_after)
        {
            // printf("%s:%d malloc error start @ %ld (%ld)\n", __FILE__, __LINE__, this_count, mem_alloc_count);
            mem_alloc_do_fail = UX_TRUE;
        }
        mem_alloc_count ++;

        if (ux_system_mutex_on_count > ux_system_mutex_alloc_logs[ux_system_mutex_alloc_logs_match].last_count)
        {
            ux_system_mutex_alloc_logs_match ++;
        }
    }

    if (mem_alloc_do_fail)
    {
        ux_system_mutex_callback_skip = UX_TRUE;
        ux_test_utility_sim_mem_allocate_until_flagged(0, UX_REGULAR_MEMORY);
        ux_test_utility_sim_mem_allocate_until_flagged(0, UX_CACHE_SAFE_MEMORY);
        ux_system_mutex_callback_skip = UX_FALSE;
    }
}

static void ux_system_mutex_put_callback(UX_TEST_ACTION *action, VOID *params)
{
UX_TEST_OVERRIDE_TX_MUTEX_PUT_PARAMS *mutex_create_param = params;
ULONG                                this_count = ux_system_mutex_off_count;

    if (ux_system_mutex_callback_skip)
        return;

    ux_system_mutex_off_count ++;
    UX_TEST_ASSERT(ux_system_mutex_on_count == ux_system_mutex_off_count);

    if (mem_alloc_do_fail)
    {
        ux_system_mutex_callback_skip = UX_TRUE;
        ux_test_utility_sim_mem_free_all_flagged(UX_REGULAR_MEMORY);
        ux_test_utility_sim_mem_free_all_flagged(UX_CACHE_SAFE_MEMORY);
        ux_system_mutex_callback_skip = UX_FALSE;
    }

    /* We stop logging when generating errors. */
    if (mem_alloc_fail_after != FAIL_DISABLE)
        return;

    /* We stop logging when it's locked. */
    if (ux_system_mutex_alloc_logs_lock)
        return;

    /* It's memory allocate, if memory level down.  */
    if (_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available < rmem_free ||
        _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available < cmem_free)
    {
        /* Memory allocate count.  */
        mem_alloc_count ++;

        /* Log memory allocate count.  */
        if (ux_system_mutex_alloc_logs_area == UX_FALSE)
        {
            ux_system_mutex_alloc_logs_area = UX_TRUE;
            ux_system_mutex_alloc_logs[ux_system_mutex_alloc_logs_count].first_count = this_count;
        }
        ux_system_mutex_alloc_logs[ux_system_mutex_alloc_logs_count].last_count  = this_count;
    }
    else if(ux_system_mutex_alloc_logs_area == UX_TRUE)
    {
        ux_system_mutex_alloc_logs_area = UX_FALSE;

        UX_TEST_ASSERT(ux_system_mutex_alloc_logs_count != (SYSTEM_MUTEX_ALLOC_LOG_SIZE - 1));
        if (ux_system_mutex_alloc_logs_count < SYSTEM_MUTEX_ALLOC_LOG_SIZE - 1)
        {
            ux_system_mutex_alloc_logs_count ++;
        }
        else
        {
            ux_system_mutex_alloc_logs_lock = UX_TRUE;
        }
    }
}

UINT  _tx_mutex_put(TX_MUTEX *mutex_ptr)
{

TX_INTERRUPT_SAVE_AREA

TX_THREAD       *thread_ptr;            
TX_THREAD       *old_owner;             
UINT            old_priority;            
UINT            status;
TX_MUTEX        *next_mutex;            
TX_MUTEX        *previous_mutex;
UINT            owned_count;
UINT            suspended_count;
TX_THREAD       *current_thread;
TX_THREAD       *next_thread;
TX_THREAD       *previous_thread;
TX_THREAD       *suspended_thread;
UX_TEST_OVERRIDE_TX_MUTEX_PUT_PARAMS action_params = {mutex_ptr};

    /* Perform hooked callbacks.  */
    ux_test_do_hooks_before(UX_TEST_OVERRIDE_TX_MUTEX_PUT, &action_params);

    /* Setup status to indicate the processing is not complete.  */
    status =  TX_NOT_DONE;

    /* Disable interrupts to put an instance back to the mutex.  */
    TX_DISABLE

#ifdef TX_MUTEX_ENABLE_PERFORMANCE_INFO

    /* Increment the total mutex put counter.  */
    _tx_mutex_performance_put_count++;

    /* Increment the number of attempts to put this mutex.  */
    mutex_ptr -> tx_mutex_performance_put_count++;
#endif

    /* If trace is enabled, insert this event into the trace buffer.  */
    TX_TRACE_IN_LINE_INSERT(TX_TRACE_MUTEX_PUT, mutex_ptr, TX_POINTER_TO_ULONG_CONVERT(mutex_ptr -> tx_mutex_owner), mutex_ptr -> tx_mutex_ownership_count, TX_POINTER_TO_ULONG_CONVERT(&old_priority), TX_TRACE_MUTEX_EVENTS)

    /* Log this kernel call.  */
    TX_EL_MUTEX_PUT_INSERT

    /* Determine if this mutex is owned.  */
    if (mutex_ptr -> tx_mutex_ownership_count != ((UINT) 0))
    {

        /* Pickup the owning thread pointer.  */
        thread_ptr =  mutex_ptr -> tx_mutex_owner;

        /* Pickup thread pointer.  */
        TX_THREAD_GET_CURRENT(current_thread)

        /* Check to see if the mutex is owned by the calling thread.  */
        if (mutex_ptr -> tx_mutex_owner != current_thread)
        {
        
            /* Determine if the preempt disable flag is set, indicating that 
               the caller is not the application but from ThreadX. In such
               cases, the thread mutex owner does not need to match.  */
            if (_tx_thread_preempt_disable == ((UINT) 0))
            {

                /* Invalid mutex release.  */

                /* Restore interrupts.  */
                TX_RESTORE

                /* Caller does not own the mutex.  */
                status =  TX_NOT_OWNED;
            }
        }
        
        /* Determine if we should continue.  */
        if (status == TX_NOT_DONE)
        {
    
            /* Decrement the mutex ownership count.  */
            mutex_ptr -> tx_mutex_ownership_count--;

            /* Determine if the mutex is still owned by the current thread.  */
            if (mutex_ptr -> tx_mutex_ownership_count != ((UINT) 0))
            {

                /* Restore interrupts.  */
                TX_RESTORE

                /* Mutex is still owned, just return successful status.  */
                status =  TX_SUCCESS;
            }
            else
            {        

                /* Check for a NULL thread pointer, which can only happen during initialization.   */
                if (thread_ptr == TX_NULL)
                {

                    /* Restore interrupts.  */
                    TX_RESTORE

                    /* Mutex is now available, return successful status.  */
                    status =  TX_SUCCESS;
                }
                else
                {

                    /* The mutex is now available.   */
            
                    /* Remove this mutex from the owned mutex list.  */
                    
                    /* Decrement the ownership count.  */
                    thread_ptr -> tx_thread_owned_mutex_count--;

                    /* Determine if this mutex was the only one on the list.  */
                    if (thread_ptr -> tx_thread_owned_mutex_count == ((UINT) 0))
                    {

                        /* Yes, the list is empty.  Simply set the head pointer to NULL.  */
                        thread_ptr -> tx_thread_owned_mutex_list =  TX_NULL;
                    }
                    else
                    {

                        /* No, there are more mutexes on the list.  */

                        /* Link-up the neighbors.  */
                        next_mutex =                             mutex_ptr -> tx_mutex_owned_next;
                        previous_mutex =                         mutex_ptr -> tx_mutex_owned_previous;
                        next_mutex -> tx_mutex_owned_previous =  previous_mutex;
                        previous_mutex -> tx_mutex_owned_next =  next_mutex;

                        /* See if we have to update the created list head pointer.  */
                        if (thread_ptr -> tx_thread_owned_mutex_list == mutex_ptr)
                        {

                            /* Yes, move the head pointer to the next link. */
                            thread_ptr -> tx_thread_owned_mutex_list =  next_mutex; 
                        }
                    }

                    /* Determine if the simple, non-suspension, non-priority inheritance case is present.  */ 
                    if (mutex_ptr -> tx_mutex_suspension_list == TX_NULL)
                    {
                    
                        /* Is this a priority inheritance mutex?  */
                        if (mutex_ptr -> tx_mutex_inherit == TX_FALSE)
                        {

                            /* Yes, we are done - set the mutex owner to NULL.   */
                            mutex_ptr -> tx_mutex_owner =  TX_NULL;
                            
                            /* Restore interrupts.  */
                            TX_RESTORE

                            /* Mutex is now available, return successful status.  */
                            status =  TX_SUCCESS;
                        }
                    }
                     
                    /* Determine if the processing is complete.  */
                    if (status == TX_NOT_DONE)
                    {
   
                        /* Initialize original owner and thread priority.  */
                        old_owner =      TX_NULL;
                        old_priority =   thread_ptr -> tx_thread_user_priority;

                        /* Does this mutex support priority inheritance?  */
                        if (mutex_ptr -> tx_mutex_inherit == TX_TRUE)
                        {

#ifndef TX_NOT_INTERRUPTABLE

                            /* Temporarily disable preemption.  */
                            _tx_thread_preempt_disable++;

                            /* Restore interrupts.  */
                            TX_RESTORE
#endif

                            /* Search the owned mutexes for this thread to determine the highest priority for this 
                               former mutex owner to return to.  */
                            next_mutex =  thread_ptr -> tx_thread_owned_mutex_list;
                            while (next_mutex != TX_NULL)
                            {

                                /* Does this mutex support priority inheritance?  */
                                if (next_mutex -> tx_mutex_inherit == TX_TRUE)
                                {
                            
                                    /* Determine if highest priority field of the mutex is higher than the priority to 
                                       restore.  */
                                    if (next_mutex -> tx_mutex_highest_priority_waiting < old_priority)
                                    {

                                        /* Use this priority to return releasing thread to.  */
                                        old_priority =   next_mutex -> tx_mutex_highest_priority_waiting;
                                    }
                                }

                                /* Move mutex pointer to the next mutex in the list.  */
                                next_mutex =  next_mutex -> tx_mutex_owned_next;

                                /* Are we at the end of the list?  */
                                if (next_mutex == thread_ptr -> tx_thread_owned_mutex_list)
                                {
                            
                                    /* Yes, set the next mutex to NULL.  */
                                    next_mutex =  TX_NULL;
                                }
                            }

#ifndef TX_NOT_INTERRUPTABLE

                            /* Disable interrupts.  */
                            TX_DISABLE

                            /* Undo the temporarily preemption disable.  */
                            _tx_thread_preempt_disable--;
#endif
                        }

                        /* Determine if priority inheritance is in effect and there are one or more
                           threads suspended on the mutex.  */
                        if (mutex_ptr -> tx_mutex_suspended_count > ((UINT) 1))
                        {

                            /* Is priority inheritance in effect?  */
                            if (mutex_ptr -> tx_mutex_inherit == TX_TRUE)
                            {

                                /* Yes, this code is simply to ensure the highest priority thread is positioned
                                   at the front of the suspension list.  */

#ifndef TX_NOT_INTERRUPTABLE

                                /* Temporarily disable preemption.  */
                                _tx_thread_preempt_disable++;

                                /* Restore interrupts.  */
                                TX_RESTORE
#endif

                                /* Call the mutex prioritize processing to ensure the 
                                   highest priority thread is resumed.  */
#ifdef TX_MISRA_ENABLE
                                do
                                {
                                    status =  _tx_mutex_prioritize(mutex_ptr);
                                } while (status != TX_SUCCESS);
#else
                                _tx_mutex_prioritize(mutex_ptr);
#endif

                                /* At this point, the highest priority thread is at the
                                   front of the suspension list.  */

#ifndef TX_NOT_INTERRUPTABLE

                                /* Disable interrupts.  */
                                TX_DISABLE

                                /* Back off the preemption disable.  */
                                _tx_thread_preempt_disable--;
#endif
                            }
                        }

                        /* Now determine if there are any threads still waiting on the mutex.  */
                        if (mutex_ptr -> tx_mutex_suspension_list == TX_NULL)
                        {           

                            /* No, there are no longer any threads waiting on the mutex.  */

#ifndef TX_NOT_INTERRUPTABLE

                            /* Temporarily disable preemption.  */
                            _tx_thread_preempt_disable++;
 
                            /* Restore interrupts.  */
                            TX_RESTORE
#endif

                            /* Mutex is not owned, but it is possible that a thread that 
                               caused a priority inheritance to occur is no longer waiting
                               on the mutex.  */
                            if (mutex_ptr -> tx_mutex_inherit == TX_TRUE) 
                            {

                                /* Setup the highest priority waiting thread.  */
                                mutex_ptr -> tx_mutex_highest_priority_waiting =  (UINT) TX_MAX_PRIORITIES;
  
                                /* Determine if we need to restore priority.  */
                                if ((mutex_ptr -> tx_mutex_owner) -> tx_thread_priority != old_priority)
                                {
                      
                                    /* Yes, restore the priority of thread.  */
                                    _tx_mutex_priority_change(mutex_ptr -> tx_mutex_owner, old_priority);
                                }
                            }

#ifndef TX_NOT_INTERRUPTABLE

                            /* Disable interrupts again.  */
                            TX_DISABLE

                            /* Back off the preemption disable.  */
                            _tx_thread_preempt_disable--;
#endif

                            /* Clear the owner flag.  */
                            if (mutex_ptr -> tx_mutex_ownership_count == ((UINT) 0))
                            {

                                /* Set the mutex owner to NULL.  */
                                mutex_ptr -> tx_mutex_owner =  TX_NULL;
                            }

                            /* Restore interrupts.  */
                            TX_RESTORE

                            /* Check for preemption.  */
                            _tx_thread_system_preempt_check();

                            /* Set status to success.  */
                            status =  TX_SUCCESS;
                        }
                        else
                        {

                            /* Pickup the thread at the front of the suspension list.  */
                            thread_ptr =  mutex_ptr -> tx_mutex_suspension_list;

                            /* Save the previous ownership information, if inheritance is
                               in effect.  */
                            if (mutex_ptr -> tx_mutex_inherit == TX_TRUE)
                            {

                                /* Remember the old mutex owner.  */
                                old_owner =  mutex_ptr -> tx_mutex_owner;
        
                                /* Setup owner thread priority information.  */
                                mutex_ptr -> tx_mutex_original_priority =   thread_ptr -> tx_thread_priority;

                                /* Setup the highest priority waiting thread.  */
                                mutex_ptr -> tx_mutex_highest_priority_waiting =  (UINT) TX_MAX_PRIORITIES;
                            }
 
                            /* Determine how many mutexes are owned by this thread.  */
                            owned_count =  thread_ptr -> tx_thread_owned_mutex_count;

                            /* Determine if this thread owns any other mutexes that have priority inheritance.  */
                            if (owned_count == ((UINT) 0))
                            {

                                /* The owned mutex list is empty.  Add mutex to empty list.  */
                                thread_ptr -> tx_thread_owned_mutex_list =     mutex_ptr;
                                mutex_ptr -> tx_mutex_owned_next =             mutex_ptr;
                                mutex_ptr -> tx_mutex_owned_previous =         mutex_ptr;
                            }
                            else
                            {

                                /* Non-empty list. Link up the mutex.  */

                                /* Pickup tail pointer.  */
                                next_mutex =                            thread_ptr -> tx_thread_owned_mutex_list;
                                previous_mutex =                        next_mutex -> tx_mutex_owned_previous;

                                /* Place the owned mutex in the list.  */
                                next_mutex -> tx_mutex_owned_previous =  mutex_ptr;
                                previous_mutex -> tx_mutex_owned_next =  mutex_ptr;

                                /* Setup this mutex's next and previous created links.  */
                                mutex_ptr -> tx_mutex_owned_previous =   previous_mutex;
                                mutex_ptr -> tx_mutex_owned_next =       next_mutex;
                            }

                            /* Increment the number of mutexes owned counter.  */
                            thread_ptr -> tx_thread_owned_mutex_count =  owned_count + ((UINT) 1);

                            /* Mark the Mutex as owned and fill in the corresponding information.  */
                            mutex_ptr -> tx_mutex_ownership_count =  (UINT) 1;
                            mutex_ptr -> tx_mutex_owner =            thread_ptr;

                            /* Remove the suspended thread from the list.  */

                            /* Decrement the suspension count.  */
                            mutex_ptr -> tx_mutex_suspended_count--;
                
                            /* Pickup the suspended count.  */
                            suspended_count =  mutex_ptr -> tx_mutex_suspended_count;

                            /* See if this is the only suspended thread on the list.  */
                            if (suspended_count == TX_NO_SUSPENSIONS)
                            {

                                /* Yes, the only suspended thread.  */
    
                                /* Update the head pointer.  */
                                mutex_ptr -> tx_mutex_suspension_list =  TX_NULL;
                            }
                            else
                            {

                                /* At least one more thread is on the same expiration list.  */

                                /* Update the list head pointer.  */
                                next_thread =                                  thread_ptr -> tx_thread_suspended_next;
                                mutex_ptr -> tx_mutex_suspension_list =        next_thread;

                                /* Update the links of the adjacent threads.  */
                                previous_thread =                              thread_ptr -> tx_thread_suspended_previous;
                                next_thread -> tx_thread_suspended_previous =  previous_thread;
                                previous_thread -> tx_thread_suspended_next =  next_thread;
                            } 
 
                            /* Prepare for resumption of the first thread.  */

                            /* Clear cleanup routine to avoid timeout.  */
                            thread_ptr -> tx_thread_suspend_cleanup =  TX_NULL;

                            /* Put return status into the thread control block.  */
                            thread_ptr -> tx_thread_suspend_status =  TX_SUCCESS;        

#ifdef TX_NOT_INTERRUPTABLE

                            /* Determine if priority inheritance is enabled for this mutex.  */
                            if (mutex_ptr -> tx_mutex_inherit == TX_TRUE)
                            {

                                /* Yes, priority inheritance is requested.  */

                                /* Determine if there are any more threads still suspended on the mutex.  */
                                if (mutex_ptr -> tx_mutex_suspended_count != ((ULONG) 0))
                                {

                                    /* Determine if there are more than one thread suspended on the mutex.  */
                                    if (mutex_ptr -> tx_mutex_suspended_count > ((ULONG) 1))
                                    {

                                        /* If so, prioritize the list so the highest priority thread is placed at the
                                           front of the suspension list.  */
#ifdef TX_MISRA_ENABLE
                                        do
                                        {
                                            status =  _tx_mutex_prioritize(mutex_ptr);
                                        } while (status != TX_SUCCESS);
#else
                                        _tx_mutex_prioritize(mutex_ptr);
#endif
                                    }
    
                                    /* Now, pickup the list head and set the priority.  */

                                    /* Determine if there still are threads suspended for this mutex.  */
                                    suspended_thread =  mutex_ptr -> tx_mutex_suspension_list;
                                    if (suspended_thread != TX_NULL)
                                    {

                                        /* Setup the highest priority thread waiting on this mutex.  */
                                        mutex_ptr -> tx_mutex_highest_priority_waiting =  suspended_thread -> tx_thread_priority;
                                    }
                                }

                                /* Restore previous priority needs to be restored after priority
                                   inheritance.  */
                                if (old_owner != TX_NULL)
                                {
                    
                                    /* Determine if we need to restore priority.  */
                                    if (old_owner -> tx_thread_priority != old_priority)
                                    {
    
                                        /* Restore priority of thread.  */
                                        _tx_mutex_priority_change(old_owner, old_priority);
                                    }
                                }
                            }

                            /* Resume the thread!  */
                            _tx_thread_system_ni_resume(thread_ptr);

                            /* Restore interrupts.  */
                            TX_RESTORE
#else

                            /* Temporarily disable preemption.  */
                            _tx_thread_preempt_disable++;

                            /* Restore interrupts.  */
                            TX_RESTORE

                            /* Determine if priority inheritance is enabled for this mutex.  */
                            if (mutex_ptr -> tx_mutex_inherit == TX_TRUE)
                            {

                                /* Yes, priority inheritance is requested.  */
        
                                /* Determine if there are any more threads still suspended on the mutex.  */
                                if (mutex_ptr -> tx_mutex_suspended_count != TX_NO_SUSPENSIONS)
                                {

                                    /* Prioritize the list so the highest priority thread is placed at the
                                       front of the suspension list.  */
#ifdef TX_MISRA_ENABLE
                                    do
                                    {
                                        status =  _tx_mutex_prioritize(mutex_ptr);
                                    } while (status != TX_SUCCESS);
#else
                                    _tx_mutex_prioritize(mutex_ptr);
#endif
                            
                                    /* Now, pickup the list head and set the priority.  */

                                    /* Disable interrupts.  */
                                    TX_DISABLE

                                    /* Determine if there still are threads suspended for this mutex.  */
                                    suspended_thread =  mutex_ptr -> tx_mutex_suspension_list;
                                    if (suspended_thread != TX_NULL)
                                    {

                                        /* Setup the highest priority thread waiting on this mutex.  */
                                        mutex_ptr -> tx_mutex_highest_priority_waiting =  suspended_thread -> tx_thread_priority;
                                    }

                                    /* Restore interrupts.  */
                                    TX_RESTORE
                                }

                                /* Restore previous priority needs to be restored after priority
                                   inheritance.  */
                                if (old_owner != TX_NULL)
                                {
                    
                                    /* Is the priority different?  */
                                    if (old_owner -> tx_thread_priority != old_priority)
                                    {
        
                                        /* Restore the priority of thread.  */
                                        _tx_mutex_priority_change(old_owner, old_priority);
                                    }
                                }
                            }

                            /* Resume thread.  */
                            _tx_thread_system_resume(thread_ptr);
#endif
                     
                            /* Return a successful status.  */
                            status =  TX_SUCCESS;
                        }
                    }
                }
            }
        }
    }
    else
    {

        /* Restore interrupts.  */
        TX_RESTORE
    
        /* Caller does not own the mutex.  */
        status =  TX_NOT_OWNED;  
    }

    /* Perform hooked callbacks.  */
    ux_test_do_hooks_after(UX_TEST_OVERRIDE_TX_MUTEX_PUT, &action_params);

    /* Return the completion status.  */
    return(status);
}

UINT  _nx_packet_pool_create(NX_PACKET_POOL *pool_ptr, CHAR *name_ptr, ULONG payload_size,
                             VOID *pool_start, ULONG pool_size)
{

TX_INTERRUPT_SAVE_AREA

NX_PACKET_POOL *tail_ptr;              /* Working packet pool pointer */
ULONG           packets;               /* Number of packets in pool   */
ULONG           original_payload_size; /* Original payload size       */
ULONG           header_size;           /* Rounded header size         */
CHAR           *packet_ptr;            /* Working packet pointer      */
CHAR           *next_packet_ptr;       /* Next packet pointer         */
CHAR           *end_of_pool;           /* End of pool area            */
CHAR           *payload_address;       /* Address of the first payload*/
VOID           *rounded_pool_start;    /* Rounded stating address     */
UX_TEST_OVERRIDE_TX_THREAD_CREATE_PARAMS        action_params = { name_ptr };
UX_TEST_ACTION                                  action;


    /* Perform action.  */
    action = ux_test_action_handler(UX_TEST_OVERRIDE_NX_PACKET_POOL_CREATE, &action_params);
    ux_test_do_action_before(&action, &action_params);
    if (ux_test_is_expedient_on())
    {
        if (action.matched && !action.do_after)
        {
            if (!action.no_return)
            {
                return action.status;
            }
        }
    }

    /* Save the original payload size.  */
    original_payload_size =  payload_size;

    /* Align the starting address to four bytes. */
    /*lint -e{923} suppress cast between ULONG and pointer.  */
    rounded_pool_start = (VOID *)((((ALIGN_TYPE)pool_start + NX_PACKET_ALIGNMENT  - 1) / NX_PACKET_ALIGNMENT) * NX_PACKET_ALIGNMENT);

    /* Round the pool size down to something that is evenly divisible by alignment.  */
    /*lint -e{923} suppress cast between ULONG and pointer.  */
    pool_size = (ULONG)(((pool_size - ((ALIGN_TYPE)rounded_pool_start - (ALIGN_TYPE)pool_start)) / NX_PACKET_ALIGNMENT) * NX_PACKET_ALIGNMENT);

    /* Set the pool starting address. */
    pool_start = rounded_pool_start;

    /* Calculate the address of payload. */
    /*lint -e{923} suppress cast between ULONG and pointer.  */
    payload_address = (CHAR *)((ALIGN_TYPE)rounded_pool_start + sizeof(NX_PACKET));

    /* Align the address of payload. */
    /*lint -e{923} suppress cast between ULONG and pointer.  */
    payload_address = (CHAR *)((((ALIGN_TYPE)payload_address + NX_PACKET_ALIGNMENT  - 1) / NX_PACKET_ALIGNMENT) * NX_PACKET_ALIGNMENT);

    /* Calculate the header size. */
    /*lint -e{923} suppress cast between ULONG and pointer.  */
    header_size = (ULONG)((ALIGN_TYPE)payload_address - (ALIGN_TYPE)rounded_pool_start);

    /* Round the packet size up to something that helps guarantee proper alignment for header and payload.  */
    payload_size = (ULONG)(((header_size + payload_size + NX_PACKET_ALIGNMENT  - 1) / NX_PACKET_ALIGNMENT) * NX_PACKET_ALIGNMENT - header_size);

    /* Clear pool fields. */
    memset(pool_ptr, 0, sizeof(NX_PACKET_POOL));

    /* Setup the basic packet pool fields.  */
    pool_ptr -> nx_packet_pool_name =             name_ptr;
    pool_ptr -> nx_packet_pool_suspension_list =  TX_NULL;
    pool_ptr -> nx_packet_pool_suspended_count =  0;
    pool_ptr -> nx_packet_pool_start =            (CHAR *)pool_start;
    pool_ptr -> nx_packet_pool_size =             pool_size;
    pool_ptr -> nx_packet_pool_payload_size =     original_payload_size;

    /* Calculate the end of the pool's memory area.  */
    end_of_pool =  ((CHAR *)pool_start) + pool_size;

    /* Walk through the pool area, setting up the available packet list.  */
    packets =            0;
    packet_ptr =         (CHAR *)rounded_pool_start;
    next_packet_ptr =    packet_ptr + (payload_size + header_size);

    /*lint -e{946} suppress pointer subtraction, since it is necessary. */
    while (next_packet_ptr <= end_of_pool)
    {

        /* Yes, we have another packet.  Increment the packet count.  */
        packets++;

        /* Setup the link to the next packet.  */
        /*lint -e{929} -e{740} -e{826} suppress cast of pointer to pointer, since it is necessary  */
        ((NX_PACKET *)packet_ptr) -> nx_packet_queue_next =  (NX_PACKET *)next_packet_ptr;

        /* Remember that this packet pool is the owner.  */
        /*lint -e{929} -e{740} -e{826} suppress cast of pointer to pointer, since it is necessary  */
        ((NX_PACKET *)packet_ptr) -> nx_packet_pool_owner =  pool_ptr;

#ifndef NX_DISABLE_PACKET_CHAIN
        /* Clear the next packet pointer.  */
        /*lint -e{929} -e{740} -e{826} suppress cast of pointer to pointer, since it is necessary  */
        ((NX_PACKET *)packet_ptr) -> nx_packet_next =  (NX_PACKET *)NX_NULL;
#endif /* NX_DISABLE_PACKET_CHAIN */

        /* Mark the packet as free.  */
        /*lint -e{929} -e{923} -e{740} -e{826} suppress cast of pointer to pointer, since it is necessary  */
        ((NX_PACKET *)packet_ptr) -> nx_packet_union_next.nx_packet_tcp_queue_next =  (NX_PACKET *)NX_PACKET_FREE;

        /* Setup the packet data pointers.  */
        /*lint -e{929} -e{928} -e{740} -e{826} suppress cast of pointer to pointer, since it is necessary  */
        ((NX_PACKET *)packet_ptr) -> nx_packet_data_start =  (UCHAR *)(packet_ptr + header_size);

        /*lint -e{929} -e{928} -e{740} -e{826} suppress cast of pointer to pointer, since it is necessary  */
        ((NX_PACKET *)packet_ptr) -> nx_packet_data_end =    (UCHAR *)(packet_ptr + header_size + original_payload_size);

        /* Add debug information. */
        NX_PACKET_DEBUG(__FILE__, __LINE__, (NX_PACKET *)packet_ptr);

        /* Advance to the next packet.  */
        packet_ptr =   next_packet_ptr;

        /* Update the next packet pointer.  */
        next_packet_ptr =  packet_ptr + (payload_size + header_size);
    }

    /* Backup to the last packet in the pool.  */
    packet_ptr =  packet_ptr - (payload_size + header_size);

    /* Set the last packet's forward pointer to NULL.  */
    /*lint -e{929} -e{740} -e{826} suppress cast of pointer to pointer, since it is necessary  */
    ((NX_PACKET *)packet_ptr) -> nx_packet_queue_next =  NX_NULL;

    /* Save the remaining information in the pool control packet.  */
    pool_ptr -> nx_packet_pool_available =  packets;
    pool_ptr -> nx_packet_pool_total =      packets;

    /* Set the packet pool available list.  */
    pool_ptr -> nx_packet_pool_available_list =  (NX_PACKET *)pool_start;

    /* If trace is enabled, register this object.  */
    NX_TRACE_OBJECT_REGISTER(NX_TRACE_OBJECT_TYPE_PACKET_POOL, pool_ptr, name_ptr, payload_size, packets);

    /* If trace is enabled, insert this event into the trace buffer.  */
    NX_TRACE_IN_LINE_INSERT(NX_TRACE_PACKET_POOL_CREATE, pool_ptr, payload_size, pool_start, pool_size, NX_TRACE_PACKET_EVENTS, 0, 0);

    /* Disable interrupts to place the packet pool on the created list.  */
    TX_DISABLE

    /* Setup the packet pool ID to make it valid.  */
    pool_ptr -> nx_packet_pool_id =  NX_PACKET_POOL_ID;

    /* Place the packet pool on the list of created packet pools.  First,
       check for an empty list.  */
    if (_nx_packet_pool_created_ptr)
    {

        /* Pickup tail pointer.  */
        tail_ptr =  _nx_packet_pool_created_ptr -> nx_packet_pool_created_previous;

        /* Place the new packet pool in the list.  */
        _nx_packet_pool_created_ptr -> nx_packet_pool_created_previous =  pool_ptr;
        tail_ptr -> nx_packet_pool_created_next =  pool_ptr;

        /* Setup this packet pool's created links.  */
        pool_ptr -> nx_packet_pool_created_previous =  tail_ptr;
        pool_ptr -> nx_packet_pool_created_next =      _nx_packet_pool_created_ptr;
    }
    else
    {

        /* The created packet pool list is empty.  Add packet pool to empty list.  */
        _nx_packet_pool_created_ptr =                  pool_ptr;
        pool_ptr -> nx_packet_pool_created_next =      pool_ptr;
        pool_ptr -> nx_packet_pool_created_previous =  pool_ptr;
    }

    /* Increment the number of packet pools created.  */
    _nx_packet_pool_created_count++;

    /* Restore interrupts.  */
    TX_RESTORE

    /* Return NX_SUCCESS.  */
    return(NX_SUCCESS);
}

UINT  _nx_packet_allocate(NX_PACKET_POOL *pool_ptr,  NX_PACKET **packet_ptr,
                          ULONG packet_type, ULONG wait_option)
{
TX_INTERRUPT_SAVE_AREA

UINT       status;              /* Return status           */
TX_THREAD *thread_ptr;          /* Working thread pointer  */
NX_PACKET *work_ptr;            /* Working packet pointer  */
UX_TEST_OVERRIDE_NX_PACKET_ALLOCATE_PARAMS      action_params = { pool_ptr };
UX_TEST_ACTION                                  action;


    /* Perform action.  */
    action = ux_test_action_handler(UX_TEST_OVERRIDE_NX_PACKET_ALLOCATE, &action_params);
    ux_test_do_action_before(&action, &action_params);
    if (ux_test_is_expedient_on())
    {
        if (action.matched && !action.do_after)
        {
            if (!action.no_return)
            {
                return action.status;
            }
        }
    }

#ifdef TX_ENABLE_EVENT_TRACE
TX_TRACE_BUFFER_ENTRY *trace_event;
ULONG                  trace_timestamp;
#endif

    /* Make sure the packet_type does not go beyond nx_packet_data_end. */
    if (pool_ptr -> nx_packet_pool_payload_size < packet_type)
    {
        return(NX_INVALID_PARAMETERS);
    }

    /* Set the return pointer to NULL initially.  */
    *packet_ptr =   NX_NULL;

    /* If trace is enabled, insert this event into the trace buffer.  */
    NX_TRACE_IN_LINE_INSERT(NX_TRACE_PACKET_ALLOCATE, pool_ptr, 0, packet_type, pool_ptr -> nx_packet_pool_available, NX_TRACE_PACKET_EVENTS, &trace_event, &trace_timestamp);

    /* Disable interrupts to get a packet from the pool.  */
    TX_DISABLE

    /* Determine if there is an available packet.  */
    if (pool_ptr -> nx_packet_pool_available)
    {

        /* Yes, a packet is available.  Decrement the available count.  */
        pool_ptr -> nx_packet_pool_available--;

        /* Pickup the current packet pointer.  */
        work_ptr =  pool_ptr -> nx_packet_pool_available_list;

        /* Modify the available list to point at the next packet in the pool. */
        pool_ptr -> nx_packet_pool_available_list =  work_ptr -> nx_packet_queue_next;

        /* Setup various fields for this packet.  */
        work_ptr -> nx_packet_queue_next =   NX_NULL;
#ifndef NX_DISABLE_PACKET_CHAIN
        work_ptr -> nx_packet_next =         NX_NULL;
        work_ptr -> nx_packet_last =         NX_NULL;
#endif /* NX_DISABLE_PACKET_CHAIN */
        work_ptr -> nx_packet_length =       0;
        work_ptr -> nx_packet_prepend_ptr =  work_ptr -> nx_packet_data_start + packet_type;
        work_ptr -> nx_packet_append_ptr =   work_ptr -> nx_packet_prepend_ptr;
        work_ptr -> nx_packet_address.nx_packet_interface_ptr = NX_NULL;
#ifdef NX_ENABLE_INTERFACE_CAPABILITY
        work_ptr -> nx_packet_interface_capability_flag = 0;
#endif /* NX_ENABLE_INTERFACE_CAPABILITY */
        /* Set the TCP queue to the value that indicates it has been allocated.  */
        /*lint -e{923} suppress cast of ULONG to pointer.  */
        work_ptr -> nx_packet_union_next.nx_packet_tcp_queue_next =  (NX_PACKET *)NX_PACKET_ALLOCATED;

#ifdef FEATURE_NX_IPV6

        /* Clear the option state. */
        work_ptr -> nx_packet_option_state = 0;
#endif /* FEATURE_NX_IPV6 */

#ifdef NX_IPSEC_ENABLE

        /* Clear the ipsec state. */
        work_ptr -> nx_packet_ipsec_state = 0;
        work_ptr -> nx_packet_ipsec_sa_ptr = NX_NULL;
#endif /* NX_IPSEC_ENABLE */

#ifndef NX_DISABLE_IPV4
        /* Initialize the IP version field */
        work_ptr -> nx_packet_ip_version = NX_IP_VERSION_V4;
#endif /* !NX_DISABLE_IPV4  */

        /* Initialize the IP identification flag.  */
        work_ptr -> nx_packet_identical_copy = NX_FALSE;

        /* Initialize the IP header length. */
        work_ptr -> nx_packet_ip_header_length = 0;

#ifdef NX_ENABLE_THREAD
        work_ptr -> nx_packet_type = 0;
#endif /* NX_ENABLE_THREAD  */

        /* Place the new packet pointer in the return destination.  */
        *packet_ptr =  work_ptr;

        /* Set status to success.  */
        status =  NX_SUCCESS;

        /* Add debug information. */
        NX_PACKET_DEBUG(__FILE__, __LINE__, work_ptr);
    }
    else
    {

#ifndef NX_DISABLE_PACKET_INFO
        /* Increment the packet pool empty request count.  */
        pool_ptr -> nx_packet_pool_empty_requests++;
#endif

        /* Determine if the request specifies suspension.  */
        if (wait_option)
        {

            /* Prepare for suspension of this thread.  */

#ifndef NX_DISABLE_PACKET_INFO
            /* Increment the packet pool empty request suspension count.  */
            pool_ptr -> nx_packet_pool_empty_suspensions++;
#endif

            /* Pickup thread pointer.  */
            thread_ptr =  _tx_thread_current_ptr;

            /* Setup cleanup routine pointer.  */
            thread_ptr -> tx_thread_suspend_cleanup =  _nx_packet_pool_cleanup;

            /* Setup cleanup information, i.e. this pool control
               block.  */
            thread_ptr -> tx_thread_suspend_control_block =  (void *)pool_ptr;

            /* Save the return packet pointer address as well.  */
            thread_ptr -> tx_thread_additional_suspend_info =  (void *)packet_ptr;

            /* Save the packet type (or prepend offset) so this can be added
               after a new packet becomes available.  */
            thread_ptr -> tx_thread_suspend_info =  packet_type;

            /* Setup suspension list.  */
            if (pool_ptr -> nx_packet_pool_suspension_list)
            {

                /* This list is not NULL, add current thread to the end. */
                thread_ptr -> tx_thread_suspended_next =
                    pool_ptr -> nx_packet_pool_suspension_list;
                thread_ptr -> tx_thread_suspended_previous =
                    (pool_ptr -> nx_packet_pool_suspension_list) -> tx_thread_suspended_previous;
                ((pool_ptr -> nx_packet_pool_suspension_list) -> tx_thread_suspended_previous) -> tx_thread_suspended_next =
                    thread_ptr;
                (pool_ptr -> nx_packet_pool_suspension_list) -> tx_thread_suspended_previous =   thread_ptr;
            }
            else
            {

                /* No other threads are suspended.  Setup the head pointer and
                   just setup this threads pointers to itself.  */
                pool_ptr -> nx_packet_pool_suspension_list =  thread_ptr;
                thread_ptr -> tx_thread_suspended_next =            thread_ptr;
                thread_ptr -> tx_thread_suspended_previous =        thread_ptr;
            }

            /* Increment the suspended thread count.  */
            pool_ptr -> nx_packet_pool_suspended_count++;

            /* Set the state to suspended.  */
            thread_ptr -> tx_thread_state =  TX_TCP_IP;

            /* Set the suspending flag.  */
            thread_ptr -> tx_thread_suspending =  TX_TRUE;

            /* Temporarily disable preemption.  */
            _tx_thread_preempt_disable++;

            /* Save the timeout value.  */
            thread_ptr -> tx_thread_timer.tx_timer_internal_remaining_ticks =  wait_option;

            /* Restore interrupts.  */
            TX_RESTORE

            /* Call actual thread suspension routine.  */
            _tx_thread_system_suspend(thread_ptr);

            /* Update the trace event with the status.  */
            NX_TRACE_EVENT_UPDATE(trace_event, trace_timestamp, NX_TRACE_PACKET_ALLOCATE, 0, *packet_ptr, 0, 0);

#ifdef NX_ENABLE_PACKET_DEBUG_INFO
            if (thread_ptr -> tx_thread_suspend_status == NX_SUCCESS)
            {

                /* Add debug information. */
                NX_PACKET_DEBUG(__FILE__, __LINE__, *packet_ptr);
            }
#endif /* NX_ENABLE_PACKET_DEBUG_INFO */

            /* Return the completion status.  */
            return(thread_ptr -> tx_thread_suspend_status);
        }
        else
        {

            /* Immediate return, return error completion.  */
            status =  NX_NO_PACKET;
        }
    }

    /* Restore interrupts.  */
    TX_RESTORE

    /* Update the trace event with the status.  */
    NX_TRACE_EVENT_UPDATE(trace_event, trace_timestamp, NX_TRACE_PACKET_ALLOCATE, 0, *packet_ptr, 0, 0);

    /* Return completion status.  */
    return(status);
}

VOID  ux_test_utility_sim_event_create_count_reset    (VOID)
{
    event_create_count = 0;
}
ULONG ux_test_utility_sim_event_create_count          (VOID)
{
    return event_create_count;
}
VOID  ux_test_utility_sim_event_error_generation_start(ULONG fail_after)
{
    event_create_count = 0;
    event_fail_after = fail_after;
}
VOID  ux_test_utility_sim_event_error_generation_stop (VOID)
{
    event_fail_after = FAIL_DISABLE;
    event_create_count = 0;
}

UINT  _tx_event_flags_create(TX_EVENT_FLAGS_GROUP *group_ptr, CHAR *name_ptr)
{

TX_INTERRUPT_SAVE_AREA

TX_EVENT_FLAGS_GROUP    *next_group;
TX_EVENT_FLAGS_GROUP    *previous_group;


    if (event_fail_after != FAIL_DISABLE)
    {

        if (event_create_count >= event_fail_after)
        {

            /* Return testing error instead of actual creation. */
            return UX_MUTEX_ERROR;
        }
    }

    /* Do actual creating. */
    event_create_count ++;


    /* Initialize event flags control block to all zeros.  */
    TX_MEMSET(group_ptr, 0, (sizeof(TX_EVENT_FLAGS_GROUP)));

    /* Setup the basic event flags group fields.  */
    group_ptr -> tx_event_flags_group_name =             name_ptr;
    
    /* Disable interrupts to put the event flags group on the created list.  */
    TX_DISABLE

    /* Setup the event flags ID to make it valid.  */
    group_ptr -> tx_event_flags_group_id =  TX_EVENT_FLAGS_ID;

    /* Place the group on the list of created event flag groups.  First,
       check for an empty list.  */
    if (_tx_event_flags_created_count == TX_EMPTY)
    {

        /* The created event flags list is empty.  Add event flag group to empty list.  */
        _tx_event_flags_created_ptr =                         group_ptr;
        group_ptr -> tx_event_flags_group_created_next =      group_ptr;
        group_ptr -> tx_event_flags_group_created_previous =  group_ptr;
    }
    else
    {

        /* This list is not NULL, add to the end of the list.  */
        next_group =      _tx_event_flags_created_ptr;
        previous_group =  next_group -> tx_event_flags_group_created_previous;

        /* Place the new event flag group in the list.  */
        next_group -> tx_event_flags_group_created_previous =  group_ptr;
        previous_group -> tx_event_flags_group_created_next =  group_ptr;

        /* Setup this group's created links.  */
        group_ptr -> tx_event_flags_group_created_previous =  previous_group;
        group_ptr -> tx_event_flags_group_created_next =      next_group;    
    }

    /* Increment the number of created event flag groups.  */
    _tx_event_flags_created_count++;
    
    /* Optional event flag group create extended processing.  */
    TX_EVENT_FLAGS_GROUP_CREATE_EXTENSION(group_ptr)

    /* If trace is enabled, register this object.  */
    TX_TRACE_OBJECT_REGISTER(TX_TRACE_OBJECT_TYPE_EVENT_FLAGS, group_ptr, name_ptr, 0, 0)

    /* If trace is enabled, insert this event into the trace buffer.  */
    TX_TRACE_IN_LINE_INSERT(TX_TRACE_EVENT_FLAGS_CREATE, group_ptr, TX_POINTER_TO_ULONG_CONVERT(&next_group), 0, 0, TX_TRACE_EVENT_FLAGS_EVENTS)

    /* Log this kernel call.  */
    TX_EL_EVENT_FLAGS_CREATE_INSERT

    /* Restore interrupts.  */
    TX_RESTORE

    /* Return TX_SUCCESS.  */
    return(TX_SUCCESS);
}


/****** IO inp?/outp? *****************/
static ULONG *_ux_test_sim_inp_seq = UX_NULL;
static ULONG  _ux_test_sim_inp_seq_len = 0;
static ULONG  _ux_test_sim_inp_seq_i = 0;
static ULONG _ux_test_sim_inp_seq_value(VOID)
{
    if (_ux_test_sim_inp_seq && _ux_test_sim_inp_seq_len)
    {
        if (_ux_test_sim_inp_seq_i >= _ux_test_sim_inp_seq_len)
            return _ux_test_sim_inp_seq[_ux_test_sim_inp_seq_len - 1];

        return _ux_test_sim_inp_seq[_ux_test_sim_inp_seq_i];
    }
    else
    {
        return 0;
    }
    
}
static VOID _ux_test_sim_inp_seq_inc(VOID)
{
    if (_ux_test_sim_inp_seq_i < _ux_test_sim_inp_seq_len)
        _ux_test_sim_inp_seq_i ++;
}
VOID  ux_test_sim_inp_sequence_set(ULONG* seq, ULONG len)
{
    _ux_test_sim_inp_seq = seq;
    _ux_test_sim_inp_seq_len = len;
    _ux_test_sim_inp_seq_i = 0;
}
UCHAR   inpb(ULONG addr)
{
    UCHAR value = (UCHAR)_ux_test_sim_inp_seq_value();
    (void)addr;
    _ux_test_sim_inp_seq_inc();
    return value;
}
#ifndef _MSC_BUILD
USHORT  inpw(ULONG addr)
#else
USHORT  inpw_mok(ULONG addr)
#endif
{
    USHORT value = (USHORT)_ux_test_sim_inp_seq_value();
    (void)addr;
    _ux_test_sim_inp_seq_inc();
    return value;
}
ULONG   inpl(ULONG addr)
{
    ULONG value = (ULONG)_ux_test_sim_inp_seq_value();
    (void)addr;
    _ux_test_sim_inp_seq_inc();
    return value;
}


static ULONG* _ux_test_sim_outp_logbuf = UX_NULL;
static ULONG  _ux_test_sim_outp_logbuf_size = 0;
static ULONG  _ux_test_sim_outp_logbuf_i = 0;
static VOID  _ux_test_sim_outp_log_save(ULONG addr, ULONG value)
{
    if (!_ux_test_sim_outp_logbuf || !_ux_test_sim_outp_logbuf_size)
        return;

    if (_ux_test_sim_outp_logbuf_i < _ux_test_sim_outp_logbuf_size - 1)
    {
        _ux_test_sim_outp_logbuf[_ux_test_sim_outp_logbuf_i ++] = addr;
        _ux_test_sim_outp_logbuf[_ux_test_sim_outp_logbuf_i ++] = value;
    }
}
ULONG ux_test_sim_outp_log_count(VOID)
{
    return (_ux_test_sim_outp_logbuf_i >> 1);
}
VOID  ux_test_sim_outp_log_reset(VOID)
{
    _ux_test_sim_outp_logbuf_i = 0;
}
VOID  ux_test_sim_outp_logbuf_set(ULONG* buf, ULONG size)
{
    _ux_test_sim_outp_logbuf = buf;
    _ux_test_sim_outp_logbuf_size = size;
    _ux_test_sim_outp_logbuf_i = 0;
}
ULONG ux_test_sim_outp_log_get(ULONG seq, ULONG *addr, ULONG *value)
{
    seq = seq << 1;
    if (seq >= _ux_test_sim_outp_logbuf_i)
        return 0;
    if (addr)
        *addr = _ux_test_sim_outp_logbuf[seq];
    if (value)
        *value = _ux_test_sim_outp_logbuf[seq + 1];
    return 1;
}
UCHAR outpb(ULONG addr, UCHAR b)
{
    _ux_test_sim_outp_log_save(addr, b);
    return b;
}
#ifndef _MSC_BUILD
USHORT  outpw(ULONG addr, USHORT w)
#else
USHORT  outpw_mok(ULONG addr, USHORT w)
#endif
{
    _ux_test_sim_outp_log_save(addr, w);
    return w;
}
ULONG outpl(ULONG addr, ULONG l)
{
    _ux_test_sim_outp_log_save(addr, l);
#ifdef _MSC_BUILD
    return l;
#endif
}
