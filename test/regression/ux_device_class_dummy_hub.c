#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_device_stack.h"
#include "ux_test.h"
#include "ux_host_class_hub.h"
#include "ux_device_class_dummy_hub.h"

#define PORT_START 1
#define PORT_STATUS_PORT_POWER_BIT 8

UCHAR _ux_device_class_hub_name[] = "_ux_device_class_hub_name";

static VOID _ux_device_class_hub_thread_entry(ULONG param)
{
UX_DEVICE_CLASS_HUB *hub;
UX_SLAVE_CLASS      *hub_class;
UX_SLAVE_ENDPOINT   *endpoint;
UX_SLAVE_TRANSFER   *transfer;
UINT                status;
    UX_THREAD_EXTENSION_PTR_GET(hub_class ,UX_SLAVE_CLASS, param);
    hub = (UX_DEVICE_CLASS_HUB *)hub_class->ux_slave_class_instance;
    while(1)
    {
        endpoint = hub->interrupt_endpoint;
        if (endpoint)
        {
            transfer = &endpoint->ux_slave_endpoint_transfer_request;
            status = _ux_device_stack_transfer_request(transfer,
                        transfer -> ux_slave_transfer_request_requested_length,
                        transfer -> ux_slave_transfer_request_requested_length);
            _ux_utility_memory_set(transfer->ux_slave_transfer_request_data_pointer, 0, transfer->ux_slave_transfer_request_transfer_length);
        }
        _ux_device_thread_suspend(&hub->hub_thread);
    }
}

static VOID _ux_device_class_hub_initialize(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS              *hub_class = (UX_SLAVE_CLASS *)command->ux_slave_class_command_class_ptr;
UX_DEVICE_CLASS_HUB_PARAMS  *hub_params = (UX_DEVICE_CLASS_HUB_PARAMS *)command->ux_slave_class_command_parameter;
UX_DEVICE_CLASS_HUB         *hub;

    hub = (UX_DEVICE_CLASS_HUB *) _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_DEVICE_CLASS_HUB));
    UX_TEST_ASSERT(hub != UX_NULL);

    hub_class->ux_slave_class_instance = hub;
    hub->hub_class = hub_class;

    hub->hub_thread_stack = _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, UX_THREAD_STACK_SIZE);
    UX_TEST_ASSERT(hub->hub_thread_stack != UX_NULL);
    UX_TEST_CHECK_SUCCESS(_ux_device_thread_create(
            &hub->hub_thread, "hub_thread",
            _ux_device_class_hub_thread_entry, (ULONG)(ALIGN_TYPE)hub_class,
            hub->hub_thread_stack, UX_THREAD_STACK_SIZE,
            30, 30, 0, UX_DONT_START));
    UX_THREAD_EXTENSION_PTR_SET(&hub->hub_thread, hub_class);

    /* Save parameters.  */
    _ux_utility_memory_copy(&hub->params, hub_params, sizeof(*hub_params));
}

static VOID _ux_device_class_hub_uninitialize(UX_SLAVE_CLASS_COMMAND *command)
{
}
VOID _ux_device_class_hub_notify_changes(UX_DEVICE_CLASS_HUB *hub, UCHAR *changes, UINT rpt_size)
{
UX_SLAVE_ENDPOINT   *endpoint;
UX_SLAVE_TRANSFER   *transfer;
UCHAR               *buff;
    endpoint = hub->interrupt_endpoint;
    if (endpoint == UX_NULL || rpt_size == 0)
        return;
    transfer = &endpoint->ux_slave_endpoint_transfer_request;
    buff = transfer->ux_slave_transfer_request_data_pointer;
    if (rpt_size > transfer->ux_slave_transfer_request_transfer_length)
        rpt_size = transfer->ux_slave_transfer_request_transfer_length;
    transfer -> ux_slave_transfer_request_requested_length = rpt_size;
    _ux_utility_memory_copy(buff, changes, rpt_size);
    tx_thread_resume(&hub->hub_thread);
}
VOID _ux_device_class_hub_notify_change(UX_DEVICE_CLASS_HUB *hub, UINT change_pos, UINT rpt_size)
{
UX_SLAVE_ENDPOINT   *endpoint;
UX_SLAVE_TRANSFER   *transfer;
UCHAR               *buff;
UINT                byte_pos, bit_pos;
    endpoint = hub->interrupt_endpoint;
    if (endpoint == UX_NULL || rpt_size == 0)
        return;
    transfer = &endpoint->ux_slave_endpoint_transfer_request;
    buff = transfer->ux_slave_transfer_request_data_pointer;
    byte_pos = change_pos >> 3;
    bit_pos = change_pos & 0x7u;
    if (rpt_size >= transfer->ux_slave_transfer_request_transfer_length)
        rpt_size = transfer->ux_slave_transfer_request_transfer_length;
    if (byte_pos >= rpt_size)
        return;
    buff[byte_pos] |= 1u << bit_pos;
    transfer -> ux_slave_transfer_request_requested_length = rpt_size;
    tx_thread_resume(&hub->hub_thread);
}

