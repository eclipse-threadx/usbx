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
/**   Device RNDIS Class                                                  */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_rndis.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_rndis_uninitialize                 PORTABLE C      */
/*                                                           6.xx         */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function deinitializes the resources for the specified RNDIS   */
/*    instance.                                                           */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    command                               Pointer to rndis command      */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_mutex_delete               Delete mutex                  */
/*    _ux_device_thread_delete              Delete thread                 */
/*    _ux_utility_memory_free               Free memory                   */
/*    _ux_utility_event_flags_delete        Delete event flags            */
/*    _ux_device_semaphore_delete           Delete semaphore              */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Device RNDIS Class                                                  */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  xx-xx-xxxx     Mohamed ayed             Initial Version 6.x           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_rndis_uninitialize(UX_SLAVE_CLASS_COMMAND *command)
{

UX_SLAVE_CLASS_RNDIS                    *rndis;
UX_SLAVE_CLASS                          *class_ptr;


    /* Get the class container.  */
    class_ptr =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    rndis = (UX_SLAVE_CLASS_RNDIS *) class_ptr -> ux_slave_class_instance;

    /* Sanity check.  */
    if (rndis != UX_NULL)
    {

        /* Deinitialize resources. We do not check if they have been allocated
           because if they weren't, the class register (called by the application)
           would have failed.  */

#if !defined(UX_DEVICE_STANDALONE)

        /* Delete the queue mutex.  */
        _ux_device_mutex_delete(&rndis -> ux_slave_class_rndis_mutex);

        /* Delete bulk out thread .  */
        _ux_device_thread_delete(&rndis -> ux_slave_class_rndis_bulkout_thread);

        /* Free bulk out thread stack.  */
        _ux_utility_memory_free(rndis -> ux_slave_class_rndis_bulkout_thread_stack);

        /* Delete interrupt thread.  */
        _ux_device_thread_delete(&rndis -> ux_slave_class_rndis_interrupt_thread);

        /* Free interrupt thread stack.  */
        _ux_utility_memory_free(rndis -> ux_slave_class_rndis_interrupt_thread_stack);

        /* Delete bulk in thread.  */
        _ux_device_thread_delete(&rndis -> ux_slave_class_rndis_bulkin_thread);

        /* Free bulk in thread stack.  */
        _ux_utility_memory_free(rndis -> ux_slave_class_rndis_bulkin_thread_stack);

        /* Delete the interrupt thread sync event flags group.  */
        _ux_device_event_flags_delete(&rndis -> ux_slave_class_rndis_event_flags_group);

#endif

        /* Free the resources.  */
#if UX_DEVICE_ENDPOINT_BUFFER_OWNER == 1
        _ux_utility_memory_free(rndis -> ux_device_class_rndis_endpoint_buffer);
#endif
        _ux_utility_memory_free(rndis);
    }

    /* Return completion status.  */
    return(UX_SUCCESS);
}
