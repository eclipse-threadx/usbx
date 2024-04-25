# Overview

This guide is about how to implement device class modules for specific device classes/functions in USBX.

# Device Class Layer (ux_device_class)

The device class layer is the class layer above device stack layer.
It provides a unique way for different classes/functions to communicate with USBX device stack,
and provides class/function specific APIs for the application usages.

## USBX Device Class

The USBX device stack accesses device class/function modules through a unique struct of `UX_SLAVE_CLASS`.
It's the container for specific (local) class modules. It's allocated on USBX device stack initialization, and can be referenced as `_ux_system_slave -> ux_system_slave_class_array`.

## USBX Device Class Module for specific class/function

For different USB class/function the device class struct is linked to specific class entry function, called by USBX device stack.
The actual implements then handles the commands from USBX device stack, to:
* initialize/uninitialize the class/function;
* query if the class/function is supported;
* activate/deactivate the class/function;
* process class specific requests received;
* handle the interface alternate setting change.

The device class module may also provide application APIs and/or callbacks to perform class specific data/control exchange with application.

### USBX Device Class Entry for Device Stack

The device class/function module must provide an entry function. It's a parameter for `ux_device_stack_class_register` to register the class module on device stack and do initialization. It dispatches commands from USBX device stack for standard and class specific host requests handling.

The prototype for the function is:
```c
UINT  _ux_device_class_xxxxx_entry(UX_SLAVE_CLASS_COMMAND *command);
```

The command data is encapsulated in a struct of `UX_SLAVE_CLASS_COMMAND`, where `ux_slave_class_command_request` holds the device stack command to process and others are parameters for the command, the possible commands are:

| _request                                       | _interface            | _class/_subclass/_protocol | _class_ptr        | _parameter   | Description                                                 |
| ---------------------------------------------- | --------------------- | -------------------------- | ----------------- | ------------ | ----------------------------------------------------------- |
| UX_SLAVE_CLASS_COMMAND_INITIALIZE              | not used              | not used                   | (UX_SLAVE_CLASS*) | (VOID*)param | Initialize class/function module instance                   |
| UX_SLAVE_CLASS_COMMAND_QUERY                   | (UX_SLAVE_INTERFACE*) | ULONG                      | (UX_SLAVE_CLASS*) | not used     | Query if class/subclass/protocol is supported               |
| UX_SLAVE_CLASS_COMMAND_ACTIVATE                | (UX_SLAVE_INTERFACE*) | ULONG                      | (UX_SLAVE_CLASS*) | not used     | Activate local class/function instance                      |
| UX_SLAVE_CLASS_COMMAND_DEACTIVATE              | (UX_SLAVE_INTERFACE*) | not used                   | not used          | not used     | Deactivate notification                                     |
| UX_SLAVE_CLASS_COMMAND_REQUEST (optional)      | not used              | not used                   | (UX_SLAVE_CLASS*) | not used     | Handle class/vendor requests from host (if needed)          |
| UX_SLAVE_CLASS_COMMAND_CHANGE (optional)       | (UX_SLAVE_INTERFACE*) | not used                   | (UX_SLAVE_CLASS*) | not used     | Interface alternate setting change notification (if needed) |
| UX_SLAVE_CLASS_COMMAND_UNINITIALIZE (optional) | not used              | not used                   | (UX_SLAVE_CLASS*) | not used     | Uninitialize class/function module instance                 |

The function entry implement example is as following:

```c
/**************************************************************************/
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is the entry point of the xxxxx class. It             */
/*    will be called by the device stack when the host has sent specific  */
/*    request and the xxxxx class/function needs to be processed.         */
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
/*    USBX Device Stack                                                   */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_xxxxx_entry(UX_SLAVE_CLASS_COMMAND *command)
{

UINT        status;


    /* The command request will tell us we need to do here, either a enumeration
       query, an activation or a deactivation, etc.  */
    switch (command -> ux_slave_class_command_request)
    {

    case UX_SLAVE_CLASS_COMMAND_INITIALIZE:

        /* TODO: Initialize the device class/function instance.  */
        /* Allocates instance memory and other related resources and initialize them.  */
        /* Link the instance to container so it can be referenced.  */
        /* ......  */
        return(status);

    case UX_SLAVE_CLASS_COMMAND_UNINITIALIZE:

        /* TODO: uninitialize the device class/function instance.  */
        /* Uninitialize, free the resources and release instance memory.  */
        /* ......  */
        return(status);

    case UX_SLAVE_CLASS_COMMAND_QUERY:

        /* TODO: check class and/or subclass and/or protocol to see if it's supported. */
        if (command -> ux_slave_class_command_class == UX_DEVICE_CLASS_XXXXX_CLASS)
            return(UX_SUCCESS);
        else
            return(UX_NO_CLASS_MATCH);

    case UX_SLAVE_CLASS_COMMAND_ACTIVATE:

        /* TODO: activate the device class/function instance.  */
        /* The activate command is used when the host has sent a SET_CONFIGURATION command and this interface has to be mounted.  */
        /* Mandatory endpoints have to be mounted and the class thread needs to be activated if needed.  */
        /* If instance activation is success and there is notification callback, notify application.  */
        /* ......  */
        return(status);

    case UX_SLAVE_CLASS_COMMAND_CHANGE:

        /* TODO: notify that the interface alternate setting has been changed (optional).  */
        /* The change command is used when the host has sent a SET_INTERFACE command
           to go from Alternate Setting 0 to >0 or revert to the default mode (0).  */
        /* ......  */
        return(status);

    case UX_SLAVE_CLASS_COMMAND_DEACTIVATE:

        /* TODO: deactivate the device class/function instance (optional).  */
        /* The deactivate command is used when the device has been extracted.  */
        /* The instance endpoints have to be dismounted and the class thread canceled.  */
        /* If there is notification callback, notify application.  */
        /* ......  */
        return(status);

    case UX_SLAVE_CLASS_COMMAND_REQUEST:

        /* TODO: handle host class/vendor requests (optional).  */
        /* The request command is used when the host sends a command on the control endpoint.  */
        /* ......  */
        return(status);

    default:

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        /* Return an error.  */
        return(UX_FUNCTION_NOT_SUPPORTED);
    }
}
```

