# Overview

This guide is about how to implement device controller driver (DCD) of specific hardware for USBX.

# Device Controller Driver (DCD)

The device controller driver is a driver layer to provide a unique way to access different USB hardware peripherals.

## DCD for USBX Device Stack

The USBX device stack accesses device controller through a unique struct of device controller driver (DCD).
After USBX system initialization (`ux_system_initialize`), the USBX DCD struct (`UX_SLAVE_DCD`) is allocated and can be referenced by
`_ux_system_slave -> ux_system_slave_dcd`.

## DCD for specific USB controller hardware

For different USB controller hardware the DCD struct is initialized by specific initialize function, called in application.
The actual implements are then linked for USBX device stack to communicate, so that the USB flow can be performed on actual physical device.
There are three parts in such a interface:
* APIs for application
* APIs for USBX stack
* USB events (callbacks) to call USBX stack

### APIs for application

#### _ux_dcd_xxxxx_initialize

The initialize function for specific DCD is called by USBX application, after `ux_system_initialize`, `ux_device_stack_initialize` and `ux_device_stack_class_register`.
It allocates necessary resources for the driver.
Usually DCD operates directly on hardware, so the initialize function initialize hardware and connect USB.

Implement example is as following:

```c
/**************************************************************************/
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function initializes the USB device controller of the XXXXX    */
/*    microcontroller in USBX device system.                              */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    dcd_io                                Driver specific parameter 1   */
/*    parameter                             Driver specific parameter 2   */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/**************************************************************************/
UINT  _ux_dcd_xxx_initialize(ULONG dcd_io, ULONG parameter)
{

UX_SLAVE_DCD            *dcd;
UX_DCD_XXXXX            *dcd_xxxxx;


    UX_PARAMETER_NOT_USED(dcd_io);

    /* Get the pointer to the DCD.  */
    dcd =  &_ux_system_slave -> ux_system_slave_dcd;

    /* The controller initialized here is of XXXXX type.  */
    dcd -> ux_slave_dcd_controller_type =  UX_DCD_XXXXX_SLAVE_CONTROLLER;

    /* Allocate memory for this XXXXX DCD instance.  */
    dcd_xxxxx =  _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_DCD_XXXXX));

    /* Check if memory was properly allocated.  */
    if(dcd_xxxxx == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Set the pointer to the XXXXX DCD.  */
    dcd -> ux_slave_dcd_controller_hardware =  (VOID *) dcd_xxxxx;

    /* Initialize the function collector for this DCD.  */
    dcd -> ux_slave_dcd_function =  _ux_dcd_xxxxx_function;

    /* Set the generic DCD owner for the XXXXX DCD.  */
    dcd_xxxxx -> ux_dcd_xxxxx_dcd_owner =  dcd;

    /* Initialize XXXXX DCD.  */
    /* TODO: Initialize XXXXX DCD struct fields.  */
    /* TODO: if not initialized in application (after invoking this function),
             initialize controller hardware and attach to host.  */

    /* Set the state of the controller to OPERATIONAL now.  */
    dcd -> ux_slave_dcd_status =  UX_DCD_STATUS_OPERATIONAL;

    /* Return successful completion.  */
    return(UX_SUCCESS);
}
```

#### ux_dcd_xxxx_uninitialize (optional)

The uninitialize function for specific DCD is optional,
since the USB device system is usually enabled and always ready to serve.
In some cases, it is called by USBX application,
if the application needs a way to disconnect USB, completely free resources and uninitialize hardware.

