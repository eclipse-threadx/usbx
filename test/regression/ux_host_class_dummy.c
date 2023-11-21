
#include "ux_test.h"
#include "ux_api.h"
#include "ux_host_class_dummy.h"
#include "ux_host_stack.h"

static UINT _ux_host_class_dummy_activate(UX_HOST_CLASS_COMMAND *command);
static UINT _ux_host_class_dummy_deactivate(UX_HOST_CLASS_COMMAND *command);

static UINT _ux_host_class_dummy_device_activate(UX_HOST_CLASS_COMMAND *command);
static UINT _ux_host_class_dummy_device_deactivate(UX_HOST_CLASS_COMMAND *command);

UCHAR _ux_host_class_dummy_name[] = "ux_host_class_dummy";

static UX_HOST_CLASS_DUMMY_QUERY *_ux_host_class_dummy_query_list;

static UCHAR _ux_host_class_dummy_query_reject_unknown = UX_FALSE;

VOID _ux_host_class_dummy_query_reject_unknown_set(UCHAR yes_no)
{
    _ux_host_class_dummy_query_reject_unknown = yes_no;
}

VOID _ux_host_class_dummy_query_list_set(UX_HOST_CLASS_DUMMY_QUERY *query_list)
{
    _ux_host_class_dummy_query_list = query_list;
}

UINT _ux_host_class_dummy_command_query_list_check(UX_HOST_CLASS_COMMAND *command)
{
UX_HOST_CLASS_DUMMY_QUERY       *query;
    if (_ux_host_class_dummy_query_list == UX_NULL)
        return(UX_NO_CLASS_MATCH);
    query = _ux_host_class_dummy_query_list;
    while(query -> ux_host_class_query_on)
    {
        if (query -> ux_host_class_query_entry != 0 &&
            query -> ux_host_class_query_entry !=
                command -> ux_host_class_command_class_ptr -> ux_host_class_entry_function)
        {
            query ++;
            continue;
        }
        if (query -> ux_host_class_query_usage != 0 &&
            query -> ux_host_class_query_usage != command -> ux_host_class_command_usage)
        {
            query ++;
            continue;
        }
        if (query -> ux_host_class_query_vid != 0 &&
            query -> ux_host_class_query_vid != command -> ux_host_class_command_vid)
        {
            query ++;
            continue;
        }
        if (query -> ux_host_class_query_pid != 0 &&
            query -> ux_host_class_query_pid != command -> ux_host_class_command_pid)
        {
            query ++;
            continue;
        }
        if (query -> ux_host_class_query_class != 0 &&
            query -> ux_host_class_query_class != command -> ux_host_class_command_class)
        {
            query ++;
            continue;
        }
        if (query -> ux_host_class_query_subclass != 0 &&
            query -> ux_host_class_query_subclass != command -> ux_host_class_command_subclass)
        {
            query ++;
            continue;
        }
        if (query -> ux_host_class_query_protocol != 0 &&
            query -> ux_host_class_query_protocol != command -> ux_host_class_command_protocol)
        {
            query ++;
            continue;
        }
        if (query -> ux_host_class_query_iad_class != 0 &&
            query -> ux_host_class_query_iad_class != command -> ux_host_class_command_iad_class)
        {
            query ++;
            continue;
        }
        if (query -> ux_host_class_query_iad_subclass != 0 &&
            query -> ux_host_class_query_iad_subclass != command -> ux_host_class_command_iad_subclass)
        {
            query ++;
            continue;
        }
        if (query -> ux_host_class_query_iad_protocol != 0 &&
            query -> ux_host_class_query_iad_protocol != command -> ux_host_class_command_iad_protocol)
        {
            query ++;
            continue;
        }
        return(UX_SUCCESS);
    }
    return(UX_NO_CLASS_MATCH);
}

UINT _ux_host_class_dummy_entry(UX_HOST_CLASS_COMMAND *command)
{

    /* The command request will tell us we need to do here, either a enumeration
       query, an activation or a deactivation.  */
    switch (command -> ux_host_class_command_request)
    {

    case UX_HOST_CLASS_COMMAND_QUERY:
        if (_ux_host_class_dummy_command_query_list_check(command) == UX_SUCCESS)
            return(UX_SUCCESS);

        /* Only for interface : class subclass protocol.  */
        if (command -> ux_host_class_command_usage != UX_HOST_CLASS_COMMAND_USAGE_CSP)
            return(UX_NO_CLASS_MATCH);

        /* Skip undefined or composite.  */
        if (command -> ux_host_class_command_class == 0 ||
            command -> ux_host_class_command_class == 0xFE)
            return(UX_NO_CLASS_MATCH);

        if (!_ux_host_class_dummy_query_reject_unknown)
            return(UX_SUCCESS);
        return(UX_NO_CLASS_MATCH);

    case UX_HOST_CLASS_COMMAND_ACTIVATE:
        return _ux_host_class_dummy_activate(command);

    /* Standalone activate wait support.  */
    case UX_HOST_CLASS_COMMAND_ACTIVATE_WAIT:
        return (UX_STATE_NEXT);

    case UX_HOST_CLASS_COMMAND_DEACTIVATE:
        return _ux_host_class_dummy_deactivate(command);

    default:

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_FUNCTION_NOT_SUPPORTED);
    }
}