#### UX_SLAVE_CLASS_COMMAND_INITIALIZE

The initialize command implement example is as following:

```c
    /* ......  */

    /* Get the pointer to the application parameters for the xxxxx class.  */
    xxxxx_parameter =  command -> ux_slave_class_command_parameter;

    /* Get the class container.  */
    class_ptr =  command -> ux_slave_class_command_class_ptr;

    /* Create an instance of the device xxxxx class.  */
    xxxxx =  _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_SLAVE_CLASS_xxxxx));

    /* Check for successful allocation.  */
    if (xxxxx == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Save the address of the xxxxx instance inside the container.  */
    class_ptr -> ux_slave_class_instance = (VOID *) xxxxx;

    /* TODO: class/function specific initialization.  */
    /* ......  */
```

#### UX_SLAVE_CLASS_COMMAND_QUERY

The query command implement example is as following:

```c
    if (command -> ux_slave_class_command_class == UX_DEVICE_CLASS_XXXXX_CLASS &&
        command -> ux_slave_class_command_subclass == UX_DEVICE_CLASS_XXXXX_SUBCLASS &&
        command -> ux_slave_class_command_protocol == UX_DEVICE_CLASS_XXXXX_PROTOCOL)
        return(UX_SUCCESS);
    else
        return(UX_NO_CLASS_MATCH);
```

#### UX_SLAVE_CLASS_COMMAND_ACTIVATE

The activate command implement example is as following:

```c
    /* ......  */

    /* Get the class container.  */
    class_ptr =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    xxxxx =  (UX_SLAVE_CLASS_xxxxx *) class_ptr -> ux_slave_class_instance;

    /* Get the interface that owns this instance.  */
    interface_ptr =  (UX_SLAVE_INTERFACE  *) command -> ux_slave_class_command_interface;

    /* Store the class instance into the interface.  */
    interface_ptr -> ux_slave_interface_class_instance =  (VOID *)xxxxx;

    /* Now the opposite, store the interface in the class instance.  */
    xxxxx -> ux_slave_class_xxxxx_interface =  interface_ptr;

    /* TODO: class/function specific activations, such as:
     *       - Activate class and function specific threads.
     *       - Activate function specific endpoints.
     *       - Other class/function specific preparations, etc.
     */
    /* ......  */
```

#### UX_SLAVE_CLASS_COMMAND_DEACTIVATE

The deactivate command implement example is as following:

```c
UINT  _ux_device_class_xxxxx_deactivate(UX_SLAVE_CLASS_COMMAND *command)
{
    /* ......  */

    /* Get the class container.  */
    class_ptr =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    xxxxx = (UX_DEVICE_CLASS_XXXXXX *) class_ptr -> ux_slave_class_instance;

    /* TODO: class/function specific deactivations, such as:
     *       - Cancel pending transfers;
     *       - Notify application;
     *       - etc.
     */
    /* ......  */
}
```

#### UX_SLAVE_CLASS_COMMAND_REQUEST

The request command is optional.
It is implemented if the class/function needs to process class/vendor requests from host,
implement example is as following:

```c
    /* ......  */

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* Get the pointer to the transfer request associated with the control endpoint.  */
    transfer_request =  &device -> ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

    /* Get the class container.  */
    class_ptr =  command -> ux_slave_class_command_class_ptr;
    
    /* Get the storage instance from this class container.  */
    xxxxx =  (UX_DEVICE_CLASS_XXXXX *) class_ptr -> ux_slave_class_instance;

    /* TODO: handle setup request from host.  */
    /* ......  */
```

#### UX_SLAVE_CLASS_COMMAND_CHANGE

The change command is optional.
It is implemented if the class/function needs different interface alternate settings.
implement example is as following:

```c
    /* ......  */

    /* Get the class container.  */
    class_ptr =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    xxxxx = (UX_SLAVE_CLASS_XXXXX *) class_ptr -> ux_slave_class_instance;

    /* Get the interface that owns this instance.  */
    interface_ptr =  (UX_SLAVE_INTERFACE  *) command -> ux_slave_class_command_interface;

    /* Check alternate settings.  */
    if (interface_ptr -> ux_slave_interface_descriptor.bAlternateSetting != 0)
    {
        /* TODO: non-zero alternate setting.  */
    }
    else
    {
        /* TODO: zero alternate setting.  */
    }

    /* ......  */
```

#### UX_SLAVE_CLASS_COMMAND_UNINITIALIZE

The uninitialize command is optional.
It is implemented in purpose to free all resources for the class/function support.
implement example is as following:

```c
    /* ......  */
    /* Get the class container.  */
    class_ptr =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    hid = (UX_SLAVE_CLASS_HID *) class_ptr -> ux_slave_class_instance;

    /* TODO: Free resources, like
     *       - Delete threads, semaphores, mutexes, etc.
     *       - Free memories allocated for instance and the instnace memory;
     *       - etc.
     */
    /* ......  */
```

## Other considerations

### Memory management

To avoid memory fragmentation, it's recommended to allocate as much as memories in initialize instead of in activation.

Also if the system does not need USB system deinitialization, it's not necessary to process the command of uninitialize.
