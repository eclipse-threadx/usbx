/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation 
 * 
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 * 
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** USBX Component                                                        */
/**                                                                       */
/**   Device Printer Class                                                */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_printer.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_printer_ioctl                      PORTABLE C      */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function performs certain functions on the printer instance    */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    printer                               Address of printer class      */
/*                                            instance                    */
/*    ioctl_function                        Ioctl function                */
/*    Parameter                             Parameter of ioctl function   */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Status                                                              */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  01-31-2022     Chaoqiong Xiao           Initial Version 6.1.10        */
/*  03-08-2023     Yajun Xia                Modified comment(s),          */
/*                                            resulting in version 6.2.1  */
/*                                                                        */
/**************************************************************************/
UINT _ux_device_class_printer_ioctl(UX_DEVICE_CLASS_PRINTER *printer, ULONG ioctl_function,
                                    VOID *parameter)
{

UINT                                status;
UX_SLAVE_ENDPOINT                   *endpoint;
UX_SLAVE_TRANSFER                   *transfer;


    /* Default to success.  */
    status = UX_SUCCESS;

    /* The command request will tell us what we need to do here.  */
    switch (ioctl_function)
    {
    case UX_DEVICE_CLASS_PRINTER_IOCTL_PORT_STATUS_SET:

        /* Set port status.  */
        printer -> ux_device_class_printer_port_status = (ULONG)(UCHAR)(ALIGN_TYPE)parameter;
        break;

    case UX_DEVICE_CLASS_PRINTER_IOCTL_READ_TIMEOUT_SET:

        /* Set endpoint transfer timeout.  */
        endpoint = printer -> ux_device_class_printer_endpoint_out;
        transfer = &endpoint -> ux_slave_endpoint_transfer_request;
        transfer -> ux_slave_transfer_request_timeout = (ULONG)(ALIGN_TYPE)parameter;
        break;

    case UX_DEVICE_CLASS_PRINTER_IOCTL_WRITE_TIMEOUT_SET:

        /* Set endpoint transfer timeout (if exist).  */
        endpoint = printer -> ux_device_class_printer_endpoint_in;
        if (endpoint != UX_NULL)
        {
            transfer = &endpoint -> ux_slave_endpoint_transfer_request;
            transfer -> ux_slave_transfer_request_timeout = (ULONG)(ALIGN_TYPE)parameter;
        }
        break;

    default:

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        /* Function not supported. Return an error.  */
        status =  UX_FUNCTION_NOT_SUPPORTED;
        break;
    }

    /* Return status to caller.  */
    return(status);

}

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _uxe_device_class_printer_ioctl                      PORTABLE C     */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Yajun Xia, Microsoft Corporation                                    */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks errors in printer class ioctl function         */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    printer                               Address of printer class      */
/*                                            instance                    */
/*    ioctl_function                        Ioctl function                */
/*    Parameter                             Parameter of ioctl function   */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Status                                                              */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_class_printer_ioctl        Printer class ioctl function  */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  03-08-2023     Yajun Xia                Initial Version 6.2.1         */
/*                                                                        */
/**************************************************************************/
UINT _uxe_device_class_printer_ioctl(UX_DEVICE_CLASS_PRINTER *printer, ULONG ioctl_function,
                                     VOID *parameter)
{

    /* Sanity checks.  */
    if (printer == UX_NULL)
    {
        return (UX_INVALID_PARAMETER);
    }

    return (_ux_device_class_printer_ioctl(printer, ioctl_function, parameter));
}