UINT _ux_host_class_dummy_activate(UX_HOST_CLASS_COMMAND *command)
{

UX_INTERFACE                *interface;
UX_ENDPOINT                 *endpoint;
UX_HOST_CLASS_DUMMY         *dummy;


    interface =  (UX_INTERFACE *) command -> ux_host_class_command_container;
    dummy = (UX_HOST_CLASS_DUMMY *)_ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HOST_CLASS_DUMMY));
    if (dummy == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    dummy -> ux_host_class_dummy_class = command -> ux_host_class_command_class_ptr;
    dummy -> ux_host_class_dummy_interface = interface;
    dummy -> ux_host_class_dummy_device = UX_NULL;
    interface -> ux_interface_class_instance = (VOID *)dummy;

    _ux_host_stack_class_instance_create(dummy -> ux_host_class_dummy_class, (VOID *)dummy);

    endpoint = interface -> ux_interface_first_endpoint;
    while(endpoint)
    {
        endpoint -> ux_endpoint_transfer_request.ux_transfer_request_type =
                        (endpoint->ux_endpoint_descriptor.bEndpointAddress &
                                                    UX_ENDPOINT_DIRECTION) ?
                                                UX_REQUEST_IN : UX_REQUEST_OUT;
        endpoint -> ux_endpoint_transfer_request.ux_transfer_request_timeout_value =
                                                                UX_WAIT_FOREVER;
        endpoint = endpoint -> ux_endpoint_next_endpoint;
    }

    dummy -> ux_host_class_dummy_state = UX_HOST_CLASS_INSTANCE_LIVE;

    if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
        _ux_system_host ->  ux_system_host_change_function(UX_DEVICE_INSERTION, dummy -> ux_host_class_dummy_class, (VOID *) dummy);

    return(UX_SUCCESS);
}

UINT _ux_host_class_dummy_deactivate(UX_HOST_CLASS_COMMAND *command)
{

UX_HOST_CLASS_DUMMY         *dummy;
UX_ENDPOINT                 *endpoint;


    dummy = (UX_HOST_CLASS_DUMMY *)command -> ux_host_class_command_instance;

    dummy -> ux_host_class_dummy_state = UX_HOST_CLASS_INSTANCE_SHUTDOWN;

    endpoint = dummy -> ux_host_class_dummy_interface -> ux_interface_first_endpoint;
    while(endpoint)
    {
        _ux_host_stack_endpoint_transfer_abort(endpoint);
        endpoint = endpoint -> ux_endpoint_next_endpoint;
    }

    /* If the class instance was busy, let it finish properly and not return.  */
    _ux_host_thread_sleep(UX_ENUMERATION_THREAD_WAIT); 

    /* Destroy the instance.  */
    _ux_host_stack_class_instance_destroy(dummy -> ux_host_class_dummy_class, (VOID *) dummy);

    /* Notify application.  */
    if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
        _ux_system_host ->  ux_system_host_change_function(UX_DEVICE_REMOVAL, dummy -> ux_host_class_dummy_class, (VOID *) dummy);

    /* Free instance memory.  */
    _ux_utility_memory_free(dummy);

    return(UX_SUCCESS);
}

