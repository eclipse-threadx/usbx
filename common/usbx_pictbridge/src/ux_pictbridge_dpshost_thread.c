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
/**   Pictbridge Application                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_pictbridge.h"
#include "ux_host_class_pima.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_pictbridge_dpshost_thread                       PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This is the Pictbridge dpshost thread that receives and execute     */ 
/*    notifications from the dpsclient.                                   */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    pictbridge                             Pictbridge instance          */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    user application                                                    */ 
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
VOID  _ux_pictbridge_dpshost_thread(ULONG parameter)
{

UX_PICTBRIDGE           *pictbridge;
UX_PICTBRIDGE_EVENT     *pictbridge_event;
ULONG                   actual_flags;
UINT                    status;

    /* Cast the parameter passed in the thread into the pictbridge pointer.  */
    UX_THREAD_EXTENSION_PTR_GET(pictbridge, UX_PICTBRIDGE, parameter)

    /* Loop forever waiting for changes signaled through the semaphore. */     
    while (1)
    {   

        /* Wait for the semaphore to be put by the root hub or a regular hub.  */
        _ux_utility_semaphore_get(&pictbridge -> ux_pictbridge_notification_semaphore, UX_WAIT_FOREVER);

        /* Check if there is an event. Normally there should be at least one since we got awaken. */
        if (pictbridge -> ux_pictbridge_event_array_tail != pictbridge -> ux_pictbridge_event_array_head)
        {
    
            /* Get the current event tail.  */
            pictbridge_event = pictbridge -> ux_pictbridge_event_array_tail;
            
            /* Compute the next entry in the event array.  */
            if ((pictbridge -> ux_pictbridge_event_array_tail + 1) == pictbridge -> ux_pictbridge_event_array_end)
    
                /* Start at the beginning of the list.  */
                pictbridge -> ux_pictbridge_event_array_tail = pictbridge -> ux_pictbridge_event_array;
            else

                /* Point to the next entry in the event array.  */
                pictbridge -> ux_pictbridge_event_array_tail = pictbridge -> ux_pictbridge_event_array_tail + 1;        

            /* Analyze the event type.  */
            switch (pictbridge_event -> ux_pictbridge_event_code)
            {

                /* The DSC client is informing us that an object is ready.  */
                case UX_PICTBRIDGE_EC_OBJECT_ADDED                  :
                case UX_PICTBRIDGE_EC_REQUEST_OBJECT_TRANSFER       :

                    /* Check the state machine.  */
                    if (pictbridge -> ux_pictbridge_host_client_state_machine != UX_PICTBRIDGE_STATE_MACHINE_IDLE)
                    {

                        /* Set the state machine to Client Request pending.  */
                        pictbridge -> ux_pictbridge_host_client_state_machine |= UX_PICTBRIDGE_STATE_MACHINE_CLIENT_REQUEST_PENDING;

                        /* Wait for the host pending request to be completed.  */
                        status =  _ux_utility_event_flags_get(&pictbridge -> ux_pictbridge_event_flags_group, 
                                                UX_PICTBRIDGE_EVENT_FLAG_STATE_MACHINE_READY, 
                                                UX_AND_CLEAR, &actual_flags, UX_PICTBRIDGE_EVENT_TIMEOUT);

                        /* Reset the state machine to not Host Request pending.  */
                        pictbridge -> ux_pictbridge_host_client_state_machine &= (UINT)~UX_PICTBRIDGE_STATE_MACHINE_CLIENT_REQUEST_PENDING;

                        /* Check status.  */
                        if (status != UX_SUCCESS)
                            break;


                    }
                    
                    /* Change the state machine to client request being executed. */
                    pictbridge -> ux_pictbridge_host_client_state_machine |= UX_PICTBRIDGE_STATE_MACHINE_CLIENT_REQUEST;

                    /* Obtain the object and execute the XML request script.  */
                    _ux_pictbridge_dpshost_object_get(pictbridge, pictbridge_event -> ux_pictbridge_event_parameter_1);

                    /* Do we have a pending client event ?  */
                    if (pictbridge -> ux_pictbridge_host_client_state_machine & UX_PICTBRIDGE_STATE_MACHINE_CLIENT_REQUEST_PENDING)
                    {

                        /* Yes, so we need to set the event that advertise the completion of the host request.  */
                        status =  _ux_utility_event_flags_set(&pictbridge -> ux_pictbridge_event_flags_group, 
                                                UX_PICTBRIDGE_EVENT_FLAG_STATE_MACHINE_READY, 
                                                UX_AND);
                        /* Check status.  */
                        if (status != UX_SUCCESS)
                            break;


                    }

                    /* Change state machine to idle.  */
                    pictbridge -> ux_pictbridge_host_client_state_machine &=  (UINT)~UX_PICTBRIDGE_STATE_MACHINE_CLIENT_REQUEST;

                    /* Check to see if we had a configureprint service request.  */
                    if (pictbridge -> ux_pictbridge_input_request == UX_PICTBRIDGE_IR_GET_CAPABILITY)
                    {
                        
                        /* Parse the tag code that was memorized during the input.  */
                        if (pictbridge -> ux_pictbridge_input_tags & UX_PICTBRIDGE_IR_GC_CROPPINGS)

                            /* We did have a request, notify the camera of our status.  */
                            _ux_pictbridge_dpshost_input_object_send(pictbridge, UX_PICTBRIDGE_IR_NOTIFY_DEVICE_STATUS);
                        
                    }
                    break;

                case UX_PICTBRIDGE_EC_START_JOB                     :
                    
                    /* We have received an order to print one or more picture.  */
                    _ux_pictbridge_dpshost_startjob(pictbridge);
                
                default                                             :
                    break;
            }            
        }
    }    
}

