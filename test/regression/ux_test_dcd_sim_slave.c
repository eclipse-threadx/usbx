/* This test simulator is designed to redirect simulate ux_hcd_sim_slave_ APIs for test. */

#include "tx_api.h"
#include "tx_thread.h"

#include "ux_api.h"
#include "ux_dcd_sim_slave.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_utility_sim.h"

static ULONG _ux_dcd_sim_slave_speed = UX_FULL_SPEED_DEVICE;
static UCHAR *_ux_dcd_sim_framework = UX_NULL;
static ULONG _ux_dcd_sim_framework_length = 0;

static UX_TEST_DCD_SIM_ACTION *ux_test_actions = UX_NULL;
static UX_TEST_DCD_SIM_ACTION *ux_test_main_action_list;

VOID ux_test_dcd_sim_slave_cleanup(VOID)
{
    _ux_dcd_sim_slave_speed = UX_FULL_SPEED_DEVICE;

    _ux_dcd_sim_framework = UX_NULL;
    _ux_dcd_sim_framework_length = 0;

    ux_test_actions = UX_NULL;
    ux_test_main_action_list  = UX_NULL;
}

VOID ux_test_dcd_sim_slave_disconnect(VOID)
{

#if 1
UINT old_threshold;
#else
TX_INTERRUPT_SAVE_AREA
#endif

#if 1
    /* Disconnection is usually handled in an ISR, which cannot be preempted. Copy behavior.
       Note that the regular TX_DISABLE doesn't work on Windows. Might not work for Linux
       either. */
    tx_thread_preemption_change(tx_thread_identify(), 0, &old_threshold);
#else
    TX_DISABLE
#endif

    ux_device_stack_disconnect();

#if 1
    tx_thread_preemption_change(tx_thread_identify(), old_threshold, &old_threshold);
#else
    TX_RESTORE
#endif
}

VOID ux_test_dcd_sim_slave_connect(ULONG speed)
{
    _ux_dcd_sim_slave_speed = speed;
    //_ux_dcd_sim_slave_initialize_complete();
}

VOID ux_test_dcd_sim_slave_connect_framework(UCHAR * framework, ULONG framework_length)
{
    _ux_dcd_sim_framework = framework;
    _ux_dcd_sim_framework_length = framework_length;
    //_ux_dcd_sim_slave_initialize_complete();
}

