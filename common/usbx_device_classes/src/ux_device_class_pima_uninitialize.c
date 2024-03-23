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
/**   Device PIMA Class                                                   */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_pima.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_pima_uninitialize                  PORTABLE C      */
/*                                                           6.x          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function uninitializes the USB pima device.                    */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    command                              Pointer to pima command        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_thread_delete             Remove storage thread.         */
/*    _ux_utility_memory_free              Free memory used by storage    */
/*    _ux_utility_semaphore_delete         Remove semaphore structure     */
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
/*  xx-xx-xxxx     Chaoqiong Xiao           Initial Version 6.x           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_pima_uninitialize(UX_SLAVE_CLASS_COMMAND *command)
{

  UX_SLAVE_CLASS_PIMA     *pima;
  UX_SLAVE_CLASS          *class_ptr;


  /* Get the class container.  */
  class_ptr =  command -> ux_slave_class_command_class_ptr;

  /* Get the class instance in the container.  */
  pima = (UX_SLAVE_CLASS_PIMA *) class_ptr -> ux_slave_class_instance;

  /* Sanity check.  */
  if (pima != UX_NULL)
  {

#if !defined(UX_DEVICE_STANDALONE)

    /* Remove PIMA thread.  */
    _ux_device_thread_delete(&class_ptr -> ux_slave_class_thread);

    /* Remove the thread used by PIMA.  */
    _ux_utility_memory_free(class_ptr -> ux_slave_class_thread_stack);

    /* Remove PIMA interrupt thread.  */
    _ux_device_thread_delete(&pima -> ux_device_class_pima_interrupt_thread);

    /* Remove the interrupt semaphore used by PIMA.  */
    _ux_device_semaphore_delete(&pima -> ux_device_class_pima_interrupt_thread_semaphore);

    /* Remove the interrupt thread used by PIMA.  */
    _ux_utility_memory_free(pima -> ux_device_class_pima_interrupt_thread_stack );
#endif

#if UX_DEVICE_ENDPOINT_BUFFER_OWNER == 1
    _ux_utility_memory_free(pima -> ux_device_class_pima_endpoint_buffer);
#endif
  }

  /* Free the resources.  */
  _ux_utility_memory_free(pima);

  /* Return completion status.  */
  return(UX_SUCCESS);
}

