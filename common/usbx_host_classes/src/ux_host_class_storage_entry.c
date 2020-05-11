/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** USBX Component                                                        */ 
/**                                                                       */
/**   Storage Class                                                       */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_storage.h"
#include "ux_host_stack.h"

UX_COMPILE_TIME_ASSERT(!UX_OVERFLOW_CHECK_MULC_ULONG(sizeof(UX_HOST_CLASS_STORAGE_MEDIA), UX_HOST_CLASS_STORAGE_MAX_MEDIA), UX_HOST_CLASS_STORAGE_MAX_MEDIA_mul_ovf)

/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_storage_entry                        PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function is the entry point of the storage class. It will be   */ 
/*    called by the USBX stack enumeration module when there is a new     */ 
/*    USB disk on the bus or when the USB disk is removed.                */
/*                                                                        */
/*    Version 2.0 of the storage class only supports USB FAT media and    */ 
/*    not CD-ROM.                                                         */ 
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
/*    _ux_host_class_storage_activate       Activate storage class        */ 
/*    _ux_host_class_storage_deactivate     Deactivate storage class      */ 
/*    _ux_utility_memory_allocate           Allocate memory block         */ 
/*    _ux_utility_memory_free               Free memory block             */
/*    _ux_utility_thread_create             Create storage class thread   */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Host Stack                                                          */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_storage_entry(UX_HOST_CLASS_COMMAND *command)
{

UINT        status;

    /* The command request will tell us we need to do here, either a enumeration
       query, an activation or a deactivation.  */
    switch (command -> ux_host_class_command_request)
    {

    case UX_HOST_CLASS_COMMAND_QUERY:

        /* The query command is used to let the stack enumeration process know if we want to own
           this device or not.  */
        if ((command -> ux_host_class_command_usage == UX_HOST_CLASS_COMMAND_USAGE_CSP) &&
                            (command -> ux_host_class_command_class == UX_HOST_CLASS_STORAGE_CLASS))
            return(UX_SUCCESS);                        
        else            
            return(UX_NO_CLASS_MATCH);                        
                
    case UX_HOST_CLASS_COMMAND_ACTIVATE:

        /* We are assuming the device will mount. If this is the first activation of
           the storage class, we have to fire up one thread for the media insertion
           and acquire some memory for the media array.  */

        /* Allocate some memory for the media structures used by FileX.  */
        if (command -> ux_host_class_command_class_ptr -> ux_host_class_media == UX_NULL)
        {

            /* UX_HOST_CLASS_STORAGE_MAX_MEDIA*sizeof(UX_HOST_CLASS_STORAGE_MEDIA) overflow
             * is checked outside of function.
             */
            command -> ux_host_class_command_class_ptr -> ux_host_class_media =
                        _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, UX_HOST_CLASS_STORAGE_MAX_MEDIA*sizeof(UX_HOST_CLASS_STORAGE_MEDIA));

            /* Check the completion status.  */
            if (command -> ux_host_class_command_class_ptr -> ux_host_class_media == UX_NULL)
                return(UX_MEMORY_INSUFFICIENT);
        }

        /* Allocate a Thread stack and create the thread.  */
        if (command -> ux_host_class_command_class_ptr -> ux_host_class_thread_stack == UX_NULL)
        {

            /* Allocate a Thread stack.  */
            command -> ux_host_class_command_class_ptr -> ux_host_class_thread_stack =  
                        _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, UX_HOST_CLASS_STORAGE_THREAD_STACK_SIZE);

            /* Check the completion status.  */
            if (command -> ux_host_class_command_class_ptr -> ux_host_class_thread_stack == UX_NULL)
                return(UX_MEMORY_INSUFFICIENT);
            
            /* Create the storage class thread.  */
            status =  _ux_utility_thread_create(&command -> ux_host_class_command_class_ptr -> ux_host_class_thread,
                                    "ux_host_storage_thread", _ux_host_class_storage_thread_entry,
                                    (ULONG) (ALIGN_TYPE) command -> ux_host_class_command_class_ptr, 
                                    command -> ux_host_class_command_class_ptr -> ux_host_class_thread_stack,
                                    UX_HOST_CLASS_STORAGE_THREAD_STACK_SIZE, 
                                    UX_HOST_CLASS_STORAGE_THREAD_PRIORITY_CLASS,
                                    UX_HOST_CLASS_STORAGE_THREAD_PRIORITY_CLASS,
                                    TX_NO_TIME_SLICE, TX_DONT_START);
                        
            /* Check the completion status.  */
            if (status != UX_SUCCESS)
            {
                _ux_utility_memory_free(command -> ux_host_class_command_class_ptr -> ux_host_class_thread_stack);
                command -> ux_host_class_command_class_ptr -> ux_host_class_thread_stack = UX_NULL;
                return(UX_THREAD_ERROR);
            }

            UX_THREAD_EXTENSION_PTR_SET(&(command -> ux_host_class_command_class_ptr -> ux_host_class_thread), command -> ux_host_class_command_class_ptr)

            /* Now that the extension pointer has been set, resume the thread.  */
            tx_thread_resume(&command -> ux_host_class_command_class_ptr -> ux_host_class_thread);
        }

        /* The activate command is used when the device inserted has found a parent and
           is ready to complete the enumeration.   */
        status =  _ux_host_class_storage_activate(command);

        /* Return the completion status.  */
        return(status);

    case UX_HOST_CLASS_COMMAND_DEACTIVATE:

        /* The deactivate command is used when the device has been extracted either      
           directly or when its parents has been extracted.  */
        status =  _ux_host_class_storage_deactivate(command);

        /* Return the completion status.  */
        return(status);

    default: 

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, UX_FUNCTION_NOT_SUPPORTED);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        /* Return an error.  */
        return(UX_FUNCTION_NOT_SUPPORTED);
    }   
}