Code example is as following:
```c
/**************************************************************************/
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function uninitializes the USB device controller in USBX       */
/*    device system.                                                      */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    dcd_io                                Driver specific parameter 1   */
/*    parameter                             Driver specific parameter 2   */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/**************************************************************************/
UINT  _ux_dcd_xxxxx_uninitialize(ULONG dcd_io, ULONG parameter)
{

UX_SLAVE_DCD            *dcd;
UX_DCD_XXXXX            *dcd_xxxxx;


    UX_PARAMETER_NOT_USED(dcd_io);
    UX_PARAMETER_NOT_USED(parameter);

    /* Get the pointer to the DCD.  */
    dcd =  &_ux_system_slave -> ux_system_slave_dcd;

    /* Set the state of the controller to HALTED now.  */
    dcd -> ux_slave_dcd_status =  UX_DCD_STATUS_HALTED;

    /* Get controller driver.  */
    dcd_xxxxx = (UX_DCD_XXXXX *)dcd -> ux_slave_dcd_controller_hardware;

    /* Free XXXXX driver.  */
    {
        _ux_utility_memory_free(dcd_xxxxx);
        dcd -> ux_slave_dcd_controller_hardware = UX_NULL;
    }

    /* TODO: If not uninitialized in application (after invoking this function),
             detach from host and uninitialize the controller.  */

    return(UX_SUCCESS);
}
```

### Stack APIs for USBX

#### ux_dcd_xxxxx_function

In USBX device stack, the commands sent to controller is in a single API call defined as:

```c
UINT ux_dcd_xxxxx_function(UX_SLAVE_DCD *dcd, UINT cmd, VOID *param);
```

The commands should be implemented as following:

| command                                     | parameter                       | description                                       |
| ------------------------------------------- | ------------------------------- | ------------------------------------------------- |
| UX_DCD_TRANSFER_REQUEST/UX_DCD_TRANSFER_RUN | UX_SLAVE_TRANSFER*              | Issue a transfer/run transfer state machine       |
| UX_DCD_TRANSFER_ABORT                       | UX_SLAVE_TRANSFER*              | Abort on going transfer                           |
| UX_DCD_CREATE_ENDPOINT                      | UX_SLAVE_ENDPOINT*              | Create a physical endpoint                        |
| UX_DCD_DESTROY_ENDPOINT                     | UX_SLAVE_ENDPOINT*              | Destroy a physical endpoint                       |
| UX_DCD_RESET_ENDPOINT                       | UX_SLAVE_ENDPOINT*              | Reset an endpoint                                 |
| UX_DCD_STALL_ENDPOINT                       | UX_SLAVE_ENDPOINT*              | Stall an endpoint                                 |
| UX_DCD_ENDPOINT_STATUS                      | ULONG                           | Return status of endpoint (index/address)         |
| UX_DCD_SET_DEVICE_ADDRESS                   | ULONG                           | Set the device address                            |
| UX_DCD_CHANGE_STATE                         | ULONG (UX_DEVICE_REMOTE_WAKEUP) | Change USB state (to remote wakeup)               |
| UX_DCD_ISR_PENDING/UX_DCD_TASKS_RUN         | -                               | For standalone, run background task state machine |

Note for command `UX_DCD_CHANGE_STATE`, parameter of `UX_DEVICE_REMOTE_WAKEUP` should be accepted and send remote wakeup signal to host.

