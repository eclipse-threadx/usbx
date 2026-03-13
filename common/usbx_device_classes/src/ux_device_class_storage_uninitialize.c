/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 * Copyright (c) 2026-present Eclipse ThreadX contributors
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
/**   Device Storage Class                                                */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_storage.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_storage_uninitialize               PORTABLE C      */
/*                                                           6.3.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function deinitializes the USB storage device.                 */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    command                               Pointer to storage command    */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_utility_memory_free               Free memory                   */
/*    _ux_device_thread_delete              Delete thread                 */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Device Storage Class                                                */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_storage_uninitialize(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS_STORAGE                  *storage;
UX_SLAVE_CLASS                          *class_ptr;

    /* Get the class container.  */
    class_ptr =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    storage = (UX_SLAVE_CLASS_STORAGE *) class_ptr -> ux_slave_class_instance;

    /* Sanity check.  */
    if (storage != UX_NULL)
    {

        /* Remove STORAGE thread.  */
        _ux_device_thread_delete(&class_ptr -> ux_slave_class_thread);

#if !(defined(UX_DEVICE_STANDALONE) || defined(UX_STANDALONE))
        /* Remove the thread used by STORAGE.  */
        _ux_utility_memory_free(class_ptr -> ux_slave_class_thread_stack);
#endif

#if UX_DEVICE_ENDPOINT_BUFFER_OWNER == 1
        _ux_utility_memory_free(storage -> ux_device_class_storage_endpoint_buffer);
#endif

        /* Free the resources.  */
        _ux_utility_memory_free(storage);
    }

    /* Return completion status.  */
    return(UX_SUCCESS);
}

