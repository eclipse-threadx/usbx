# Overview

This guide is about how to implement host controller driver (HCD) of specific hardware for USBX.

# Host Controller Driver (HCD)

The host controller driver is a driver layer to provide a unique way to access different USB host controllers.

## HCD for USBX Host Stack

The USBX host stack accesses host controller through a unique struct of host controller driver (HCD).
After USBX system initialization, the HCD (UX_HCD) pointer array is allocated and can be referenced by `_ux_system_host->ux_system_host_hcd_array`, the lower level instances are later allocated when invoking `ux_host_stack_hcd_register`, where `hcd_initialize_function` is passed as parameter and called to do memory allocation and lower level initialization.

## HCD for specific USB host controller hardware

For different USB host controller hardware the HCD struct is initialized when application calls `ux_host_stack_hcd_register`. The actual initialization function entry is passed for this registration function to do initialization. The lower level command dispatcher is linked during initialization for USBX host stack to communicate, so that the USB flow can be performed on actual physical controller.

The initialization function prototype is as following:
```c
UINT  _ux_hcd_xxxxx_initialize(UX_HCD *hcd);
```

### Initialize API (_ux_hcd_xxxxx_initialize)

The initialize function for specific HCD is passed to host stack HCD register function, after `ux_system_initialize`, `ux_host_stack_initialize` and `ux_host_stack_class_register`.
It allocates necessary resources for the lower level driver.
Usually HCD operates directly on hardware, so the initialization function initialize hardware to accept USB connection.

The implement example is as following:

```c
/**************************************************************************/
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function initializes the USB host controller or force OTG      */
/*    controller to work in host mode, in USBX host system.               */
/*                                                                        */
/*    The function entry is passed to ux_host_stack_hcd_register()        */
/*    to perform lower level initialization.                              */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    HCD                                   Pointer to HCD                */
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
/*    Host Stack                                                          */
/*                                                                        */
/**************************************************************************/
UINT  _ux_hcd_xxxxx_initialize(UX_HCD *hcd)
{

UX_HCD_XXXXX          *hcd_xxxxx;


    /* The controller initialized here is of XXXXX type.  */
    hcd -> ux_hcd_controller_type =  UX_HCD_XXXXX_CONTROLLER;

    /* Initialize the max bandwidth for periodic endpoints. On XXXXX, the spec says
       no more than 90% to be allocated for periodic.  */
#if UX_MAX_DEVICES > 1
    hcd -> ux_hcd_available_bandwidth =  UX_HCD_XXXXX_AVAILABLE_BANDWIDTH;
#endif

    /* Allocate memory for this XXXXX HCD instance.  */
    hcd_xxxxx =  _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HCD_XXXXX));
    if (hcd_xxxxx == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Set the pointer to the XXXXX HCD.  */
    hcd -> ux_hcd_controller_hardware =  (VOID *) hcd_xxxxx;

    /* Set the generic HCD owner for the XXXXX HCD.  */
    hcd_xxxxx -> ux_hcd_xxxxx_hcd_owner =  hcd;

    /* Initialize the function collector for this HCD.  */
    hcd -> ux_hcd_entry_function =  _ux_hcd_xxxxx_entry;

    /* Set the state of the controller to HALTED first.  */
    hcd -> ux_hcd_status =  UX_HCD_STATUS_HALTED;

    /* Since we know this is a high-speed controller, we can hardwire the version.  */
#if UX_MAX_DEVICES > 1
    hcd -> ux_hcd_version =  0x200;
#endif

    /* The number of ports on the controller.
       The number of ports needs to be reflected both
       for the generic HCD container and the local xxxxx container.  */
    hcd -> ux_hcd_nb_root_hubs             =  UX_HCD_XXXXX_NB_ROOT_PORTS;

    /* Initialize XXXXX HCD.  */
    /* TODO: Initialize XXXXX hCD struct fields.  */
    /* TODO: if not initialized in application (after invoking this function),
             initialize controller hardware, including roothub.
             After initialization, roothub ports must be activated to accpet device insertion  */

    /* Set the host controller into the operational state.  */
    hcd -> ux_hcd_status =  UX_HCD_STATUS_OPERATIONAL;

    /* Return successful completion.  */
    return(UX_SUCCESS);
}
```

### Commands for host stack to operate

In USBX host stack, the commands sent to controller are in a single API call defined as:

```c
UINT ux_hcd_xxxxx_entry_function(UX_HCD *hcd, UINT cmd, VOID *param);
```

The commands should be implemented as following:

