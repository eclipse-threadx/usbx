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
/*    _ux_device_class_cdc_acm_uninitialize               PORTABLE C      */ 
/*                                                           6.1.6        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function uninitialize the cdc-acm class.                       */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    command                               Pointer to a class command    */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_utility_mutex_delete              Delete Mutex                  */ 
/*    _ux_utility_memory_free               Free used local memory        */ 
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
/*  04-02-2021     Chaoqiong Xiao           Modified comment(s),          */
/*                                            added macro to disable      */
/*                                            transmission support,       */
/*                                            moved transmission resource */
/*                                            free to here (uninit),      */
/*                                            resulting in version 6.1.6  */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_cdc_acm_uninitialize(UX_SLAVE_CLASS_COMMAND *command)
{
                                          
UX_SLAVE_CLASS_CDC_ACM      *cdc_acm;
UX_SLAVE_CLASS              *class;

    /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;

    /* Get the class instance in the container.  */
    cdc_acm = (UX_SLAVE_CLASS_CDC_ACM *) class -> ux_slave_class_instance;

    /* Sanity check.  */
    if (cdc_acm != UX_NULL)
    {

        /* Delete the IN endpoint mutex.  */
        _ux_utility_mutex_delete(&cdc_acm -> ux_slave_class_cdc_acm_endpoint_in_mutex);

        /* Out Mutex. */
        _ux_utility_mutex_delete(&cdc_acm -> ux_slave_class_cdc_acm_endpoint_out_mutex);

#ifndef UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE

        /* Free resources and return error.  */
        _ux_utility_thread_delete(&cdc_acm -> ux_slave_class_cdc_acm_bulkin_thread);
        _ux_utility_thread_delete(&cdc_acm -> ux_slave_class_cdc_acm_bulkout_thread);
        _ux_utility_event_flags_delete(&cdc_acm -> ux_slave_class_cdc_acm_event_flags_group);
        _ux_utility_memory_free(cdc_acm -> ux_slave_class_cdc_acm_bulkout_thread_stack);
#endif

        /* Free the resources.  */
        _ux_utility_memory_free(cdc_acm);

    }
            
    /* Return completion status.  */
    return(UX_SUCCESS);
}

