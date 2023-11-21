
#include "ux_api.h"
#include "ux_device_class_dummy.h"
#include "ux_device_stack.h"

UCHAR _ux_device_class_dummy_name[] = "ux_device_class_dummy";

static UINT _ux_device_class_dummy_initialize(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS                  *class;
UX_DEVICE_CLASS_DUMMY           *dummy;
UX_DEVICE_CLASS_DUMMY_PARAMETER *dummy_parameter;


    /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;

    /* Get pointer to the application parameters.  */
    dummy_parameter = (UX_DEVICE_CLASS_DUMMY_PARAMETER *)command -> ux_slave_class_command_parameter;

    /* Create dummy class instance.  */
    dummy = (UX_DEVICE_CLASS_DUMMY *)_ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_DEVICE_CLASS_DUMMY));
    if (dummy == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Save the address of the Dummy instance inside the Dummy container.  */
    class -> ux_slave_class_instance = (VOID *) dummy;

    /* Link to class instance.  */
    dummy -> ux_device_class_dummy_class = class;

    /* Save parameters.  */
    _ux_utility_memory_copy(&dummy -> ux_device_class_dummy_callbacks,
                            &dummy_parameter -> ux_device_class_dummy_parameter_callbacks,
                            sizeof(UX_DEVICE_CLASS_DUMMY_CALLBACKS));

    if (dummy->ux_device_class_dummy_callbacks.ux_device_class_dummy_initialize)
        dummy->ux_device_class_dummy_callbacks.ux_device_class_dummy_initialize(dummy);

    return(UX_SUCCESS);
}

static UINT _ux_device_class_dummy_uninitialize(UX_SLAVE_CLASS_COMMAND *command)
{

UX_DEVICE_CLASS_DUMMY       *dummy = NULL;
UX_SLAVE_CLASS              *class;


    /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    dummy = (UX_DEVICE_CLASS_DUMMY *) class -> ux_slave_class_instance;

    if (dummy->ux_device_class_dummy_callbacks.ux_device_class_dummy_uninitialize)
        dummy->ux_device_class_dummy_callbacks.ux_device_class_dummy_uninitialize(dummy);

    if (dummy != UX_NULL)
        _ux_utility_memory_free(dummy);
    return(UX_SUCCESS);
}

static UINT _ux_device_class_dummy_query(UX_SLAVE_CLASS_COMMAND *command)
{

UINT        status = UX_SUCCESS;


    return(status);
}

static UINT _ux_device_class_dummy_activate(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS                          *class;
UX_SLAVE_DEVICE                         *device;
UX_SLAVE_INTERFACE                      *interface;
UX_DEVICE_CLASS_DUMMY                   *dummy;


    /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    dummy = (UX_DEVICE_CLASS_DUMMY *) class -> ux_slave_class_instance;

    /* Get the interface that owns this instance.  */
    interface =  (UX_SLAVE_INTERFACE  *) command -> ux_slave_class_command_interface;
    dummy -> ux_device_class_dummy_interface = interface;

    /* Get the device instance.  */
    device = &_ux_system_slave -> ux_system_slave_device;
    dummy -> ux_device_class_dummy_device = device;

    /* If there is a activate function call it.  */
    if (dummy -> ux_device_class_dummy_callbacks.ux_device_class_dummy_instance_activate)
        dummy -> ux_device_class_dummy_callbacks.ux_device_class_dummy_instance_activate(dummy);
    return(UX_SUCCESS);
}

static UINT _ux_device_class_dummy_change(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS                          *class;
UX_SLAVE_INTERFACE                      *interface;
UX_DEVICE_CLASS_DUMMY                   *dummy;


    /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    dummy = (UX_DEVICE_CLASS_DUMMY *) class -> ux_slave_class_instance;

    /* Get the interface that owns this instance.  */
    interface =  (UX_SLAVE_INTERFACE  *) command -> ux_slave_class_command_interface;

    /* Update the interface.  */
    dummy -> ux_device_class_dummy_interface = interface;

    /* Invoke change callback.  */
    if (dummy -> ux_device_class_dummy_callbacks.ux_device_class_dummy_change)
        dummy -> ux_device_class_dummy_callbacks.ux_device_class_dummy_change(dummy);
    return(UX_SUCCESS);
}

static UINT _ux_device_class_dummy_deactivate(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS                          *class;
UX_DEVICE_CLASS_DUMMY                   *dummy;


    /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    dummy = (UX_DEVICE_CLASS_DUMMY *) class -> ux_slave_class_instance;

    /* If there is a deactivate function call it.  */
    if (dummy -> ux_device_class_dummy_callbacks.ux_device_class_dummy_instance_deactivate)
        dummy -> ux_device_class_dummy_callbacks.ux_device_class_dummy_instance_deactivate(dummy);
    return(UX_SUCCESS);
}

