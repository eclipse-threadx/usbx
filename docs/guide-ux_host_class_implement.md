# Overview

This guide is about how to implement host class modules for specific host classes/functions in USBX.

# Host Class Layer (ux_host_class)

The host class layer is the class layer above host stack layer.
It provides a unique way for different classes/functions to communicate with USBX host stack,
and provides class/function specific APIs for the application usages.

## USBX Host Class

The USBX host stack accesses host class/function modules through a unique struct of `UX_HOST_CLASS`.
It's the container for specific class modules. It's allocated on USBX host stack initialization, and can be referenced as `_ux_system_host -> ux_system_host_class_array`.

## USBX Host Class Module for specific class/function

For different USB class/function the host class struct is linked to specific class entry function, called by USBX host stack.
The actual implements then handles the commands from USBX host stack, to:
* query if the class/function is supported;
* activate/deactivate the class/function;
* destroy the class/function anyway.

The host class module may also provide application APIs and/or callbacks to perform class specific data/control exchange with application.

### USBX Host Class Entry for Host Stack

The host class/function module must provide an entry function. It's a parameter for `ux_host_stack_class_register` to register the class module on host stack. It dispatches commands from USBX host stack for standard enumeration actions.

The prototype for the function is:
```c
UINT  _ux_host_class_xxxxx_entry(UX_HOST_CLASS_COMMAND *command);
```

The command data is encapsulated in a struct of `UX_HOST_CLASS_COMMAND`, where `ux_host_class_command_request` holds the host stack command to process and others are parameters for the command, the possible commands are:

| _usage                             | _request                                         | _container                   | _instance | _class_ptr       | _pid/_vid | _class/_subclass/_protocol | _iad_class/_iad_subclass/_iad_protocol | Description                                                                                                                                                                        |
| ---------------------------------- | ------------------------------------------------ | ---------------------------- | --------- | ---------------- | --------- | -------------------------- | -------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| UX_HOST_CLASS_COMMAND_USAGE_PIDVID | UX_HOST_CLASS_COMMAND_QUERY                      | (UX_DEVICE*)                 | not used  | not used         | ULONG     | not used                   | not used                               | Query if class module want to own the device by its idVendor/idProduct in device descriptor                                                                                        |
| UX_HOST_CLASS_COMMAND_USAGE_DCSP   | UX_HOST_CLASS_COMMAND_QUERY                      | (UX_DEVICE*)                 | not used  | not used         | ULONG     | ULONG                      | not used                               | Query if class module want to own the device by its bDeviceClass/bDeviceSubClass/bDeviceProtocol in device descriptor, other information can also be referenced                    |
| UX_HOST_CLASS_COMMAND_USAGE_CSP    | UX_HOST_CLASS_COMMAND_QUERY                      | (UX_INTERFACE*)              | not used  | not used         | ULONG     | ULONG                      | ULONG                                  | Query if class module want to own the interface by its bInterfaceClass/bInterfaceSubClass/bInterfaceProtocol in configuration descriptor, other information can also be referenced |
| UX_HOST_CLASS_COMMAND_USAGE_DCSP   | UX_HOST_CLASS_COMMAND_ACTIVATE/_ACTIVATE_START   | (UX_DEVICE*)                 | not used  | (UX_HOST_CLASS*) | ULONG     | ULONG                      | not used                               | Activate/start activating a new device class instance                                                                                                                              |
| UX_HOST_CLASS_COMMAND_USAGE_CSP    | UX_HOST_CLASS_COMMAND_ACTIVATE/_ACTIVATE_START   | (UX_INTERFACE*)              | not used  | (UX_HOST_CLASS*) | ULONG     | ULONG                      | ULONG                                  | Activate/start activating a new interface class instance                                                                                                                           |
| not used                           | UX_HOST_CLASS_COMMAND_ACTIVATE_WAIT (standalone) | (UX_DEVICE*)/(UX_INTERFACE*) | (VOID*)   | (UX_HOST_CLASS*) | not used  | not used                   | not used                               | Standalone mode only, run device/interface class module instance activating state machine                                                                                          |
| not used                           | UX_HOST_CLASS_COMMAND_DEACTIVATE                 | not used                     | (VOID*)   |                  | not used  | not used                   | not used                               | Deactivate the specific instance (on device/interface)                                                                                                                             |
| not used                           | UX_HOST_CLASS_COMMAND_DESTROY (optional)         | not used                     | not used  | (UX_HOST_CLASS*) | not used  | not used                   | not used                               | Destroy instance on class unregistering                                                                                                                                            |