| command                                     | parameter        | description                                                    |
| ------------------------------------------- | ---------------- | -------------------------------------------------------------- |
| UX_HCD_GET_PORT_STATUS                      | ULONG port_index | Return roothub port status                                     |
| UX_HCD_ENABLE_PORT                          | ULONG port_index | Enable roothub port                                            |
| UX_HCD_RESET_PORT                           | ULONG port_index | Issue a USB bus reset on roothub port                          |
| UX_HCD_TRANSFER_REQUEST/UX_HCD_TRANSFER_RUN | UX_TRANSFER*     | Issue a transfer request/run transfer request state machine    |
| UX_HCD_TRANSFER_ABORT                       | UX_TRANSFER*     | Abort a transfer request                                       |
| UX_HCD_CREATE_ENDPOINT                      | UX_ENDPOINT*     | Create a physical endpoint                                     |
| UX_HCD_DESTROY_ENDPOINT                     | UX_ENDPOINT*     | Destroy a physical endpoint                                    |
| UX_HCD_RESET_ENDPOINT                       | UX_ENDPOINT*     | Reset a physical endpoint (clear stall)                        |
| UX_HCD_PROCESS_DONE_QUEUE/UX_HCD_TASKS_RUN  | not used         | Process background tasks (e.g., process done queue)            |
| UX_HCD_UNINITIALIZE                         | not used         | Uninitialize controller hardware and free resources (optional) |

The implement example is as following:

```c
/**************************************************************************/
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function dispatch the HCD function internally to the           */
/*    controller driver.                                                  */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    HCD                                   Pointer to HCD                */
/*    function                              Function for driver to perform*/
/*    parameter                             Pointer to parameter(s)       */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Host Stack                                                          */
/*                                                                        */
/**************************************************************************/
UINT  _ux_hcd_xxxxx_entry(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT                status;
UX_HCD_xxxxx       *hcd_xxxxx;
UX_INT_SAVE_AREA


    /* Check the status of the controller.  */
    if (hcd -> ux_hcd_status == UX_UNUSED)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_CONTROLLER_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_CONTROLLER_UNKNOWN, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_CONTROLLER_UNKNOWN);
    }

    /* Get the pointer to the xxxxx HCD.  */
    hcd_xxxxx =  (UX_HCD_xxxxx *) hcd -> ux_hcd_controller_hardware;

    /* look at the function and route it.  */
    switch(function)
    {
    case UX_HCD_GET_PORT_STATUS:
        /* TODO: Lower level roothub port status get, return port status.  */
        /*       PORT_STATUS _entry(hcd,UX_HCD_GET_PORT_STATUS,(ULONG)port_index)  */
        /* ......  */
        break;

    case UX_HCD_ENABLE_PORT:
        /* TODO: Lower level roothub port enable.  */
        /*       STATUS _entry(hcd,UX_HCD_ENABLE_PORT,(ULONG)port_index).  */
        /* ......  */
        break;

    case UX_HCD_RESET_PORT:
        /* TODO: Lower level roothub port reset.  */
        /*       STATUS _entry(hcd,UX_HCD_RESET_PORT,(ULONG)port_index).  */
        /* ......  */
        break;

    case UX_HCD_TRANSFER_REQUEST:
        /* TODO: Lower level transfer request process.  */
        /* Transfer types to support:
           Control: mandatory for all devices;
           Bulk: optional, for functions that need bulk endpoint (like CDC-ACM, Mass Storage, etc.);
           Interrupt: optional, for function that need interrupt endpoint (like HID, CDC-ACM, etc.);
           Isochronous: optional, for function that need Isochronous endpoint (like audio, video, etc.).
         */
        /*       STATUS _entry(hcd,UX_HCD_TRANSFER_REQUEST,(UX_TRANSFER*)transfer).  */
#if !defined(UX_DEVICE_STANDALONE)
        /* For RTOS mode.  */
        /* control request: blocking until setup-data-status phases passed and status updated,
                            transfer_request -> ux_transfer_request_semaphore could be used to wait
                            transfer or phase terminating;
        /* bulk request: non-blocking, starts background transfer for a single transfer request.  */
        /* periodic request: non-blocking, ADDs transfer request to a QUEUE to send in background
                             periodic transfer.
                             Note queue is linked via UX_TRANSFER::ux_transfer_request_next_transfer_request,
                             when issued transfer request list is accepted as input.  */
        /* ......  */
#else
        /* For Standalone mode.  */
        /* Must be non-blocking;
         * in start state: start background transfer and return UX_STATE_NEXT;
         * in pending state: return UX_STATE_WAIT;
         * in done state: confirm transfer fields including completion code are updated and return UX_STATE_NEXT.
         * Note the inputs for this mode is same as in RTOS mode for different request types.  */
        /* ......  */
#endif
        break;

    case UX_HCD_TRANSFER_ABORT:
        /* TODO: Lower level transfer request abort.  */
        /*       STATUS _entry(hcd,UX_HCD_TRANSFER_ABORT,(UX_TRANSFER*)transfer).  */
        /* ......  */
        break;

    case UX_HCD_CREATE_ENDPOINT:
        /* TODO: Lower level endpoint create.  */
        /*       STATUS _entry(hcd,UX_HCD_CREATE_ENDPOINT,(UX_ENDPOINT*)endpoint).  */
        /* ......  */
        break;

    case UX_HCD_DESTROY_ENDPOINT:
        /* TODO: Lower level endpoint destroy.  */
        /*       STATUS _entry(hcd,UX_HCD_DESTROY_ENDPOINT,(UX_ENDPOINT*)endpoint).  */
        /* ......  */
        break;

    case UX_HCD_RESET_ENDPOINT:
        /* TODO: Lower level halted endpoint reset.  */
        /*       STATUS _entry(hcd,UX_HCD_RESET_ENDPOINT,(UX_ENDPOINT*)endpoint).  */
        /* Note ClearEndpointFeature request should be sent to device to clear halted state on device side.  */
        /* ......  */
        break;

    case UX_HCD_PROCESS_DONE_QUEUE: /* Named UX_HCD_TASKS_RUN for standalone mode.  */

        /* TODO: Lower level request queue/background tasks process.  */
        /*       STATUS _entry(hcd,UX_HCD_PROCESS_DONE_QUEUE,not_used).  */
        /* In RTOS mode, the entry is called in a HCD processing thread, which
         * sleeps until hcd -> ux_hcd_thread_signal is larger than zero and
         * _ux_system_host -> ux_system_host_hcd_semaphore is put.
         * In standalone mode, the entry is invoked by host stack as background
         * task to run background state machine, it must be non-blocking.
         */
        /* ......  */
        break;

    case UX_HCD_UNINITIALIZE:

        /* TODO: Lower level uninitialize, executed while unregister the HCD.  */
        /*       STATUS _entry(hcd,UX_HCD_UNINITIALIZE,not_used).  */
        /* It's optional, if the application needs a way to deinitialize USB,
           completely free resources and uninitialize hardware. */
        /* ......  */
        break;

    default:

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_FUNCTION_NOT_SUPPORTED);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        /* Unknown request, return an error.  */
        status =  UX_FUNCTION_NOT_SUPPORTED;
        break;

    }

    /* Return completion status.  */
    return(status);
}
```

