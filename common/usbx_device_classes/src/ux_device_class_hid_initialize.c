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
/**                                                                       */ 
/** USBX Component                                                        */ 
/**                                                                       */
/**   Device HID Class                                                    */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_hid.h"
#include "ux_device_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_device_class_hid_initialize                     PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function initializes the USB HID device.                       */ 
/*    This function is called by the class register function. It is only  */ 
/*    done once.                                                          */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    command                              Pointer to hid command         */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_utility_memory_allocate           Allocate memory               */
/*    _ux_utility_memory_free               Free memory                   */
/*    _ux_utility_thread_create             Create thread                 */
/*    _ux_utility_thread_delete             Delete thread                 */
/*    _ux_utility_event_flags_create        Create event flags group      */
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    USBX Source Code                                                    */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            used UX prefix to refer to  */
/*                                            TX symbols instead of using */
/*                                            them directly,              */
/*                                            resulting in version 6.1    */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_hid_initialize(UX_SLAVE_CLASS_COMMAND *command)
{
                                          
UX_SLAVE_CLASS_HID                      *hid;
UX_SLAVE_CLASS_HID_PARAMETER            *hid_parameter;
UX_SLAVE_CLASS                          *class;
UINT                                    status = UX_SUCCESS;


    /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;

    /* Create an instance of the device hid class.  */
    hid =  _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_SLAVE_CLASS_HID));

    /* Check for successful allocation.  */
    if (hid == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Save the address of the HID instance inside the HID container.  */
    class -> ux_slave_class_instance = (VOID *) hid;

    /* Allocate some memory for the thread stack. */
    class -> ux_slave_class_thread_stack =  
            _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, UX_THREAD_STACK_SIZE);
    
    /* Check for successful allocation.  */
    if (class -> ux_slave_class_thread_stack == UX_NULL)
        status = UX_MEMORY_INSUFFICIENT;

    /* This instance needs to be running in a different thread. So start
       a new thread. We pass a pointer to the class to the new thread.  This thread
       does not start until we have a instance of the class. */
    if (status == UX_SUCCESS)
        status =  _ux_utility_thread_create(&class -> ux_slave_class_thread, "ux_slave_hid_thread", 
                    _ux_device_class_hid_interrupt_thread,
                    (ULONG) (ALIGN_TYPE) class, (VOID *) class -> ux_slave_class_thread_stack,
                    UX_THREAD_STACK_SIZE, UX_THREAD_PRIORITY_CLASS,
                    UX_THREAD_PRIORITY_CLASS, UX_NO_TIME_SLICE, UX_DONT_START);

    /* Check the creation of this thread.  */
    if (status == UX_SUCCESS)
    {

        UX_THREAD_EXTENSION_PTR_SET(&(class -> ux_slave_class_thread), class)

        /* Get the pointer to the application parameters for the hid class.  */
        hid_parameter =  command -> ux_slave_class_command_parameter;

        /* Store all the application parameter information about the report.  */
        hid -> ux_device_class_hid_report_address             = hid_parameter -> ux_device_class_hid_parameter_report_address;
        hid -> ux_device_class_hid_report_length              = hid_parameter -> ux_device_class_hid_parameter_report_length;
        hid -> ux_device_class_hid_report_id                  = hid_parameter -> ux_device_class_hid_parameter_report_id;

        /* Store the callback function.  */
        hid -> ux_device_class_hid_callback                   = hid_parameter -> ux_device_class_hid_parameter_callback;
        hid -> ux_device_class_hid_get_callback               = hid_parameter -> ux_device_class_hid_parameter_get_callback;

        /* Create the event array.  */
        hid -> ux_device_class_hid_event_array =  _ux_utility_memory_allocate_mulc_safe(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_SLAVE_CLASS_HID_EVENT), UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE);

        /* Check for successful allocation.  */
        if (hid -> ux_device_class_hid_event_array != UX_NULL)
        {

            /* Initialize the head and tail of the notification round robin buffers. 
               At first, the head and tail are pointing to the beginning of the array.  */
            hid -> ux_device_class_hid_event_array_head =  hid -> ux_device_class_hid_event_array;
            hid -> ux_device_class_hid_event_array_tail =  hid -> ux_device_class_hid_event_array;
            hid -> ux_device_class_hid_event_array_end  =  hid -> ux_device_class_hid_event_array + UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE;

            /* Store the start and stop signals if needed by the application.  */
            hid -> ux_slave_class_hid_instance_activate = hid_parameter -> ux_slave_class_hid_instance_activate;
            hid -> ux_slave_class_hid_instance_deactivate = hid_parameter -> ux_slave_class_hid_instance_deactivate;

            /* By default no event wait timeout.  */
            hid -> ux_device_class_hid_event_wait_timeout = UX_WAIT_FOREVER;

            /* Create a event flag group for the hid class to synchronize with the event interrupt thread.  */
            status =  _ux_utility_event_flags_create(&hid -> ux_device_class_hid_event_flags_group, "ux_device_class_hid_event_flag");

            /* Check status.  */
            if (status == UX_SUCCESS)
                return(status);

            /* It's event error. */
            status =  UX_EVENT_ERROR;
        }
        else
            status =  UX_MEMORY_INSUFFICIENT;

        /* Delete thread.  */
        _ux_utility_thread_delete(&class -> ux_slave_class_thread);
    }
    else
        status = (UX_THREAD_ERROR);

    /* Free stack. */
    if (class -> ux_slave_class_thread_stack)
        _ux_utility_memory_free(class -> ux_slave_class_thread_stack);

    /* Unmount instance. */
    class -> ux_slave_class_instance =  UX_NULL;

    /* Free HID instance. */
    _ux_utility_memory_free(hid);

    /* Return completion status.  */
    return(status);
}