The function entry implement example is as following:

```c
/**************************************************************************/
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is the entry point of the XXXXX class. It will be     */
/*    called by the USBX stack enumeration module when there is a new     */
/*    device on the bus or when there is a device extraction.             */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    command                               Pointer to class command      */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Host Stack                                                   */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_xxxxx_entry(UX_HOST_CLASS_COMMAND *command)
{

UINT        status;


    /* The command request will tell us we need to do here, either a enumeration
       query, an activation or a deactivation, etc.  */
    switch (command -> ux_host_class_command_request)
    {

    case UX_HOST_CLASS_COMMAND_QUERY:

        /* TODO: check if the module want to own the device/interface and activate instance later. */
        if (/* take the ownership.  */)
            return(UX_SUCCESS);
        else
            return(UX_NO_CLASS_MATCH);

    case UX_HOST_CLASS_COMMAND_ACTIVATE:

        /* TODO: activate the host class/function instance.  */
        /* The activate command is used when the device inserted has found a parent and is ready to complete the enumeration.  */
        /* The instance for the device/interface class is created and appended to class instance list.  */
        /* Mandatory endpoints have to be mounted and the class thread needs to be activated if needed.  */
        /* If instance activation is success and there is notification callback, notify application.  */
        /* ......  */
        return(status);

#if defined(UX_HOST_STANDALONE)
    case UX_HOST_CLASS_COMMAND_ACTIVATE_WAIT:

        /* TODO: run activating state machine for the class on the instance.  */
        /* ......  */
        return(status);
#endif

    case UX_HOST_CLASS_COMMAND_DEACTIVATE:

        /* TODO: deactivate the device class/function instance (optional).  */
        /* The deactivate command is used when the device has been extracted either directly or when its parents has been extracted.  */
        /* The instance endpoints have to be dismounted and the class thread canceled.  */
        /* If there is notification callback, notify application.  */
        /* The instance is removed from class instance list and freed.  */
        /* ......  */
        return(status);

    case UX_HOST_CLASS_COMMAND_DESTROY:

        /* TODO: destroy the instance on class unregistering.  */
        /* Terminate pending transfers and threads.  */
        /* Free/delete allocated resources.  */
        /* Remove the instance from class instance list and free it.  */
        /* ......  */
        return(status);

    default:

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        /* Return error status.  */
        return(UX_FUNCTION_NOT_SUPPORTED);
    }
}
```

#### UX_HOST_CLASS_COMMAND_QUERY

##### Device query

  The query is performed after device descriptor is get from device. If the query is success the host stack pass ownership to class module to let class control the next enumeration steps.

  * Example of checking device descriptor VID/PID:
    ```c
        if (command -> ux_host_class_command_usage == UX_HOST_CLASS_COMMAND_USAGE_PIDVID &&
            command -> ux_host_class_command_pid == UX_DEVICE_CLASS_XXXXX_PID &&
            command -> ux_host_class_command_vid == UX_DEVICE_CLASS_XXXXX_VID)
            return(UX_SUCCESS);
        else
            return(UX_NO_CLASS_MATCH);
    ```
  * Example of checking device descriptor class/subclass/protocol:
    ```c
        if (command -> ux_host_class_command_usage == UX_HOST_CLASS_COMMAND_USAGE_DCSP &&
            command -> ux_host_class_command_class == UX_DEVICE_CLASS_XXXXX_CLASS &&
            command -> ux_host_class_command_subclass == UX_DEVICE_CLASS_XXXXX_SUBCLASS &&
            command -> ux_host_class_command_protocol == UX_DEVICE_CLASS_XXXXX_PROTOCOL)
            return(UX_SUCCESS);
        else
            return(UX_NO_CLASS_MATCH);
    ```

