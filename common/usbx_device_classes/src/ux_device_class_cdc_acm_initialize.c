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
/*    _ux_device_class_cdc_acm_initialize                 PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function initializes the USB CDC device.                       */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    command                               Pointer to cdc_acm command    */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_utility_memory_allocate           Allocate memory               */ 
/*    _ux_utility_memory_free               Free memory                   */ 
/*    _ux_utility_mutex_create              Create mutex                  */ 
/*    _ux_utility_mutex_delete              Delete mutex                  */ 
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
/*                                            resulting in version 6.1    */
/*                                                                        */
/**************************************************************************/
UINT  _ux_device_class_cdc_acm_initialize(UX_SLAVE_CLASS_COMMAND *command)
{
                                          
UX_SLAVE_CLASS_CDC_ACM                  *cdc_acm;
UX_SLAVE_CLASS_CDC_ACM_PARAMETER        *cdc_acm_parameter;
UX_SLAVE_CLASS                          *class;
UINT                                    status;

    /* Get the class container.  */
    class =  command -> ux_slave_class_command_class_ptr;

    /* Create an instance of the device cdc_acm class.  */
    cdc_acm =  _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_SLAVE_CLASS_CDC_ACM));

    /* Check for successful allocation.  */
    if (cdc_acm == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Save the address of the CDC instance inside the CDC container.  */
    class -> ux_slave_class_instance = (VOID *) cdc_acm;

    /* Get the pointer to the application parameters for the cdc_acm class.  */
    cdc_acm_parameter =  command -> ux_slave_class_command_parameter;

    /* Store the start and stop signals if needed by the application.  */
    cdc_acm -> ux_slave_class_cdc_acm_parameter.ux_slave_class_cdc_acm_instance_activate = cdc_acm_parameter -> ux_slave_class_cdc_acm_instance_activate;
    cdc_acm -> ux_slave_class_cdc_acm_parameter.ux_slave_class_cdc_acm_instance_deactivate = cdc_acm_parameter -> ux_slave_class_cdc_acm_instance_deactivate;
    cdc_acm -> ux_slave_class_cdc_acm_parameter.ux_slave_class_cdc_acm_parameter_change = cdc_acm_parameter -> ux_slave_class_cdc_acm_parameter_change;

    /* Create the Mutex for each endpoint as multiple threads cannot access each pipe at the same time.  */
    status =  _ux_utility_mutex_create(&cdc_acm -> ux_slave_class_cdc_acm_endpoint_in_mutex, "ux_slave_class_cdc_acm_in_mutex");

    /* Check Mutex creation error.  */
    if(status != UX_SUCCESS)
    {

        /* Free the resources.  */
        _ux_utility_memory_free(cdc_acm);
        
        /* Return fatal error.  */
        return(UX_MUTEX_ERROR);
    }        

    /* Out Mutex. */
    status =  _ux_utility_mutex_create(&cdc_acm -> ux_slave_class_cdc_acm_endpoint_out_mutex, "ux_slave_class_cdc_acm_out_mutex");

    /* Check Mutex creation error.  */
    if(status != UX_SUCCESS)
    {

        /* Delete the endpoint IN mutex.  */
        _ux_utility_mutex_delete(&cdc_acm -> ux_slave_class_cdc_acm_endpoint_in_mutex);

        /* Free the resources.  */
        _ux_utility_memory_free(cdc_acm);
        
        /* Return fatal error.  */
        return(UX_MUTEX_ERROR);
    }        
    
    /* Update the line coding fields with default values.  */
    cdc_acm -> ux_slave_class_cdc_acm_baudrate  =  UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_BAUDRATE;
    cdc_acm -> ux_slave_class_cdc_acm_stop_bit  =  UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_STOP_BIT;
    cdc_acm -> ux_slave_class_cdc_acm_parity    =  UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_PARITY;
    cdc_acm -> ux_slave_class_cdc_acm_data_bit  =  UX_SLAVE_CLASS_CDC_ACM_LINE_CODING_DATA_BIT;

    /* Return completion status.  */
    return(UX_SUCCESS);
}