## Other considerations

### HCD background thread in RTOS mode

With RTOS support, a thread is created in host stack to process HCD background tasks.
It waits `_ux_system_host -> ux_system_host_hcd_semaphore` and `UX_HCD::ux_hcd_thread_signal`, to issue command `UX_HCD_PROCESS_DONE_QUEUE` on specific HCD.

E.g., for a HCD based on STM32 HAL, to process SOF signal in background task, the code could be:
```c
void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{
    /* ...... */

    /* Signals HCD thread to process done queue.  */
    hcd -> ux_hcd_thread_signal++;
    _ux_host_semaphore_put(&_ux_system_host -> ux_system_host_hcd_semaphore);

    /* ...... */
}
UINT  _ux_hcd_stm32_entry(UX_HCD *hcd, UINT function, VOID *parameter)
{
    /* ...... */
    switch(function)
    {

    /* ...... */

    case UX_HCD_PROCESS_DONE_QUEUE:

        /* Process periodic queue.  */
        _ux_hcd_stm32_periodic_schedule(hcd_stm32);

    /* ...... */
    }

    /* ...... */
}
```

### DMA/CACHE safe of transfer request buffer (optional)

When data cache is enabled in system, if DMA is used for data transfer, the physical address and cache may need synchronization. In USBX the buffer for endpoint transfer is always allocated from "cache safe" memory pool to avoid such issue. It's also possible to implement HCD transfer without DMA or with cache invalidate/flush to remove the "cache safe" pool.

### Isochronous transfer request queue support (optional)

Usually isochronous transfer is not supported unless it's planned to support devices like audio, video, etc. When adding isochronous support, request queue must be supported for it (but not for other types of transfer). In this case transfer queue linked by `UX_TRANSFER::ux_transfer_request_next_transfer_request` can be processed, to prepare multiple request buffers in one function call.


### Split transfer support (optional)

Usually split transfer is not supported unless it's planned to support high speed hub with full/low speed devices on its downstream port. In this case, the split transfer is sent to hub instead of to full/low speed device directly.