Implement example is as following:
```c
/**************************************************************************/
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function dispatches the DCD function internally to the XXXXX   */
/*    controller in USBX device system.                                   */
/*                                                                        */
/*    Following commands are supported:                                   */
/*    - UX_DCD_TRANSFER_REQUEST/UX_DCD_TRANSFER_RUN                       */
/*      - control transfer: mandatory for all devices;                    */
/*      - bulk/interrupt/isochronous transfer: optional if needed, e.g.,   */
/*        bulk is needed by CDC, Mass Storage classes,                    */
/*        interrupt is needed by HID class,                               */
/*        isochronous is needed by audio/video classes, etc.              */
/*    - UX_DCD_TRANSFER_ABORT                                             */
/*    - UX_DCD_CREATE_ENDPOINT                                            */
/*    - UX_DCD_DESTROY_ENDPOINT                                           */
/*    - UX_DCD_RESET_ENDPOINT                                             */
/*    - UX_DCD_ENDPOINT_STATUS                                            */
/*    - UX_DCD_SET_DEVICE_ADDRESS                                         */
/*    - UX_DCD_CHANGE_STATE                                               */
/*      - UX_DEVICE_REMOTE_WAKEUP: optional if remote wakeup is supported */
/*    - UX_DCD_ISR_PENDING/UX_DCD_TASKS_RUN                               */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    dcd                                   Pointer to device controller  */
/*    function                              Function requested            */
/*    parameter                             Pointer to function parameters*/
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Device Stack                                                   */
/*                                                                        */
/**************************************************************************/
UINT  _ux_dcd_xxxxx_function(UX_SLAVE_DCD *dcd, UINT function, VOID *parameter)
{

UINT                status = UX_FUNCTION_NOT_SUPPORTED;
UX_DCD_XXXXX        *dcd_xxxxx;


    /* Check the status of the controller.  */
    if (dcd -> ux_slave_dcd_status == UX_UNUSED)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_DCD, UX_CONTROLLER_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_CONTROLLER_UNKNOWN, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_CONTROLLER_UNKNOWN);
    }

    /* Get the pointer to the XXXXX DCD.  */
    dcd_xxxxx =  (UX_DCD_XXXXX *) dcd -> ux_slave_dcd_controller_hardware;

    /* Look at the function and route it.  */
    switch(function)
    {

    case UX_DCD_TRANSFER_REQUEST:

        /* TODO: controller level transfer state machine/transfer request, parameter: UX_SLAVE_TRANSFER *transfer. */
        /* Transfer types to support:
           Control: mandatory for all devices;
           Bulk: optional, for functions that need bulk endpoint (like CDC-ACM, Mass Storage, etc.);
           Interrupt: optional, for function that need interrupt endpoint (like HID, CDC-ACM, etc.);
           Isochronous: optional, for function that need Isochronous endpoint (like audio, video, etc.).
         */
#if defined(UX_DEVICE_STANDALONE)
        /* Must be non-blocking;
           in start state: start background transfer and return UX_STATE_NEXT;
           in pending state: return UX_STATE_WAIT;
           in done state: confirm transfer fields including completion code are updated and return UX_STATE_NEXT. */
        /* ......  */
#else
        /* control request: start background transfer for data stage followed by status stage.
        /* non-control: it's blocking, start background transfer and wait transfer->ux_slave_transfer_request_semaphore. */
        /* ......  */
#endif
        break;

    case UX_DCD_TRANSFER_ABORT:
        /* TODO: controller level transfer abort, parameter: UX_SLAVE_TRANSFER *transfer. */
        /* ......  */
        break;

    case UX_DCD_CREATE_ENDPOINT:

        /* TODO: controller level endpoint create, parameter: UX_SLAVE_ENDPOINT *endpoint. */
        /* ......  */
        break;

    case UX_DCD_DESTROY_ENDPOINT:

        /* TODO: controller level endpoint destroy, parameter: UX_SLAVE_ENDPOINT *endpoint. */
        /* ......  */
        break;

    case UX_DCD_RESET_ENDPOINT:

        /* TODO: controller level endpoint reset to clear halt and reset toggle, parameter: UX_SLAVE_ENDPOINT *endpoint.  */
        /* ......  */
        break;

    case UX_DCD_STALL_ENDPOINT:

        /* TODO: controller level endpoint stall, parameter: UX_SLAVE_ENDPOINT *endpoint. */
        /* ......  */
        break;

    case UX_DCD_SET_DEVICE_ADDRESS:

        /* TODO: update hardware to set address of the device, parameter: ULONG address.  */
        /* ......  */
        break;

    case UX_DCD_CHANGE_STATE:

        /* TODO: actively change USB state in controller level, parameter: ULONG state. It could be:
           - UX_DEVICE_REMOTE_WAKEUP: send remote wakeup signal on USB bus (if device needs the feature). */
        /* ......  */
        break;

    case UX_DCD_ENDPOINT_STATUS:

        /* TODO: get endpoint status (stall).. */
        /* If UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT is defined parameter is ULONG endpoint_address,
           which kept endpoint direction, in this case both 0x81 and 0x01 are accepted as different
           endpoints addresses.
           Otherwise, parameter is ULONG endpoint_index, which removes endpoint direction, in this case
           address of 0x81 and 0x01 both pass index 0x01 as parameter, so the index must be unique. */
        /* ......  */
        break;

#if defined(UX_DEVICE_STANDALONE)
    case UX_DCD_TASKS_RUN:

        /* TODO: controller level background tasks running. */
        /* ......  */
        status = UX_SUCCESS;
        break;
#endif

    default:

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_DCD, status);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, status, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        break;
    }

    /* Return completion status.  */
    return(status);
}
```