UINT _ux_host_class_dummy_device_entry(UX_HOST_CLASS_COMMAND *command)
{

    /* The command request will tell us we need to do here, either a enumeration
       query, an activation or a deactivation.  */
    switch (command -> ux_host_class_command_request)
    {

    case UX_HOST_CLASS_COMMAND_QUERY:
        if (_ux_host_class_dummy_command_query_list_check(command) == UX_SUCCESS)
            return(UX_SUCCESS);

        /* Only for device : class subclass protocol.  */
        if (command -> ux_host_class_command_usage != UX_HOST_CLASS_COMMAND_USAGE_DCSP)
            return(UX_NO_CLASS_MATCH);

        /* Skip undefined or composite.  */
        if (command -> ux_host_class_command_class == 0 ||
            command -> ux_host_class_command_class == 0xFE)
            return(UX_NO_CLASS_MATCH);

        if (!_ux_host_class_dummy_query_reject_unknown)
            return(UX_SUCCESS);
        return(UX_NO_CLASS_MATCH);

    case UX_HOST_CLASS_COMMAND_ACTIVATE:
        return _ux_host_class_dummy_device_activate(command);

    /* Standalone activate wait support.  */
    case UX_HOST_CLASS_COMMAND_ACTIVATE_WAIT:
        return (UX_STATE_NEXT);

    case UX_HOST_CLASS_COMMAND_DEACTIVATE:
        return _ux_host_class_dummy_device_deactivate(command);

    default:

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_FUNCTION_NOT_SUPPORTED);
    }
}

static UINT _ux_host_class_dummy_device_configure(UX_HOST_CLASS_DUMMY *dummy)
{
UINT                status;
UX_DEVICE           *device;

    device = dummy -> ux_host_class_dummy_device;
    status =  _ux_host_stack_device_configuration_select(device -> ux_device_first_configuration);
    return(UX_SUCCESS);
}

UINT _ux_host_class_dummy_device_activate(UX_HOST_CLASS_COMMAND *command)
{
UX_DEVICE                   *device;
UX_HOST_CLASS_DUMMY         *dummy;

    device =  (UX_DEVICE *) command -> ux_host_class_command_container;
    dummy = (UX_HOST_CLASS_DUMMY *)_ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HOST_CLASS_DUMMY));
    if (dummy == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    dummy -> ux_host_class_dummy_class = command -> ux_host_class_command_class_ptr;
    dummy -> ux_host_class_dummy_interface = UX_NULL;
    dummy -> ux_host_class_dummy_device = device;
    device -> ux_device_class_instance = (VOID *)dummy;

    _ux_host_stack_class_instance_create(dummy -> ux_host_class_dummy_class, (VOID *)dummy);

    if (_ux_host_class_dummy_device_configure(dummy) != UX_SUCCESS)
    {
        _ux_utility_memory_free(dummy);
        return(UX_ERROR);
    }

    dummy -> ux_host_class_dummy_state = UX_HOST_CLASS_INSTANCE_LIVE;

    if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
        _ux_system_host ->  ux_system_host_change_function(UX_DEVICE_INSERTION, dummy -> ux_host_class_dummy_class, (VOID *) dummy);

    return(UX_SUCCESS);
}

UINT _ux_host_class_dummy_device_deactivate(UX_HOST_CLASS_COMMAND *command)
{
UX_HOST_CLASS_DUMMY         *dummy;
UX_CONFIGURATION            *configuration;
UX_INTERFACE                *interface;
UX_ENDPOINT                 *endpoint;


    dummy = (UX_HOST_CLASS_DUMMY *)command -> ux_host_class_command_instance;

    dummy -> ux_host_class_dummy_state = UX_HOST_CLASS_INSTANCE_SHUTDOWN;

    /* Abort all transfer of current configuration.  */
    configuration = dummy -> ux_host_class_dummy_device -> ux_device_current_configuration;
    interface = configuration -> ux_configuration_first_interface;
    while(interface)
    {
        endpoint = interface -> ux_interface_first_endpoint;
        while(endpoint)
        {
            _ux_host_stack_endpoint_transfer_abort(endpoint);
            endpoint = endpoint -> ux_endpoint_next_endpoint;
        }
        interface = interface -> ux_interface_next_interface;
    }

    /* If the class instance was busy, let it finish properly and not return.  */
    _ux_host_thread_sleep(UX_ENUMERATION_THREAD_WAIT); 

    /* Destroy the instance.  */
    _ux_host_stack_class_instance_destroy(dummy -> ux_host_class_dummy_class, (VOID *) dummy);

    if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
        _ux_system_host ->  ux_system_host_change_function(UX_DEVICE_REMOVAL, dummy -> ux_host_class_dummy_class, (VOID *) dummy);

    return(UX_SUCCESS);
}
#if 0
UX_CONFIGURATION *_ux_host_class_dummy_get_configuration(UX_HOST_CLASS_DUMMY *dummy, UCHAR configuration_value)
{
UX_DEVICE                   *device;
UX_CONFIGURATION            *configuration;
UX_INTERFACE                *interface;
    if (dummy == UX_NULL)
        return(UX_NULL);
    if (dummy -> ux_host_class_dummy_device)
        device = dummy -> ux_host_class_dummy_device;
    else if (dummy -> ux_host_class_dummy_interface)
    {
        interface = dummy -> ux_host_class_dummy_interface;
        configuration = interface -> ux_interface_configuration;
        device = dummy -> ux_host_class_dummy_device;
    }
    else
        return(UX_NULL);
    configuration = device -> ux_device_first_configuration;
    while(configuration)
    {
        if (configuration -> ux_configuration_descriptor.bConfigurationValue == configuration_value)
            return(configuration);
        configuration = configuration -> ux_configuration_next_configuration;
    }
    return(UX_NULL);
}
#endif
static UX_INTERFACE *_ux_host_class_dummy_get_interface(UX_HOST_CLASS_DUMMY *dummy, UCHAR interface_number, UCHAR alternate_setting)
{
UX_DEVICE                   *device;
UX_CONFIGURATION            *configuration;
UX_INTERFACE                *interface;
    if (dummy == UX_NULL)
        return(UX_NULL);
    if (dummy -> ux_host_class_dummy_device)
    {
        device = dummy -> ux_host_class_dummy_device;
        configuration = device -> ux_device_current_configuration;
    }
    else if (dummy -> ux_host_class_dummy_interface)
    {
        interface = dummy -> ux_host_class_dummy_interface;
        configuration = interface -> ux_interface_configuration;
    }
    else
        return(UX_NULL);
    interface = configuration -> ux_configuration_first_interface;
    while(interface)
    {
        if (interface -> ux_interface_descriptor.bInterfaceNumber == (ULONG)interface_number &&
            interface -> ux_interface_descriptor.bAlternateSetting == (ULONG)alternate_setting)
            return(interface);
        interface = interface -> ux_interface_next_interface;
    }
    return(UX_NULL);
}

