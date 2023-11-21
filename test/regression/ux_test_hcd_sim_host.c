/* This test simulator is designed to simulate ux_hcd_ APIs for test. */

#include "tx_api.h"
#include "tx_thread.h"

#include "ux_api.h"
#include "ux_hcd_sim_host.h"

#include "ux_test_utility_sim.h"
#include "ux_test_hcd_sim_host.h"

void  test_control_return(UINT status);

ULONG ux_test_port_status = UX_PS_CCS | UX_PS_DS_FS;

/* We have a no wait version because in a real world scenario, there is no waiting. */
VOID ux_test_hcd_sim_host_disconnect_no_wait(VOID)
{

UINT            status;
UX_HCD          *hcd = &_ux_system_host -> ux_system_host_hcd_array[0];
UX_HCD_SIM_HOST *hcd_sim_host = hcd -> ux_hcd_controller_hardware;


    /* Port is disconnected. */
    ux_test_port_status = 0;
    if (hcd_sim_host != UX_NULL)
    {
#if !defined(UX_HOST_STANDALONE)
        /* Don't let the timer function run, or else the HCD thread will perform transfer requests. */
        status = tx_timer_deactivate(&hcd_sim_host -> ux_hcd_sim_host_timer);
        if (status != TX_SUCCESS)
        {

            printf("test_hcd_sim_host #%d, error: %d\n", __LINE__, status);
            test_control_return(1);
        }
#endif
    }
    /* Signal change on the port (HCD0.RH.PORT0). */
    hcd -> ux_hcd_root_hub_signal[0] = 1;
    /* Signal detach to host enum thread. */
    _ux_host_semaphore_put(&_ux_system_host -> ux_system_host_enum_semaphore);
}

VOID ux_test_hcd_sim_host_disconnect(VOID)
{

    ux_test_hcd_sim_host_disconnect_no_wait();

#if defined(UX_HOST_STANDALONE)
    {
        UX_HCD *hcd = &_ux_system_host -> ux_system_host_hcd_array[0];
        ULONG n_device = hcd -> ux_hcd_nb_devices;
        for (unsigned i = 0; i < 20000; i ++)
        {
            ux_system_tasks_run();
            if (hcd -> ux_hcd_nb_devices == 0 ||
                hcd -> ux_hcd_nb_devices < n_device)
                break;
        }
    }
#else

    /* Sleep current thread for enum thread to run. */
    tx_thread_sleep(100);
#endif
}

VOID ux_test_hcd_sim_host_connect_no_wait(ULONG speed)
{

UINT            status;
UX_HCD          *hcd = &_ux_system_host -> ux_system_host_hcd_array[0];
UX_HCD_SIM_HOST *hcd_sim_host = hcd -> ux_hcd_controller_hardware;


    /* Connect with specific speed. */
    switch(speed)
    {
        case UX_LOW_SPEED_DEVICE:

            ux_test_port_status = UX_PS_CCS | UX_PS_DS_LS;
            break;

        case UX_FULL_SPEED_DEVICE:

            ux_test_port_status = UX_PS_CCS | UX_PS_DS_FS;
            break;

        case UX_HIGH_SPEED_DEVICE:

            ux_test_port_status = UX_PS_CCS | UX_PS_DS_HS;
            break;

        default:
            break;
    }

#if !defined(UX_HOST_STANDALONE)
    /* Allow the timer function to run, or else the HCD thread won't perform transfer requests. */
    status = tx_timer_activate(&hcd_sim_host -> ux_hcd_sim_host_timer);
    if (status != TX_SUCCESS &&
        /* Was the timer already active? */
        status != TX_ACTIVATE_ERROR)
    {

        printf("test_hcd_sim_host #%d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }
#endif

    /* Signal change on the port (HCD0.RH.PORT0). */
    _ux_system_host -> ux_system_host_hcd_array -> ux_hcd_root_hub_signal[0] = 1;
    /* Signal detach to host enum thread. */
    _ux_host_semaphore_put(&_ux_system_host -> ux_system_host_enum_semaphore);
}

VOID ux_test_hcd_sim_host_connect(ULONG speed)
{

    ux_test_hcd_sim_host_connect_no_wait(speed);

    /* Sleep current thread for enum thread to run. */
    tx_thread_sleep(100);
}

/* Fork and modify _ux_hcd_sim_host_port_status_get. */

ULONG _ux_hcd_sim_host_port_status_get(UX_HCD_SIM_HOST *hcd_sim_host, ULONG port_index)
{

    /* Check to see if this port is valid on this controller.  */
    if (hcd_sim_host -> ux_hcd_sim_host_nb_root_hubs < port_index)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_PORT_INDEX_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_PORT_INDEX_UNKNOWN, port_index, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_PORT_INDEX_UNKNOWN);
    }

    /* Return port status.  */
    return(ux_test_port_status);
}

