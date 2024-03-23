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
/**                                                                       */
/** USBX Component                                                        */
/**                                                                       */
/**   Device DFU Class                                                    */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_dfu.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_dfu_uninitialize                   PORTABLE C      */
/*                                                           6.x          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function uninitializes the USB DFU device.                     */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    command                              Pointer to dfu command         */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_thread_delete             Remove storage thread.         */
/*    _ux_utility_memory_free              Free memory used by storage    */
/*    _ux_utility_event_flags_delete       Remove flag event structure    */
/*                                                                        */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    USBX Source Code                                                    */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  xx-xx-xxxx     Mohamed ayed             Initial Version 6.x           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_dfu_uninitialize(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS_DFU                      *dfu;
UX_SLAVE_CLASS                          *class_ptr;


    /* Get the class container.  */
    class_ptr =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    dfu = (UX_SLAVE_CLASS_DFU *) class_ptr -> ux_slave_class_instance;

    /* Sanity check.  */
    if (dfu != UX_NULL)
    {

#if !defined(UX_DEVICE_STANDALONE)

        /* Free resources and return error.  */
        _ux_device_thread_delete(&dfu -> ux_slave_class_dfu_thread);
        _ux_utility_event_flags_delete(&dfu -> ux_slave_class_dfu_event_flags_group);
        _ux_utility_memory_free(dfu -> ux_slave_class_dfu_thread_stack);
#endif

        /* Free the resources.  */
        _ux_utility_memory_free(dfu);
    }

    /* Return completion status.  */
    return(UX_SUCCESS);
}