UINT _ux_host_class_dummy_select_interface(UX_HOST_CLASS_DUMMY *dummy, UCHAR interface_number, UCHAR alternate_setting)
{
UX_INTERFACE    *interface = _ux_host_class_dummy_get_interface(dummy, interface_number, alternate_setting);
    if (interface == UX_NULL)
        return(UX_ERROR);
    return(ux_host_stack_interface_setting_select(interface));
}

static UX_ENDPOINT *_ux_host_class_dummy_device_get_endpoint(UX_HOST_CLASS_DUMMY *dummy, UCHAR endpoint_address, UCHAR alternate_setting)
{
UX_CONFIGURATION            *configuration;
UX_INTERFACE                *interface;
UX_ENDPOINT                 *endpoint;
    if (dummy == UX_NULL)
        return(UX_NULL);
    if (endpoint_address == 0)
        return(&dummy -> ux_host_class_dummy_device -> ux_device_control_endpoint);
    configuration = dummy -> ux_host_class_dummy_device -> ux_device_current_configuration;
    if (configuration == UX_NULL)
        return(UX_NULL);
    interface = configuration -> ux_configuration_first_interface;
    while(interface)
    {
        if (interface -> ux_interface_descriptor.bAlternateSetting == alternate_setting)
        {
            endpoint = interface -> ux_interface_first_endpoint;
            while(endpoint)
            {
                if (endpoint -> ux_endpoint_descriptor.bEndpointAddress == endpoint_address)
                    return(endpoint);
                endpoint = endpoint -> ux_endpoint_next_endpoint;
            }
        }
        interface = interface -> ux_interface_next_interface;
    }
    return(UX_NULL);
}
UX_ENDPOINT *_ux_host_class_dummy_get_endpoint(UX_HOST_CLASS_DUMMY *dummy, UCHAR endpoint_address, UCHAR alternate_setting)
{
UX_INTERFACE                *interface;
UX_ENDPOINT                 *endpoint;
    if (dummy == UX_NULL)
        return(UX_NULL);
    if (dummy -> ux_host_class_dummy_device)
        return(_ux_host_class_dummy_device_get_endpoint(dummy, endpoint_address, alternate_setting));
    interface = dummy -> ux_host_class_dummy_interface;
    if (endpoint_address == 0)
        return(&interface -> ux_interface_configuration -> ux_configuration_device -> ux_device_control_endpoint);
    if (interface -> ux_interface_descriptor.bAlternateSetting != (ULONG)alternate_setting)
        return(UX_NULL);
    endpoint = interface -> ux_interface_first_endpoint;
    while(endpoint)
    {
        if (endpoint -> ux_endpoint_descriptor.bEndpointAddress == (ULONG)endpoint_address)
        {
            return(endpoint);
        }
        endpoint = endpoint -> ux_endpoint_next_endpoint;
    }
    return(UX_NULL);
}

