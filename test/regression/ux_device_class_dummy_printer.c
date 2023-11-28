#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_device_stack.h"
#include "ux_test.h"
#include "ux_host_class_printer.h"
#include "ux_device_class_dummy_printer.h"

#define PORT_START 1
#define PORT_STATUS_PORT_POWER_BIT 8

UCHAR _ux_device_class_printer_name[] = "_ux_device_class_printer";

static VOID _ux_device_class_printer_initialize(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS                  *printer_class = (UX_SLAVE_CLASS *)command->ux_slave_class_command_class_ptr;
UX_DEVICE_CLASS_PRINTER_PARAMS  *printer_params = (UX_DEVICE_CLASS_PRINTER_PARAMS *)command->ux_slave_class_command_parameter;
UX_DEVICE_CLASS_PRINTER         *printer;

    printer = (UX_DEVICE_CLASS_PRINTER *) _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_DEVICE_CLASS_PRINTER));
    UX_TEST_ASSERT(printer != UX_NULL);

    printer_class->ux_slave_class_instance = printer;

    /* Save parameters.  */
    _ux_utility_memory_copy(&printer->params, printer_params, sizeof(*printer_params));
}

static VOID _ux_device_class_printer_uninitialize(UX_SLAVE_CLASS_COMMAND *command)
{
UX_DEVICE_CLASS_PRINTER     *printer = NULL;
UX_SLAVE_CLASS              *class;


    /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    printer = (UX_DEVICE_CLASS_PRINTER *) class -> ux_slave_class_instance;

    if (printer != UX_NULL)
        _ux_utility_memory_free(printer);
    return;
}

static UINT _ux_device_class_printer_activate(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS              *printer_class = command->ux_slave_class_command_class_ptr;
UX_SLAVE_INTERFACE          *interface = (UX_SLAVE_INTERFACE *)command->ux_slave_class_command_interface;
UX_SLAVE_ENDPOINT           *endpoint;
UX_DEVICE_CLASS_PRINTER     *printer;
UINT                        i;

    printer = (UX_DEVICE_CLASS_PRINTER *)printer_class->ux_slave_class_instance;
    interface -> ux_slave_interface_class_instance = (VOID *)printer;
    printer -> interface = interface;

    /* We need to get the bulk endpoints. */
    printer -> bulk_in_endpoint = UX_NULL;
    printer -> bulk_out_endpoint = UX_NULL;
    endpoint = interface->ux_slave_interface_first_endpoint;
    while (endpoint)
    {
        /* Is this bulk endpoint?  */
        if (endpoint->ux_slave_endpoint_descriptor.bmAttributes == UX_BULK_ENDPOINT)
        {
            /* Is this endpoint IN?  */
            if (endpoint->ux_slave_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_IN)
            {
                printer -> bulk_in_endpoint = endpoint;
                if (printer -> bulk_out_endpoint)
                    break;
            }
            else
            {
                printer -> bulk_out_endpoint = endpoint;
                if (printer -> bulk_in_endpoint)
                    break;
            }
        }
        endpoint = endpoint->ux_slave_endpoint_next_endpoint;
    }
    /* Endpoints must be available.  */
    if (printer->bulk_in_endpoint == UX_NULL || printer->bulk_out_endpoint == UX_NULL)
        return(UX_ERROR);
    /* Invoke activate callback.  */
    if (printer->params.instance_activate)
    {
        printer->params.instance_activate(printer);
    }
    return(UX_SUCCESS);
}

static VOID _ux_device_class_printer_deactivate(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS                  *printer_class = command->ux_slave_class_command_class_ptr;
UX_DEVICE_CLASS_PRINTER         *printer;

    printer = (UX_DEVICE_CLASS_PRINTER *)printer_class->ux_slave_class_instance;
    _ux_device_stack_transfer_all_request_abort(printer->bulk_in_endpoint, UX_TRANSFER_BUS_RESET);
    _ux_device_stack_transfer_all_request_abort(printer->bulk_out_endpoint, UX_TRANSFER_BUS_RESET);
    if (printer->params.instance_deactivate)
    {
        printer->params.instance_deactivate(printer);
    }
}

static VOID _ux_device_class_printer_change(UX_SLAVE_CLASS_COMMAND *command)
{
    UX_TEST_ASSERT(0);
}