static UINT last_function = 0;
static char *_func_name[] = {
    "",
    "DISABLE_CONTROLLER",
    "GET_PORT_STATUS",
    "ENABLE_PORT",
    "DISABLE_PORT",
    "POWER_ON_PORT",
    "POWER_DOWN_PORT",
    "SUSPEND_PORT",
    "RESUME_PORT",
    "RESET_PORT",
    "GET_FRAME_NUMBER",
    "SET_FRAME_NUMBER",
    "TRANSFER_REQUEST",
    "TRANSFER_ABORT",
    "CREATE_ENDPOINT",
    "DESTROY_ENDPOINT",
    "RESET_ENDPOINT",
    "PROCESS_DONE_QUEUE"
};

#if 0 /* Not sure what the purpose of this is. -Nick */
UINT _ux_test_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT                    status;
UCHAR                   act  = 0;
UX_TRANSFER            *req;
UX_ENDPOINT            *ep;
TX_THREAD              *this_thread;
UINT                    i;
UCHAR                   action_matched;


#if 0 /* TODO: Enable/disable HCD entry call display */
if (function != last_function)
{
    last_function = function;
    printf("\n_H %2d(%s) ", function, _func_name[function]);
}
else
    printf(".");
#endif

    status = ux_test_cd_handle_action(action, hcd, UX_TEST_CD_TYPE_HCD, function, parameter, &action_matched);
    if (action_matched)
    {
        /* Proceed to next action */
        action++;
        if (action->function == 0)
            action = UX_NULL;
    }

    return status;
}
#endif

UINT _ux_test_hcd_sim_host_dummy_entry(UX_HCD *hcd, UINT function, VOID *parameter)
{
    if (function == UX_HCD_GET_PORT_STATUS)
        /* Never connected. */
        return 0;

    return _ux_hcd_sim_host_entry(hcd, function, parameter);
}

UINT _ux_test_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT                                            status;
UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   params = { hcd, function, parameter };
UX_TEST_ACTION                                  action;
ULONG                                           action_taken;

    /* Perform hooked callbacks.  */
    action_taken = ux_test_do_hooks_before(UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY, &params);
    if (action_taken & 0x80000000u) /* The hook breaks normal process.  */
        return(action.status);

    action = ux_test_action_handler(UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY, &params);
    ux_test_do_action_before(&action, &params);

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

    status = _ux_hcd_sim_host_entry(hcd, function, parameter);

    ux_test_do_action_after(&action, &params);

    if (ux_test_is_expedient_on())
    {
        if (action.matched && action.do_after)
        {
            if (!action.no_return)
            {
                return action.status;
            }
        }
    }

    /* Perform hooked callbacks.  */
    ux_test_do_hooks_after(UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY, &params);
    
    /* Return completion status.  */
    return(status);
}

UINT  _ux_test_hcd_sim_host_initialize(UX_HCD *hcd)
{
UINT status;

    if (ux_utility_name_match((UCHAR*)"dummy", hcd->ux_hcd_name, 5))
    {
        /* Use dummy halted */
        hcd -> ux_hcd_status = UX_HCD_STATUS_DEAD; /* Must not be UX_UNUSED == UX_HCD_STATUS_HALTED */
        hcd -> ux_hcd_entry_function = _ux_test_hcd_sim_host_dummy_entry;
        return UX_SUCCESS;
    }

    /* Use hcd sim host */
    status = _ux_hcd_sim_host_initialize(hcd);

    /* Redirect the entry function */
    hcd -> ux_hcd_entry_function =  _ux_test_hcd_sim_host_entry;

    return status;
}

VOID ux_test_hcd_sim_host_cleanup(VOID)
{

    ux_test_port_status = UX_PS_CCS | UX_PS_DS_FS;
}