UX_TRANSFER *_ux_host_class_dummy_get_transfer_request(UX_HOST_CLASS_DUMMY *dummy, UCHAR endpoint_address, UCHAR alternate_setting)
{
UX_ENDPOINT         *endpoint;
    endpoint = _ux_host_class_dummy_get_endpoint(dummy, endpoint_address, alternate_setting);
    if (endpoint == UX_NULL)
        return(UX_NULL);
    return(&endpoint -> ux_endpoint_transfer_request);
}

VOID  _ux_host_class_dummy_set_timeout(UX_HOST_CLASS_DUMMY *dummy, UCHAR endpoint_address, UCHAR alternate_setting, ULONG timeout)
{
UX_TRANSFER *transfer = _ux_host_class_dummy_get_transfer_request(dummy, endpoint_address, alternate_setting);
    UX_TEST_ASSERT(transfer);
    transfer -> ux_transfer_request_timeout_value = timeout;
}

ULONG _ux_host_class_dummy_get_max_packet_size(UX_HOST_CLASS_DUMMY *dummy, UCHAR endpoint_address, UCHAR alternate_setting)
{
UX_ENDPOINT *endpoint = _ux_host_class_dummy_get_endpoint(dummy, endpoint_address, alternate_setting);
    UX_TEST_ASSERT(endpoint);
    return(endpoint->ux_endpoint_descriptor.wMaxPacketSize & UX_MAX_PACKET_SIZE_MASK);
}

ULONG _ux_host_class_dummy_get_max_payload_size(UX_HOST_CLASS_DUMMY *dummy, UCHAR endpoint_address, UCHAR alternate_setting)
{
UX_ENDPOINT *endpoint = _ux_host_class_dummy_get_endpoint(dummy, endpoint_address, alternate_setting);
ULONG       trans, size;
    UX_TEST_ASSERT(endpoint);
    size = endpoint->ux_endpoint_descriptor.wMaxPacketSize & UX_MAX_PACKET_SIZE_MASK;
    trans = endpoint->ux_endpoint_descriptor.wMaxPacketSize & UX_MAX_NUMBER_OF_TRANSACTIONS_MASK;
    if (trans)
    {
        trans >>= UX_MAX_NUMBER_OF_TRANSACTIONS_SHIFT;
        UX_TEST_ASSERT(trans < 3);
        size *= (trans + 1);
    }
    return(size);
}
#if !defined(UX_HOST_STANDALONE)
UINT  _ux_host_class_dummy_transfer(UX_HOST_CLASS_DUMMY *dummy, UCHAR endpoint_address, UCHAR alternate_setting, UCHAR *buffer, ULONG length, ULONG *actual_length)
{
UX_TRANSFER     *transfer = _ux_host_class_dummy_get_transfer_request(dummy, endpoint_address, alternate_setting);
ULONG           payload_size = _ux_host_class_dummy_get_max_payload_size(dummy, endpoint_address, alternate_setting);
ULONG           transfer_size;
UINT            status;
    UX_TEST_ASSERT(transfer);
    *actual_length = 0;
    do {
        transfer_size = UX_MIN(payload_size, length);
        transfer->ux_transfer_request_requested_length = transfer_size;
        transfer->ux_transfer_request_data_pointer = buffer;
        status = ux_host_stack_transfer_request(transfer);

        /* Error check.  */
        if (status != UX_SUCCESS)
            return(status);

        /* Semaphore wait.  */
        status = _ux_utility_semaphore_get(&transfer->ux_transfer_request_semaphore, transfer->ux_transfer_request_timeout_value);
        *actual_length += transfer->ux_transfer_request_actual_length;

        /* Semaphore error.  */
        if (status != UX_SUCCESS)
        {
            ux_host_stack_transfer_request_abort(transfer);
            transfer->ux_transfer_request_completion_code = UX_TRANSFER_TIMEOUT;
            return(UX_TRANSFER_TIMEOUT);
        }

        /* Short packet check.  */
        if (transfer_size != transfer->ux_transfer_request_actual_length)
            return(UX_SUCCESS);

        /* Update transfer.  */
        buffer += transfer_size;
        length -= transfer_size;

    } while(length);
    return(UX_SUCCESS);
}
#endif
UINT  _ux_host_class_dummy_abort(UX_HOST_CLASS_DUMMY *dummy, UCHAR endpoint_address, UCHAR alternate_setting)
{
UX_TRANSFER     *transfer = _ux_host_class_dummy_get_transfer_request(dummy, endpoint_address, alternate_setting);
    UX_TEST_ASSERT(transfer);
    return ux_host_stack_transfer_request_abort(transfer);
}