static UINT _configure_index_value(ULONG index)
{
UCHAR       *descriptor;
ULONG       configuration_index;
ULONG       length;
UCHAR       type;
    descriptor = _ux_system_slave->ux_system_slave_device_framework;
    configuration_index = 0;
    while(1)
    {
        length = descriptor[0];
        type = descriptor[1];

        if (type == UX_CONFIGURATION_DESCRIPTOR_ITEM)
        {
            if (configuration_index == index)
                return(descriptor[5]);
            /* Length is total length.  */
            length = _ux_utility_short_get(&descriptor[2]);
            /* Next configuration.  */
            configuration_index ++;
        }

        /* Next descriptor.  */
        descriptor += length;
    }
}

static UINT _ux_device_class_printer_control_request(UX_SLAVE_CLASS_COMMAND *command)
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
UX_SLAVE_CLASS              *printer_class = (UX_SLAVE_CLASS *)command->ux_slave_class_command_class_ptr;
UX_DEVICE_CLASS_PRINTER     *printer = (UX_DEVICE_CLASS_PRINTER *)printer_class->ux_slave_class_instance;
UX_SLAVE_INTERFACE          *interface=printer->interface;
ULONG                       length;

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
    case UX_HOST_CLASS_PRINTER_GET_DEVICE_ID:
        /* Check configuration INDEX (wValue), interface (HIGH wIndex), alt (LOW wIndex).  */
        if (_configure_index_value(wValue) != interface->ux_slave_interface_class->ux_slave_class_configuration_number ||
            setup_data[4] != interface->ux_slave_interface_descriptor.bAlternateSetting ||
            setup_data[5] != interface->ux_slave_interface_descriptor.bInterfaceNumber)
            return(UX_ERROR);
        /* Send device ID.  */
        transfer_request->ux_slave_transfer_request_phase = UX_TRANSFER_PHASE_DATA_OUT;
        length = UX_MIN(UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH, wLength);
        length = UX_MIN(length, printer->params.device_id_length + 2);
        if (length)
        {
            _ux_utility_short_put_big_endian(data_ptr, length - 2);
            _ux_utility_memory_copy(data_ptr + 2, printer->params.device_id, length - 2);
        }
        return _ux_device_stack_transfer_request(transfer_request, length, wLength);

    case UX_HOST_CLASS_PRINTER_GET_STATUS:
        /* Check interface number.  */
        if (wIndex != interface->ux_slave_interface_descriptor.bInterfaceNumber)
            return(UX_ERROR);
        /* Send status.  */
        transfer_request->ux_slave_transfer_request_phase = UX_TRANSFER_PHASE_DATA_OUT;
        *data_ptr = printer->port_status;
        return _ux_device_stack_transfer_request(transfer_request, 1, wLength);

    case UX_HOST_CLASS_PRINTER_SOFT_RESET:
        /* Check interface number.  */
        if (wIndex != interface->ux_slave_interface_descriptor.bInterfaceNumber)
            return(UX_ERROR);
        /* Execute soft reset.  */
        _ux_device_stack_transfer_all_request_abort(printer->bulk_in_endpoint, UX_TRANSFER_BUS_RESET);
        _ux_device_stack_transfer_all_request_abort(printer->bulk_out_endpoint, UX_TRANSFER_BUS_RESET);
        printer->soft_reset = UX_TRUE;
        return(UX_SUCCESS);

    default:

        UX_TEST_ASSERT(0);
        break;
    }
}

UINT _ux_device_class_printer_entry(UX_SLAVE_CLASS_COMMAND *command)
{

    switch(command -> ux_slave_class_command_request)
    {

    case UX_SLAVE_CLASS_COMMAND_INITIALIZE:
        _ux_device_class_printer_initialize(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_UNINITIALIZE:
        _ux_device_class_printer_uninitialize(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_QUERY:
        /* For now, always return success. */
        break;

    case UX_SLAVE_CLASS_COMMAND_ACTIVATE:
        return _ux_device_class_printer_activate(command);

    case UX_SLAVE_CLASS_COMMAND_CHANGE:
        UX_TEST_ASSERT(0);
        break;

    case UX_SLAVE_CLASS_COMMAND_DEACTIVATE:
        _ux_device_class_printer_deactivate(command);
        break;

    case UX_SLAVE_CLASS_COMMAND_REQUEST:
        return _ux_device_class_printer_control_request(command);

    default:
        UX_TEST_ASSERT(0);
        break;

    }
    return(UX_SUCCESS);
}
