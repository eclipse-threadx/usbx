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
/**   Storage Class                                                       */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_storage.h"
#include "ux_host_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_host_class_storage_sense_code_translate         PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will perform a conversion between the SCSI error      */
/*    codes and error codes expected by FileX.                            */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    storage                               Pointer to storage class      */
/*    status                                Status to convert             */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Converted Status                                                    */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Storage Class                                                       */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_storage_sense_code_translate(UX_HOST_CLASS_STORAGE *storage, UINT status)
{

    UX_PARAMETER_NOT_USED(storage);

    /* Return status.  */
    return(status);
}