static UINT _ux_device_class_hub_activate(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS              *hub_class = command->ux_slave_class_command_class_ptr;
UX_SLAVE_INTERFACE          *interface = (UX_SLAVE_INTERFACE *)command->ux_slave_class_command_interface;
UX_SLAVE_ENDPOINT           *endpoint;
UX_SLAVE_TRANSFER           *transfer;
UX_DEVICE_CLASS_HUB         *hub;
UINT                        i;

    hub = (UX_DEVICE_CLASS_HUB *)hub_class->ux_slave_class_instance;

    /* Our dummy hub has one port. When the host sends the SetPortFeature request,
       wIndex contains the port number, which is 1-based. USBX uses wIndex to
       pick which class/interface to send the command to. The problem is that
       in this case, wIndex is not an interface number! So we have to manually
       link the port number (1 in this case) to this class. We just set every
       entry since we don't expect there to be any other classes registered.
       If we do start expecting that, then we'll handle it then (as opposed to now). */
    for (i = 0; i < UX_MAX_SLAVE_INTERFACES; i++)
    {
        _ux_system_slave->ux_system_slave_interface_class_array[i] = hub_class;
    }

    /* We need to get the interrupt in endpoint. */
    endpoint = interface->ux_slave_interface_first_endpoint;
    while (endpoint)
    {
        /* Is this interrupt in? */
        if (endpoint->ux_slave_endpoint_descriptor.bmAttributes == 0x03 &&
            (endpoint->ux_slave_endpoint_descriptor.bEndpointAddress & 0x80) == 0x80)
        {
            /* Found it. */
            break;
        }

        endpoint = endpoint->ux_slave_endpoint_next_endpoint;
    }
    /* Note that we don't always expect there to be an endpoint since some tests
       depend on no endpoint. */
    if (endpoint)
    {
        hub->interrupt_endpoint = endpoint;

        /* If there is interrupt endpoint, create notify thread for notification support.  */
        if (endpoint != UX_NULL)
        {
            transfer = &endpoint->ux_slave_endpoint_transfer_request;
            transfer->ux_slave_transfer_request_data_pointer = _ux_utility_memory_allocate(UX_NO_ALIGN, UX_CACHE_SAFE_MEMORY, transfer->ux_slave_transfer_request_transfer_length);
            if(transfer->ux_slave_transfer_request_data_pointer == UX_NULL)
                return(UX_MEMORY_INSUFFICIENT);
        }
    }

    if (hub->params.instance_activate)
    {
        hub->params.instance_activate(hub);
    }
    return(UX_SUCCESS);
}

static VOID _ux_device_class_hub_deactivate(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS              *hub_class = command->ux_slave_class_command_class_ptr;
UX_DEVICE_CLASS_HUB         *hub;

    hub = (UX_DEVICE_CLASS_HUB *)hub_class->ux_slave_class_instance;
    if (hub->interrupt_endpoint)
    {
        _ux_device_stack_transfer_abort(&hub->interrupt_endpoint->ux_slave_endpoint_transfer_request, UX_TRANSFER_BUS_RESET);
        _ux_utility_memory_free(hub->interrupt_endpoint->ux_slave_endpoint_transfer_request.ux_slave_transfer_request_data_pointer);
    }

    if (hub->params.instance_deactivate)
    {
        hub->params.instance_deactivate(hub);
    }
}

static VOID _ux_device_class_hub_change(UX_SLAVE_CLASS_COMMAND *command)
{
    UX_TEST_ASSERT(0);
}

