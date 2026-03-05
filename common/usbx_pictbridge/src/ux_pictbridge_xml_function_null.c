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


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_pictbridge_xml_function_null                    PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is a repository when a tag is not treated.            */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    pictbridge                             Pictbridge instance          */
/*    input_variable                         Pointer to variable          */
/*    input_string                           Pointer to string            */
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
/*    _ux_pictbridge_object_parse                                         */
/*                                                                        */
/**************************************************************************/
UINT  _ux_pictbridge_xml_function_null(UX_PICTBRIDGE *pictbridge, UCHAR *input_variable, UCHAR *input_string,
                                        UCHAR *xml_parameter)
{

    UX_PARAMETER_NOT_USED(input_string);
    UX_PARAMETER_NOT_USED(input_variable);
    UX_PARAMETER_NOT_USED(pictbridge);
    UX_PARAMETER_NOT_USED(xml_parameter);

    /* This function never fails.  */
    return(UX_SUCCESS);
}