### Events CALLs to USBX

Following USBX device stack function should be called in event handling:

#### _ux_device_stack_control_request_process(UX_SLAVE_TRANSFER *)
* Control SETUP received and IN (device to host) data is expected
* Control SETUP received and there is no OUT (host to device) data
* Control SETUP received and OUT data also received

E.g., for a DCD based on STM32 HAL, the code reference could be:
```c
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    /* ...... */

    /* Check if the transaction is IN.  */
    if (*transfer_request -> ux_slave_transfer_request_setup & UX_REQUEST_IN)
    {

        /* ...... */

        /* Call the Control Transfer dispatcher.  */
        _ux_device_stack_control_request_process(transfer_request);

    }
    else
    {
        /* ....... */

        /* We are in a OUT transaction. Check if there is a data payload. If so, wait for the payload
           to be delivered.  */
        if (*(transfer_request -> ux_slave_transfer_request_setup + 6) == 0 &&
            *(transfer_request -> ux_slave_transfer_request_setup + 7) == 0)
        {

            /* Call the Control Transfer dispatcher.  */
            _ux_device_stack_control_request_process(transfer_request);

            /* TODO: start background control request status phase.  */
            /* ...... */
        }
        else
        {
            /* TODO: start background control request data OUT transfer,
                     when transfer/partial transfer is done, HAL_PCD_DataOutStageCallback is triggered.  */
            /* ...... */
        }
    }
}
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    /* Endpoint 0 is the control endpoint.  */
    if (epnum == 0U)
    {
        /* Check if we have received something on control endpoint during data phase .  */
        if (ed -> ux_dcd_stm32_ed_state == UX_DCD_STM32_ED_STATE_DATA_RX)
        {
                /* Are we done with this transfer ? */
                if ((transfer_request -> ux_slave_transfer_request_actual_length ==
                     transfer_request -> ux_slave_transfer_request_requested_length) || 
                    (transfer_length != endpoint -> ux_slave_endpoint_descriptor.wMaxPacketSize))
                {
                    /* ...... */

                    _ux_device_stack_control_request_process(transfer_request);
                }
        }
    }

}
```

#### _ux_device_stack_disconnect(void)
* A USB bus reset happen in not reset states.
* A USB bus disconnect happen in not reset states

E.g., for a DCD based on STM32 HAL, the code reference could be:
```c
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
    /* If the device is attached or configured, we need to disconnect it.  */
    if (_ux_system_slave -> ux_system_slave_device.ux_slave_device_state !=  UX_DEVICE_RESET)
    {

        /* Disconnect the device.  */
        _ux_device_stack_disconnect();
    }

    /* Put the device in attached default state.  */
    /* ...... */
}
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{

    /* Check if the device is attached or configured.  */
    if (_ux_system_slave -> ux_system_slave_device.ux_slave_device_state !=  UX_DEVICE_RESET)
    {

        /* Disconnect the device.  */
        _ux_device_stack_disconnect();
    }
}
```

#### _ux_system_slave -> ux_system_slave_speed and _ux_system_slave -> ux_system_slave_device.ux_slave_device_state
* Updated when USB bus reset is done

E.g., for a DCD based on STM32 HAL, the code reference could be:
```c
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{

    /* ...... */

    /* Set USB Current Speed */
    switch(hpcd -> Init.speed)
    {
    case PCD_SPEED_HIGH:

        /* We are connected at high speed.  */
        _ux_system_slave -> ux_system_slave_speed =  UX_HIGH_SPEED_DEVICE;
        break;

    case PCD_SPEED_FULL:

        /* We are connected at full speed.  */
        _ux_system_slave -> ux_system_slave_speed =  UX_FULL_SPEED_DEVICE;
        break;   

    default:

        /* We are connected at full speed.  */
        _ux_system_slave -> ux_system_slave_speed =  UX_FULL_SPEED_DEVICE;
        break;
    }

    /* ...... */

    /* Mark the device as attached now.  */
    _ux_system_slave -> ux_system_slave_device.ux_slave_device_state =  UX_DEVICE_ATTACHED;
}
```
