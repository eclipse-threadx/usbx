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
/**   Device CDC Class                                                    */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_cdc_acm_entry                      PORTABLE C      */
/*                                                           6.x          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is the entry point of the cdc_acm class. It           */
/*    will be called by the device stack enumeration module when the      */
/*    host has sent a SET_CONFIGURATION command and the cdc_acm interface */
/*    needs to be mounted.                                                */
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
/*    _ux_device_class_cdc_acm_initialize       Initialize cdc_acm class  */
/*    _ux_device_class_cdc_acm_uninitialize     Uninitialize cdc_acm class*/
/*    _ux_device_class_cdc_acm_activate         Activate cdc_acm class    */
/*    _ux_device_class_cdc_acm_deactivate       Deactivate cdc_acm class  */
/*    _ux_device_class_cdc_acm_control_request  Request control           */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    CDC Class                                                           */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  xx-xx-xxxx     Mohamed ayed             Modified comment(s),          */
/*                                            fix typo,                   */
/*                                            remove extra spaces,        */
/*                                            resulting in version 6.x    */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_cdc_acm_entry(UX_SLAVE_CLASS_COMMAND *command)
{

UINT        status;


    /* The command request will tell us we need to do here, either a enumeration
       query, an activation or a deactivation.  */
    switch (command -> ux_slave_class_command_request)
    {

    case UX_SLAVE_CLASS_COMMAND_INITIALIZE:

        /* Call the init function of the CDC ACM class.  */
        status =  _ux_device_class_cdc_acm_initialize(command);

        /* Return the completion status.  */
        return(status);

    case UX_SLAVE_CLASS_COMMAND_UNINITIALIZE:

        /* Call the uninit function of the CDC ACM class.  */
        status =  _ux_device_class_cdc_acm_uninitialize(command);

        /* Return the completion status.  */
        return(status);

    case UX_SLAVE_CLASS_COMMAND_QUERY:

        /* Check the CLASS definition in the interface descriptor. */
        if (command -> ux_slave_class_command_class == UX_SLAVE_CLASS_CDC_ACM_CLASS)
            return(UX_SUCCESS);
        else
            return(UX_NO_CLASS_MATCH);

    case UX_SLAVE_CLASS_COMMAND_ACTIVATE:

        /* The activate command is used when the host has sent a SET_CONFIGURATION command
           and this interface has to be mounted. Both Bulk endpoints have to be mounted
           and the cdc_acm thread needs to be activated.  */
        status =  _ux_device_class_cdc_acm_activate(command);

        /* Return the completion status.  */
        return(status);

    case UX_SLAVE_CLASS_COMMAND_DEACTIVATE:

        /* The deactivate command is used when the device has been extracted.
           The device endpoints have to be dismounted and the cdc_acm thread canceled.  */
        status =  _ux_device_class_cdc_acm_deactivate(command);

        /* Return the completion status.  */
        return(status);

    case UX_SLAVE_CLASS_COMMAND_REQUEST:

        /* The request command is used when the host sends a command on the control endpoint.  */
        status = _ux_device_class_cdc_acm_control_request(command);

        /* Return the completion status.  */
        return(status);

    default:

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        /* Return an error.  */
        return(UX_FUNCTION_NOT_SUPPORTED);
    }
}