static UINT _ux_device_class_dummy_request(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS                          *class;
UX_DEVICE_CLASS_DUMMY                   *dummy;
UX_SLAVE_DEVICE                         *device;
UX_SLAVE_TRANSFER                       *transfer_request;


    /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;

    /* Get the audio instance from this class container.  */
    dummy =  (UX_DEVICE_CLASS_DUMMY *) class -> ux_slave_class_instance;

    /* Get the pointer to the device.  */
    device =  dummy -> ux_device_class_dummy_device;

    /* Get the pointer to the transfer request associated with the control endpoint.  */
    transfer_request =  &device -> ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

    /* Invoke callback.  */
    if (dummy -> ux_device_class_dummy_callbacks.ux_device_class_dummy_control_request)
    {
        dummy -> ux_device_class_dummy_callbacks.ux_device_class_dummy_control_request(dummy, transfer_request);
        return(UX_SUCCESS);
    }

    /* By default, not handled, stall the endpoint.  */
    _ux_device_stack_endpoint_stall(&device -> ux_slave_device_control_endpoint);
    return(UX_SUCCESS);
}

UINT  _ux_device_class_dummy_entry(UX_SLAVE_CLASS_COMMAND *command)
{

UINT        status;


    switch(command -> ux_slave_class_command_request)
    {

    case UX_SLAVE_CLASS_COMMAND_INITIALIZE:
        status = _ux_device_class_dummy_initialize(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_UNINITIALIZE:
        status = _ux_device_class_dummy_uninitialize(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_QUERY:
        status = _ux_device_class_dummy_query(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_ACTIVATE:
        status = _ux_device_class_dummy_activate(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_CHANGE:
        status = _ux_device_class_dummy_change(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_DEACTIVATE:
        status = _ux_device_class_dummy_deactivate(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_REQUEST:
        status = _ux_device_class_dummy_request(command);
        break;

    default:
        status = UX_FUNCTION_NOT_SUPPORTED;
        break;

    }
    return(status);
}

VOID *_ux_device_class_dummy_get_arg(UX_DEVICE_CLASS_DUMMY *dummy)
{
    return dummy -> ux_device_class_dummy_callbacks.ux_device_class_dummy_arg;
}

ULONG _ux_device_class_dummy_get_max_packet_size(UX_DEVICE_CLASS_DUMMY *dummy, UCHAR endpoint_address)
{

UX_SLAVE_ENDPOINT  *endpoint = _ux_device_class_dummy_get_endpoint(dummy, endpoint_address);


    if (endpoint == UX_NULL)
        return 0;
    return endpoint -> ux_slave_endpoint_descriptor.wMaxPacketSize;
}

UX_SLAVE_ENDPOINT *_ux_device_class_dummy_get_endpoint(UX_DEVICE_CLASS_DUMMY *dummy, UCHAR endpoint_address)
{

UX_SLAVE_ENDPOINT  *endpoint;
UX_SLAVE_INTERFACE *interface;

    if (dummy == UX_NULL)
        return(UX_NULL);
    if (endpoint_address == 0 || endpoint_address == 0x80)
        return(&dummy -> ux_device_class_dummy_device -> ux_slave_device_control_endpoint);

    interface = dummy -> ux_device_class_dummy_interface;
    if (interface == UX_NULL)
        return((UX_SLAVE_ENDPOINT *)UX_NULL);

    endpoint = interface -> ux_slave_interface_first_endpoint;
    while(endpoint)
    {
        if (endpoint -> ux_slave_endpoint_descriptor.bEndpointAddress == endpoint_address)
            break;
        endpoint = endpoint -> ux_slave_endpoint_next_endpoint;
    }
    return endpoint;
}

UX_SLAVE_TRANSFER *_ux_device_class_dummy_get_transfer_request(UX_DEVICE_CLASS_DUMMY *dummy, UCHAR endpoint_address)
{

UX_SLAVE_ENDPOINT *endpoint = _ux_device_class_dummy_get_endpoint(dummy, endpoint_address);


    return &endpoint -> ux_slave_endpoint_transfer_request;
}

UINT  _ux_device_class_dummy_transfer(UX_DEVICE_CLASS_DUMMY *dummy, UCHAR endpoint_address, UCHAR *buffer, ULONG length, ULONG *actual_length)
{

UINT               status;
UX_SLAVE_TRANSFER *transfer_request = _ux_device_class_dummy_get_transfer_request(dummy, endpoint_address);


    if (transfer_request == UX_NULL)
        return(UX_ERROR);

    if (length > UX_SLAVE_REQUEST_DATA_MAX_LENGTH)
        length = UX_SLAVE_REQUEST_DATA_MAX_LENGTH;

    if (endpoint_address & 0x80) /* Device to host */
    {
        if (length)
            _ux_utility_memory_copy(transfer_request -> ux_slave_transfer_request_data_pointer,
                                    buffer, length);

        status = _ux_device_stack_transfer_request(transfer_request, length, length);
    }

    else /* Host to Device */
    {
        status = _ux_device_stack_transfer_request(transfer_request, length, length);

        if (status == UX_SUCCESS && buffer != UX_NULL)
        {
            if (length)
                _ux_utility_memory_copy(buffer,
                                        transfer_request -> ux_slave_transfer_request_data_pointer,
                                        length);
        }
    }

    if (actual_length)
        *actual_length = transfer_request -> ux_slave_transfer_request_actual_length;

    return(status);
}
