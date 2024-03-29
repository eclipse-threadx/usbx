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
/**   Device Audio Class                                                  */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_audio.h"
#include "ux_device_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_audio_ioctl                        PORTABLE C      */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function performs certain functions on the audio instance      */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    audio                                 Address of audio class        */
/*                                            instance                    */
/*    ioctl_function                        IOCTL function code           */
/*    parameter                             Parameter for function        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Status                                                              */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  03-08-2023     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.2.1  */
/*                                                                        */
/**************************************************************************/
UINT _ux_device_class_audio_ioctl(UX_DEVICE_CLASS_AUDIO *audio, ULONG ioctl_function,
                                    VOID *parameter)
{

UINT                                                status;
VOID                                                **pptr_parameter;


    /* Let's be optimist ! */
    status = UX_SUCCESS;

    /* The command request will tell us what we need to do here.  */
    switch (ioctl_function)
    {

        case UX_DEVICE_CLASS_AUDIO_IOCTL_GET_ARG:

            /* Properly cast the parameter pointer.  */
            pptr_parameter = (VOID **) parameter;

            /* Save argument.  */
            *pptr_parameter = audio -> ux_device_class_audio_callbacks.ux_device_class_audio_arg;

            break;

        default:

            /* Function not supported. Return an error.  */
            status =  UX_FUNCTION_NOT_SUPPORTED;
    }

    /* Return status to caller.  */
    return(status);

}

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _uxe_device_class_audio_ioctl                       PORTABLE C      */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks errors in ioctl function call.                 */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    audio                                 Address of audio class        */
/*                                            instance                    */
/*    ioctl_function                        IOCTL function code           */
/*    parameter                             Parameter for function        */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Status                                                              */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_device_class_audio_ioctl          Perform IOCTL function        */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  03-08-2023     Chaoqiong Xiao           Initial Version 6.2.1         */
/*                                                                        */
/**************************************************************************/
UINT _uxe_device_class_audio_ioctl(UX_DEVICE_CLASS_AUDIO *audio, ULONG ioctl_function,
                                    VOID *parameter)
{

    /* Sanity check.  */
    if (audio == UX_NULL)
        return(UX_INVALID_PARAMETER);

    /* Dispatch IOCTL commands.  */
    return(_ux_device_class_audio_ioctl(audio, ioctl_function, parameter));
}