##### Interface query

  The query is performed after configuration descriptor is get from device. The host stack goes through interfaces in the configuration to query class modules. The host stack continues enumeration to do set configuration and issue command on modules linked to interfaces then.

  * Example of checking a interface descriptor class/subclass/protocol:
    ```c
        if (command -> ux_host_class_command_usage == UX_HOST_CLASS_COMMAND_USAGE_CSP &&
            command -> ux_host_class_command_class == UX_DEVICE_CLASS_XXXXX_CLASS &&
            command -> ux_host_class_command_subclass == UX_DEVICE_CLASS_XXXXX_SUBCLASS &&
            command -> ux_host_class_command_protocol == UX_DEVICE_CLASS_XXXXX_PROTOCOL)
            return(UX_SUCCESS);
        else
            return(UX_NO_CLASS_MATCH);
    ```


#### UX_HOST_CLASS_COMMAND_ACTIVATE

According to query results, the activations can be different:
* Device query is success: the class module is device owner to finish enumeration;
* Interface query is success: the class module is interface owner to execute activation command from host stack to do enumeration steps.

##### Activate implement for device owner

The activate command is invoked after a device report its device descriptor. The owner must continue enumeration, to process configuration descriptor and select configuration in activation.

The activate command implement example for a device owner is as following:

```c
    /* ......  */

    /* Get the device that owns this instance.  */
    device =  (UX_DEVICE *) command -> ux_host_class_command_container;

    /* Allocate the memory for XXXXX instance.  */
    xxxxx =  (UX_HOST_CLASS_XXXXX *) _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HOST_CLASS_XXXXX));
    if (xxxxx == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Store the instance in the device container, this is for the USBX stack when it needs to invoke the class for deactivation.  */
    device -> ux_device_class_instance =  (VOID *) xxxxx;

    /* Add this instance to class instance list.  */
    _ux_host_stack_class_instance_create(xxxxx -> ux_host_class_xxxxx_class, (VOID *) xxxxx);

    /* TODO: Configure the device.
     *       - Get configuration descriptor from device (through _ux_host_stack_device_configuration_get)
     *       - Get other descriptors (class/function specific)
     *       - Initialize the instance fields according to device reported things
     *       - Select expected configuation on device (_ux_host_stack_device_configuration_select)
     *       - Select expected interface in configuration (optional, if alternate settings supported)
     *       - Issue sequence of requests to device to initialize it
     *       - etc.
     */
    /* ......  */

    /* TODO: class/function specific activations, such as:
     *       - Activate class and function specific threads.
     *       - Activate function specific endpoints.
     *       - Other class/function specific preparations.
     *         - Issue requests to initialize device functionalities;
     *         - etc.
     */
    /* ......  */

    /* If all is fine and the device is mounted, we need to inform the application
    if a function has been programmed in the system structure. */
    if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
    {

        /* Call system change function.  */
        _ux_system_host ->  ux_system_host_change_function(UX_DEVICE_INSERTION, xxxxx -> ux_host_class_xxxxx_class, (VOID *) xxxxx);
    }

    /* ......  */
```

Check existing HUB or ASIX implement for more detailed references.

##### Activate implement for interface owner

The activate command is invoked after a device is configured. In this case host stack has built device configuration tree. The owner uses host stack enumerated things to do class/function specific activation.

The activate command implement example for a interface owner is as following:

