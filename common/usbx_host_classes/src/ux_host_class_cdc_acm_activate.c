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
/**   CDC ACM Class                                                       */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_cdc_acm.h"
#include "ux_host_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_host_class_cdc_acm_activate                     PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function creates the ACM instance, configure the device ...    */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    command                                ACM  class command pointer   */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_host_class_cdc_acm_configure        Configure cdc_acm class     */
/*    _ux_host_class_cdc_acm_endpoints_get    Get endpoints of cdc_acm    */
/*    _ux_host_class_cdc_acm_ioctl            IOCTL function for ACM      */
/*    _ux_host_stack_class_instance_destroy   Destroy the class instance  */
/*    _ux_host_stack_endpoint_transfer_abort  Abort transfer              */
/*    _ux_utility_memory_allocate             Allocate memory block       */
/*    _ux_utility_memory_free                 Free memory                 */
/*    _ux_utility_semaphore_create            Create cdc_acm semaphore    */
/*    _ux_utility_semaphore_delete            Delete semaphore            */
/*    _ux_utility_delay_ms                    Delay                       */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    _ux_host_class_cdc_acm_entry          Entry of cdc_acm class        */
/*    _ux_utility_delay_ms                  Delay ms                      */
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
UINT  _ux_host_class_cdc_acm_activate(UX_HOST_CLASS_COMMAND *command)
{

UX_INTERFACE                        *interface;
UX_HOST_CLASS_CDC_ACM               *cdc_acm;
UX_HOST_CLASS_CDC_ACM_LINE_CODING   line_coding;
UX_HOST_CLASS_CDC_ACM_LINE_STATE    line_state;
UINT                                status;

    /* The CDC ACM class is always activated by the interface descriptor and not the
       device descriptor.  */
    interface =  (UX_INTERFACE *) command -> ux_host_class_command_container;

    /* Obtain memory for this class instance.  */
    cdc_acm =  _ux_utility_memory_allocate(UX_NO_ALIGN, UX_CACHE_SAFE_MEMORY, sizeof(UX_HOST_CLASS_CDC_ACM));

    /* Instance creation fail. */
    if (cdc_acm == UX_NULL)

        /* Memory allocation fail. */
        return(UX_MEMORY_INSUFFICIENT);

    /* Create the semaphore to protect 2 threads from accessing the same acm instance.  */
    status =  _ux_utility_semaphore_create(&cdc_acm -> ux_host_class_cdc_acm_semaphore, "ux_host_class_cdc_acm_semaphore", 1);
    if (status != UX_SUCCESS)
    {

        /* Free instance memory. */
        _ux_utility_memory_free(cdc_acm);

        /* Semaphore creation error. */
        return(UX_SEMAPHORE_ERROR);
    }

    /* Store the class container into this instance.  */
    cdc_acm -> ux_host_class_cdc_acm_class =  command -> ux_host_class_command_class_ptr;

    /* Store the interface container into the cdc_acm class instance.  */
    cdc_acm -> ux_host_class_cdc_acm_interface =  interface;

    /* Store the device container into the cdc_acm class instance.  */
    cdc_acm -> ux_host_class_cdc_acm_device =  interface -> ux_interface_configuration -> ux_configuration_device;

    /* This instance of the device must also be stored in the interface container.  */
    interface -> ux_interface_class_instance =  (VOID *) cdc_acm;

    /* Create this class instance.  */
    _ux_host_stack_class_instance_create(cdc_acm -> ux_host_class_cdc_acm_class, (VOID *) cdc_acm);

    /* Configure the cdc_acm.  */
    status =  _ux_host_class_cdc_acm_configure(cdc_acm);

    /* If we are done success, go on to get endpoints.  */
    if (status == UX_SUCCESS)

        /* Get the cdc_acm endpoint(s). Depending on the interface type, we will need to search for
           Bulk Out and Bulk In endpoints and the optional interrupt endpoint.  */
        status =  _ux_host_class_cdc_acm_endpoints_get(cdc_acm);

    /* If we are done success, go on to mount interface.  */
    if (status == UX_SUCCESS)
    {
        /* Mark the cdc_acm as mounting now.  Both interfaces need to be mounting. */
        cdc_acm -> ux_host_class_cdc_acm_state =  UX_HOST_CLASS_INSTANCE_MOUNTING;

        /* If we have the Control Class, we have to configure the speed, parity ... */
        if (cdc_acm -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
        {

            /* We need to wait for some device to settle. The Radicom USB Modem is an example of
               these device who fail the first Set_Line_Coding command if sent too quickly.
               The timing does not have to be precise so we use the thread sleep function.
               The default sleep value is 1 seconds.  */
            _ux_utility_delay_ms(UX_HOST_CLASS_CDC_ACM_DEVICE_INIT_DELAY);

            /* Do a GET_LINE_CODING first.  */
            status = _ux_host_class_cdc_acm_ioctl(cdc_acm, UX_HOST_CLASS_CDC_ACM_IOCTL_GET_LINE_CODING, (VOID *) &line_coding);

            /* If we are done success, go on.  */
            if (status == UX_SUCCESS)
            {

                /* Set the default values to the device, first line coding.  */
                line_coding.ux_host_class_cdc_acm_line_coding_dter      = UX_HOST_CLASS_CDC_ACM_LINE_CODING_DEFAULT_RATE;
                line_coding.ux_host_class_cdc_acm_line_coding_stop_bit  = UX_HOST_CLASS_CDC_ACM_LINE_CODING_STOP_BIT_0;
                line_coding.ux_host_class_cdc_acm_line_coding_parity    = UX_HOST_CLASS_CDC_ACM_LINE_CODING_PARITY_NONE;
                line_coding.ux_host_class_cdc_acm_line_coding_data_bits = UX_HOST_CLASS_CDC_ACM_LINE_CODING_DEFAULT_DATA_BIT;
                status = _ux_host_class_cdc_acm_ioctl(cdc_acm, UX_HOST_CLASS_CDC_ACM_IOCTL_SET_LINE_CODING, (VOID *) &line_coding);
            }

            /* If we are done success, go on.  */
            if (status == UX_SUCCESS)
            {

                /* Set the default values to the device, line state.  */
                line_state.ux_host_class_cdc_acm_line_state_rts       = 1;
                line_state.ux_host_class_cdc_acm_line_state_dtr       = 1;
                status = _ux_host_class_cdc_acm_ioctl(cdc_acm, UX_HOST_CLASS_CDC_ACM_IOCTL_SET_LINE_STATE, (VOID *) &line_state);
            }

            /* If we are done success, go on.  */
            if (status == UX_SUCCESS)
            {

                /* Get the capabilities of the device. We need to know if the commands are multiplexed over the comm
                interface or the data interface.  */
                status =  _ux_host_class_cdc_acm_capabilities_get(cdc_acm);
            }
        }

        /* If we are done success, go on.  */
        if (status == UX_SUCCESS)
        {
            /* Mark the cdc_acm as live now.  Both interfaces need to be live. */
            cdc_acm -> ux_host_class_cdc_acm_state =  UX_HOST_CLASS_INSTANCE_LIVE;

            /* If all is fine and the device is mounted, we may need to inform the application
               if a function has been programmed in the system structure.  */
            if (_ux_system_host -> ux_system_host_change_function != UX_NULL)
            {

                /* Call system change function.  */
                _ux_system_host ->  ux_system_host_change_function(UX_DEVICE_INSERTION, cdc_acm -> ux_host_class_cdc_acm_class, (VOID *) cdc_acm);
            }

            /* If trace is enabled, insert this event into the trace buffer.  */
            UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_CLASS_CDC_ACM_ACTIVATE, cdc_acm, 0, 0, 0, UX_TRACE_HOST_CLASS_EVENTS, 0, 0)

            /* If trace is enabled, register this object.  */
            UX_TRACE_OBJECT_REGISTER(UX_TRACE_HOST_OBJECT_TYPE_INTERFACE, cdc_acm, 0, 0, 0)

            /* We are done success. */
            return(UX_SUCCESS);
        }
    }

    /* On error case, it's possible data buffer allocated for interrupt endpoint and transfer started, stop and free it.  */
    if (cdc_acm -> ux_host_class_cdc_acm_interrupt_endpoint && 
        cdc_acm -> ux_host_class_cdc_acm_interrupt_endpoint -> ux_endpoint_transfer_request.ux_transfer_request_data_pointer)
    {

        /* The first transfer request has already been initiated. Abort it.  */
        _ux_host_stack_endpoint_transfer_abort(cdc_acm -> ux_host_class_cdc_acm_interrupt_endpoint);

        /* Free the memory for the data pointer.  */
        _ux_utility_memory_free(cdc_acm -> ux_host_class_cdc_acm_interrupt_endpoint -> ux_endpoint_transfer_request.ux_transfer_request_data_pointer);
    }

    /* Destroy the instance.  */
    _ux_host_stack_class_instance_destroy(cdc_acm -> ux_host_class_cdc_acm_class, (VOID *) cdc_acm);

    /* Destroy the semaphore.  */
    _ux_utility_semaphore_delete(&cdc_acm -> ux_host_class_cdc_acm_semaphore);

    /* Unmount instance. */
    interface -> ux_interface_class_instance = UX_NULL;

    /* Free instance. */
    _ux_utility_memory_free(cdc_acm);

    return(status);
}