static VOID _ux_device_class_hub_control_request(UX_SLAVE_CLASS_COMMAND *command)
{

UCHAR                       *setup_data;
UCHAR                       bmRequestType;
UCHAR                       bRequest;
USHORT                      wValue;
USHORT                      wIndex;
USHORT                      wLength;
UCHAR                       port_index;
UCHAR                       descriptor_type;
UCHAR                       *data_ptr;
UX_SLAVE_DEVICE             *device;
UX_SLAVE_TRANSFER           *transfer_request;
UX_SLAVE_CLASS              *hub_class = (UX_SLAVE_CLASS *)command->ux_slave_class_command_class_ptr;
UX_DEVICE_CLASS_HUB         *dummy_hub = (UX_DEVICE_CLASS_HUB *)hub_class->ux_slave_class_instance;

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave->ux_system_slave_device;

    /* Get the pointer to the transfer request associated with the control endpoint.  */
    transfer_request =  &device->ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

    setup_data = transfer_request->ux_slave_transfer_request_setup;
    bmRequestType = setup_data[0];
    bRequest = setup_data[1];
    wValue = (USHORT)_ux_utility_short_get(&setup_data[2]);
    wIndex = (USHORT)_ux_utility_short_get(&setup_data[4]);
    wLength = (USHORT)_ux_utility_short_get(&setup_data[6]);

    data_ptr = transfer_request->ux_slave_transfer_request_data_pointer;

    switch (bRequest)
    {

        case UX_SET_FEATURE:

            /* Get the port. */
            port_index = (UCHAR) (wValue & 0x00ff);

            /* What feature? */
            switch (wValue)
            {

                case UX_HOST_CLASS_HUB_PORT_RESET:

                    if (!dummy_hub->dont_reset_port_when_commanded_to)
                    {

                        /* Tell the host we reset. */
                        dummy_hub->port_change |= UX_HOST_CLASS_HUB_PORT_CHANGE_RESET;
                        _ux_device_class_hub_notify_change(dummy_hub, 1, 1);
                    }

                    /* Note that the speed is in port_status, and should've been
                       set by the application. */

                    break;

                case UX_HOST_CLASS_HUB_PORT_POWER:

                    /* Do nothing. */
                    break;

                default:

                    UX_TEST_ASSERT(0);
                    break;
            }

            break;

        case UX_CLEAR_FEATURE:

            /* What feature? */
            switch (wValue)
            {

                case UX_HOST_CLASS_HUB_C_PORT_RESET:

                    /* Clear it. */
                    dummy_hub->port_change &= (~UX_HOST_CLASS_HUB_PORT_CHANGE_RESET);

                    break;

                case UX_HOST_CLASS_HUB_C_PORT_ENABLE:

                    /* Clear it. */
                    dummy_hub->port_change &= (~UX_HOST_CLASS_HUB_PORT_CHANGE_ENABLE);

                    break;

                case UX_HOST_CLASS_HUB_C_PORT_SUSPEND:

                    /* Clear it. */
                    dummy_hub->port_change &= (~UX_HOST_CLASS_HUB_PORT_CHANGE_SUSPEND);

                    break;

                case UX_HOST_CLASS_HUB_C_PORT_OVER_CURRENT:

                    /* Clear it. */
                    dummy_hub->port_change &= (~UX_HOST_CLASS_HUB_PORT_CHANGE_OVER_CURRENT);

                    break;

                case UX_HOST_CLASS_HUB_C_PORT_CONNECTION:

                    /* Clear it.  */
                    dummy_hub->port_change &= (~UX_HOST_CLASS_HUB_PORT_CHANGE_CONNECTION);
                    break;

                case UX_HOST_CLASS_HUB_PORT_ENABLE:

                    /* Do nothing. */
                    break;

                default:

                    UX_TEST_ASSERT(0);
                    break;
            }

            break;

        case UX_HOST_CLASS_HUB_GET_STATUS:

            /* Setup the data. */
            *((USHORT *)&data_ptr[0]) = dummy_hub->port_status;
            *((USHORT *)&data_ptr[2]) = dummy_hub->port_change;

            UX_TEST_CHECK_SUCCESS(_ux_device_stack_transfer_request(transfer_request, UX_HUB_DESCRIPTOR_LENGTH, UX_HUB_DESCRIPTOR_LENGTH));

            break;

        case UX_GET_DESCRIPTOR:

            /* Ensure this is for the hub descriptor and send it. */

            descriptor_type = (wValue & 0xff00) >> 8;
            UX_TEST_ASSERT(descriptor_type == UX_HUB_DESCRIPTOR_ITEM);
            UX_TEST_ASSERT(wLength == UX_HUB_DESCRIPTOR_LENGTH);

            _ux_utility_memory_copy(transfer_request->ux_slave_transfer_request_data_pointer, dummy_hub->params.descriptor, dummy_hub->params.descriptor_length);

            UX_TEST_CHECK_SUCCESS(_ux_device_stack_transfer_request(transfer_request, UX_HUB_DESCRIPTOR_LENGTH, UX_HUB_DESCRIPTOR_LENGTH));

            break;

        default:

            UX_TEST_ASSERT(0);
            break;
    }
}

UINT _ux_device_class_hub_entry(UX_SLAVE_CLASS_COMMAND *command)
{

    switch(command -> ux_slave_class_command_request)
    {

    case UX_SLAVE_CLASS_COMMAND_INITIALIZE:
        _ux_device_class_hub_initialize(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_UNINITIALIZE:
        _ux_device_class_hub_uninitialize(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_QUERY:
        /* For now, always return success. */
        break;

    case UX_SLAVE_CLASS_COMMAND_ACTIVATE:
        return _ux_device_class_hub_activate(command);

    case UX_SLAVE_CLASS_COMMAND_CHANGE:
        UX_TEST_ASSERT(0);
        break;

    case UX_SLAVE_CLASS_COMMAND_DEACTIVATE:
        _ux_device_class_hub_deactivate(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_REQUEST:
        _ux_device_class_hub_control_request(command);
        break;

    default:
        UX_TEST_ASSERT(0);
        break;

    }
    return(UX_SUCCESS);
}