```c
    /* ......  */

    /* Get the interface that owns this instance.  */
    interface_ptr =  (UX_INTERFACE *) command -> ux_host_class_command_container;

    /* Allocate the memory for XXXXX instance.  */
    xxxxx =  (UX_HOST_CLASS_XXXXX *) _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HOST_CLASS_XXXXX));
    if (xxxxx == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* ......  */

    /* This instance of the interface must also be stored in the interface container.  */
    interface_ptr -> ux_interface_class_instance =  (VOID *) xxxxxx;

    /* Add this instance to class instance list.  */
    _ux_host_stack_class_instance_create(xxxxxx -> ux_host_class_xxxxxx_class, (VOID *) xxxxxx);

    /* TODO: class/function specific activations, such as:
     *       - Activate class and function specific threads.
     *       - Activate function specific endpoints.
     *       - Other class/function specific preparations.
     *         - Issue requests to initialize function module functionalities;
     *         - etc.
     */
    /* ......  */

    /* If all is fine and the instance is mounted, we need to inform the application
    if a function has been programmed in the system structure. */
    if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
    {

        /* Call system change function.  */
        _ux_system_host ->  ux_system_host_change_function(UX_DEVICE_INSERTION, xxxxx -> ux_host_class_xxxxx_class, (VOID *) xxxxx);
    }

    /* ......  */
```

Check existing CDC_ACM, Storage or HID implement for more detailed references.

#### UX_HOST_CLASS_COMMAND_DEACTIVATE

The deactivate command implement example is as following:

```c
UINT  _ux_host_class_xxxxx_deactivate(UX_HOST_CLASS_COMMAND *command)
{
    /* ......  */

    /* Get the instance to the class.  */
    xxxxx =  (UX_HOST_CLASS_XXXXX *) command -> ux_host_class_command_instance;

    /* TODO: class/function specific deactivations, such as:
     *       - Cancel pending transfers;
     *       - Free allocated resources;
     *       - etc.
     */
    /* ......  */

    /* Unlink the instance from class instance list.  */
    _ux_host_stack_class_instance_destroy(xxxxx -> ux_host_class_xxxxx_class, (VOID *) xxxxx);

    /* Before we free the instance, we need to inform the application that the instance is removed.  */
    if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
    {
        
        /* Inform the application the instance is removed.  */
        _ux_system_host -> ux_system_host_change_function(UX_DEVICE_REMOVAL, xxxxx -> ux_host_class_xxxxx_class, (VOID *) xxxxx);
    }

    /* Free the memory block used by the class instance.  */
    _ux_utility_memory_free(xxxxx);

    return(UX_SUCCESS);
}
```

## Other considerations

### Class struct for instance list management

The first two fields for class module is kept consistent for the system to manage the class instances list in class container.

```c
typedef struct UX_HOST_CLASS_XXXXX_STRUCT
{
    struct UX_HOST_CLASS_XXXXX_STRUCT
                   *ux_host_class_xxxxx_next_instance;
    UX_HOST_CLASS  *ux_host_class_xxxxx_class;

    /* ......  */
} UX_HOST_CLASS_XXXXX;
```

With these two fields, the function `_ux_host_stack_class_instance_create` can be used to link instance to class instance list and `_ux_host_stack_class_instance_destroy` can be used to unlink it.

By adding instance to class instance list, instance can be referenced in application through "class_get" and "class_instance_get", like following:

```c
    /* Get a registered class.  */
    ux_host_stack_class_get(_ux_system_host_class_cdc_acm_name, &host_class);
    /* Get the first class instance in instance list.  */
    ux_host_stack_class_instance_get(host_class, 0, &class_instance);
```

Also the class instance can be verified by `_ux_host_stack_class_instance_verify`. It's useful for APIs facing to application to confirm if right instance pointer is assigned.

### Device control endpoint 0 transfers

For each attached device, the control endpoint 0 is available with `UX_DEVICE` creation, as `UX_DEVICE::ux_device_control_endpoint`. To avoid multiple access conflicts from different interfaces class modules on that device, `UX_DEVICE::ux_device_protection_semaphore` can be used, e.g.,:

