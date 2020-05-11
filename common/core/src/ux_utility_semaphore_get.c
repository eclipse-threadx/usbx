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
/**   Utility                                                             */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_utility_semaphore_get                           PORTABLE C      */ 
/*                                                           6.0          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function gets a semaphore signal.                              */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    semaphore                             Semaphore to get signal from  */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    tx_thread_identify                    ThreadX identify thread       */
/*    tx_thread_info_get                    ThreadX get thread info       */
/*    tx_semaphore_get                      ThreadX semaphore get         */
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    USBX Components                                                     */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*                                                                        */
/**************************************************************************/
UINT  _ux_utility_semaphore_get(TX_SEMAPHORE *semaphore, ULONG semaphore_signal)
{

UINT        status;
TX_THREAD   *my_thread;
CHAR        *name;
UINT        state;
ULONG       run_count;
UINT        priority;
UINT        preemption_threshold;
ULONG       time_slice;
TX_THREAD   *next_thread;
TX_THREAD   *suspended_thread;

    /* Call TX to know my own tread.  */
    my_thread = tx_thread_identify();

    /* Retrieve information about the previously created thread "my_thread." */
    tx_thread_info_get(my_thread, &name, &state, &run_count,
                       &priority, &preemption_threshold,
                       &time_slice, &next_thread,&suspended_thread);

    /* Is this the lowest priority thread in the system trying to use TX services ? */
    if (priority > _ux_system -> ux_system_thread_lowest_priority)
    {

        /* We need to remember this thread priority.  */
        _ux_system -> ux_system_thread_lowest_priority = priority;
        
    }

    /* Get ThreadX semaphore instance.  */
    status =  tx_semaphore_get(semaphore, semaphore_signal);

    /* Return completion status.  */
    return(status);
}