/* Fork and modified to replace _ux_dcd_sim_slave_initialize_complete in lib for testing control. */
UINT  _ux_dcd_sim_slave_initialize_complete(VOID)
{

UX_SLAVE_DCD            *dcd;
UX_SLAVE_DEVICE         *device;
UCHAR *                 device_framework;
UX_SLAVE_TRANSFER       *transfer_request;

    /* Get the pointer to the DCD.  */
    dcd =  &_ux_system_slave -> ux_system_slave_dcd;

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    if (_ux_dcd_sim_framework != UX_NULL && _ux_dcd_sim_framework_length != 0)
    {

        /* Initialize customized framework.  */
        _ux_system_slave -> ux_system_slave_device_framework =  _ux_dcd_sim_framework;
        _ux_system_slave -> ux_system_slave_device_framework_length =  _ux_dcd_sim_framework_length;
    }
    else
    {
        /* Slave simulator is a Full/high speed controller.  */
        if (_ux_dcd_sim_slave_speed == UX_HIGH_SPEED_DEVICE)
        {

            _ux_system_slave -> ux_system_slave_device_framework =  _ux_system_slave -> ux_system_slave_device_framework_high_speed;
            _ux_system_slave -> ux_system_slave_device_framework_length =  _ux_system_slave -> ux_system_slave_device_framework_length_high_speed;
        }
        else
        {

            _ux_system_slave -> ux_system_slave_device_framework =  _ux_system_slave -> ux_system_slave_device_framework_full_speed;
            _ux_system_slave -> ux_system_slave_device_framework_length =  _ux_system_slave -> ux_system_slave_device_framework_length_full_speed;
        }
        /* Save device speed.  */
        _ux_system_slave->ux_system_slave_speed = _ux_dcd_sim_slave_speed;
    }

    /* Get the device framework pointer.  */
    device_framework =  _ux_system_slave -> ux_system_slave_device_framework;

    /* And create the decompressed device descriptor structure.  */
    _ux_utility_descriptor_parse(device_framework,
                                _ux_system_device_descriptor_structure,
                                UX_DEVICE_DESCRIPTOR_ENTRIES,
                                (UCHAR *) &device -> ux_slave_device_descriptor);

    /* Now we create a transfer request to accept the first SETUP packet
       and get the ball running. First get the address of the endpoint
       transfer request container.  */
    transfer_request =  &device -> ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

    /* Set the timeout to be for Control Endpoint.  */
    transfer_request -> ux_slave_transfer_request_timeout =  MS_TO_TICK(UX_CONTROL_TRANSFER_TIMEOUT);

    /* Adjust the current data pointer as well.  */
    transfer_request -> ux_slave_transfer_request_current_data_pointer =
                            transfer_request -> ux_slave_transfer_request_data_pointer;

    /* Update the transfer request endpoint pointer with the default endpoint.  */
    transfer_request -> ux_slave_transfer_request_endpoint =  &device -> ux_slave_device_control_endpoint;

    /* The control endpoint max packet size needs to be filled manually in its descriptor.  */
    transfer_request -> ux_slave_transfer_request_endpoint -> ux_slave_endpoint_descriptor.wMaxPacketSize =
                                device -> ux_slave_device_descriptor.bMaxPacketSize0;

    /* On the control endpoint, always expect the maximum.  */
    transfer_request -> ux_slave_transfer_request_requested_length =
                                device -> ux_slave_device_descriptor.bMaxPacketSize0;

    /* Attach the control endpoint to the transfer request.  */
    transfer_request -> ux_slave_transfer_request_endpoint =  &device -> ux_slave_device_control_endpoint;

    /* Create the default control endpoint attached to the device.
       Once this endpoint is enabled, the host can then send a setup packet
       The device controller will receive it and will call the setup function
       module.  */
    dcd -> ux_slave_dcd_function(dcd, UX_DCD_CREATE_ENDPOINT,
                                    (VOID *) &device -> ux_slave_device_control_endpoint);

    /* Ensure the control endpoint is properly reset.  */
    device -> ux_slave_device_control_endpoint.ux_slave_endpoint_state = UX_ENDPOINT_RESET;

    /* A SETUP packet is a DATA IN operation.  */
    transfer_request -> ux_slave_transfer_request_phase =  UX_TRANSFER_PHASE_DATA_IN;

    /* We are now ready for the USB device to accept the first packet when connected.  */
    return(UX_SUCCESS);
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
    "SET_DEVICE_ADDRESS",
    "ISR_PENDING",
    "CHANGE_STATE",
    "STALL_ENDPOINT",
    "ENDPOINT_STATUS"
};



UINT   _ux_test_dcd_sim_slave_function(UX_SLAVE_DCD *dcd, UINT function, VOID *parameter)
{

UINT                                                status;
UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS   params = { dcd, function, parameter };
UX_TEST_ACTION                                      action;
                                                        

    /* Perform hooked callbacks.  */
    ux_test_do_hooks_before(UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION, &params);

    action = ux_test_action_handler(UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION, &params);
    ux_test_do_action_before(&action, &params);

    /* NOTE: This shouldn't be used anymore. */
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

    status = _ux_dcd_sim_slave_function(dcd, function, parameter);

    ux_test_do_action_after(&action, &params);

    /* NOTE: This shouldn't be used anymore. */
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
    ux_test_do_hooks_after(UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION, &params);

    /* Return completion status.  */
    return(status);
}

UINT _ux_test_dcd_sim_slave_initialize(VOID)
{
UINT            status;
UX_SLAVE_DEVICE *device;
UX_SLAVE_DCD    *dcd;

    status = _ux_dcd_sim_slave_initialize();

    /* Redirect the entry function */
    device =  &_ux_system_slave -> ux_system_slave_device;
    dcd = &_ux_system_slave -> ux_system_slave_dcd;
    dcd -> ux_slave_dcd_function =  _ux_test_dcd_sim_slave_function;

    return status;
}

VOID ux_test_dcd_sim_slave_transfer_done(UX_SLAVE_TRANSFER *transfer, UINT code)
{
UX_SLAVE_ENDPOINT   *endpoint = transfer -> ux_slave_transfer_request_endpoint;
UX_DCD_SIM_SLAVE_ED *slave_ed = (UX_DCD_SIM_SLAVE_ED *)endpoint -> ux_slave_endpoint_ed;
    transfer -> ux_slave_transfer_request_completion_code = code;
    transfer -> ux_slave_transfer_request_status = UX_TRANSFER_STATUS_COMPLETED;
    slave_ed -> ux_sim_slave_ed_status |= UX_DCD_SIM_SLAVE_ED_STATUS_TRANSFER;
    slave_ed -> ux_sim_slave_ed_status |= UX_DCD_SIM_SLAVE_ED_STATUS_DONE;
    _ux_device_semaphore_put(&transfer -> ux_slave_transfer_request_semaphore);
}