```c
    /* ......  */

    /* We need to get the default control endpoint transfer request pointer.  */
    control_endpoint =  &xxxxx -> ux_host_class_xxxxx_device -> ux_device_control_endpoint;
    transfer_request =  &control_endpoint -> ux_endpoint_transfer_request;

    /* Protect the control endpoint semaphore here.  It will be unprotected in the
       transfer request function.  */
    status =  _ux_host_semaphore_get(&xxxxx -> ux_host_class_xxxxx_device -> ux_device_protection_semaphore, UX_WAIT_FOREVER);

    /* Check for status.  */
    if (status != UX_SUCCESS)
    {

        /* Something went wrong. */
        /* ......  */
        return(status);
    }

    /* Create a transfer request for the request.  */
    transfer_request -> ux_transfer_request_data_pointer =      data_stage_data_ptr;
    transfer_request -> ux_transfer_request_requested_length =  data_stage_data_length;
    transfer_request -> ux_transfer_request_type =              bRequestType;
    transfer_request -> ux_transfer_request_value =             wValue;
    transfer_request -> ux_transfer_request_index =             wIndex;
    transfer_request -> ux_transfer_request_function =          bRequest;

    /* Issue the transfer request.  */
    status =  _ux_host_stack_transfer_request(transfer_request);

    /* ......  */
```

### Device non-control transfers

#### Thread blocking read/write

If transfer callback is not used, in HCD lower level implement, the transfer semaphore is put when transfer ends, so in class implement semaphore wait is performed to wait transfer end. E.g.,

```c
    /* ......  */

    /* Get the pointer to the bulk in endpoint in the transfer_request.  */
    transfer_request =  &xxxxx -> ux_host_class_xxxxx_bulk_in_endpoint -> ux_endpoint_transfer_request;

    /* Initialize the transfer request.  */
    transfer_request -> ux_transfer_request_data_pointer =      data_pointer;
    transfer_request -> ux_transfer_request_requested_length =  transfer_request_length;
    
    /* Perform the transfer.  */
    status =  _ux_host_stack_transfer_request(transfer_request);

    /* Wait for the completion of the transfer_request.  */
    status =  _ux_host_semaphore_get(&transfer_request -> ux_transfer_request_semaphore,
                                        transfer_request -> ux_transfer_request_timeout_value);

    /* If the semaphore did not succeed we probably have a time out.  */
    if (status != UX_SUCCESS)
    {
        /* All transfers pending need to abort. There may have been a partial transfer.  */
        _ux_host_stack_transfer_request_abort(transfer_request);

        /* ......  */
    }
    /* ......  */
```

Refer to read/write implement in host class CDC-ACM for more details.


#### Using transfer callback

If transfer callback is assigned, in HCD lower level implement, the callback is invoked when transfer ends. E.g., to start a transfer with callback for periodic report polling:

```c

    /* Get the address of the HID associated with the interrupt endpoint.  */
    transfer_request =  &xxxxx -> ux_host_class_xxxxx_interrupt_endpoint -> ux_endpoint_transfer_request;

    /* Set the class instance owner.  */
    transfer_request -> ux_transfer_request_class_instance = xxxxx;

    /* Set the transfer callback function.  */
    transfer_request -> ux_transfer_request_completion_function = _ux_host_class_xxxxx_transfer_completed;

    /* The transfer on the interrupt endpoint can be started.  */
    _ux_host_stack_transfer_request(transfer_request);
```

In callback restart the transfer to keep periodic polling:

```c
void _ux_host_class_xxxxx_transfer_completed(UX_TRANSFER* transfer_request)
{
    /* ......  */

    /* Get the class instance for this transfer request.  */
    xxxxx =  (UX_HOST_CLASS_XXXXX *) transfer_request -> ux_transfer_request_class_instance;

    /* Check the state of the transfer.  If there is an error, we do not proceed with this report.  */
    if (transfer_request -> ux_transfer_request_completion_code != UX_SUCCESS)
    {
        /* ......  */
    }
    /* ......  */

    /* Reactivate the HID interrupt pipe.  */
    _ux_host_stack_transfer_request(transfer_request);
}
```

Refer to host HID periodic report polling implement for more details.

