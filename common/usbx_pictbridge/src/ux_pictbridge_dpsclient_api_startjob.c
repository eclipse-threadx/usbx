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
/**   Pictbridge Application                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_pictbridge.h"
#include "ux_device_class_pima.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_pictbridge_dpsclient_api_start_job              PORTABLE C      */
/*                                                           6.1.12       */
/*                                                                        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function creates the tag lines of the start job function       */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    pictbridge                            Pictbridge instance           */
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
/*    application                                                         */
/*                                                                        */
/**************************************************************************/
UINT _ux_pictbridge_dpsclient_api_start_job(UX_PICTBRIDGE *pictbridge)
{
UINT                                status;
ULONG                               actual_flags;

    /* Prepare the object for capability. */
    status = _ux_pictbridge_dpsclient_input_object_prepare(pictbridge, UX_PICTBRIDGE_OR_START_JOB, 0, 0);

    /* Check status.  */
    if (status != UX_SUCCESS)
        return(status);

    /* We should wait for the host to send a script with the response.  */
    status =  _ux_system_event_flags_get(&pictbridge -> ux_pictbridge_event_flags_group, UX_PICTBRIDGE_EVENT_FLAG_START_JOB,
                                        UX_AND_CLEAR, &actual_flags, UX_PICTBRIDGE_EVENT_TIMEOUT);

    /* Check status.  */
    if (status != UX_SUCCESS)
        return(UX_EVENT_ERROR);

    /* Ensure the flag was set.  */
    if (actual_flags & UX_PICTBRIDGE_EVENT_FLAG_START_JOB)

        /* Return completion status.  */
        return(UX_SUCCESS);

    else

        /* Return completion status.  */
        return(UX_ERROR);
}

