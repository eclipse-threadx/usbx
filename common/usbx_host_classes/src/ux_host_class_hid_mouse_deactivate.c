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
/**   HID Mouse Client Class                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_hid.h"
#include "ux_host_class_hid_mouse.h"
#include "ux_host_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_hid_mouse_deactivate                 PORTABLE C      */ 
/*                                                           6.3.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function performs the deactivation of a HID mouse.             */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    command                               Pointer to command            */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_class_hid_periodic_report_stop                             */ 
/*                                          Stop periodic report          */ 
/*    _ux_utility_memory_free               Free memory block             */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    HID Mouse Class                                                     */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  10-31-2023     Chaoqiong Xiao           Modified comment(s),          */
/*                                            improved unload sequence,   */
/*                                            resulting in version 6.3.0  */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_hid_mouse_deactivate(UX_HOST_CLASS_HID_CLIENT_COMMAND  *command)
{

UX_HOST_CLASS_HID           *hid;
UX_HOST_CLASS_HID_CLIENT    *hid_client;
UINT                        status;


    /* Get the instance to the HID class.  */
    hid =  command -> ux_host_class_hid_client_command_instance;

    /* Stop the periodic report.  */
    status =  _ux_host_class_hid_periodic_report_stop(hid);

    /* Get the HID client pointer.  */
    hid_client =  hid -> ux_host_class_hid_client;

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_HID_MOUSE_DEACTIVATE, hid, hid_client -> ux_host_class_hid_client_local_instance, 0, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

    /* We may need to inform the application
       if a function has been programmed in the system structure.  */
    if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
    {
        
        /* Call system change function.  */
        _ux_system_host ->  ux_system_host_change_function(UX_HID_CLIENT_REMOVAL, hid -> ux_host_class_hid_class, (VOID *) hid_client);
    }

    /* Now free the instance memory.  */
    _ux_utility_memory_free(hid_client -> ux_host_class_hid_client_local_instance);

    /* Return completion status.  */
    return(status);    
}

