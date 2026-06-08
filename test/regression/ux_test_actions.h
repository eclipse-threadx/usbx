/***************************************************************************/
/* Copyright (c) 2024 Microsoft Corporation                                */
/* Copyright (c) 2026 Eclipse ThreadX contributors                         */
/*                                                                         */
/* This program and the accompanying materials are made available under    */
/* the terms of the MIT License which is available at                      */
/* https://opensource.org/licenses/MIT.                                    */
/*                                                                         */
/* SPDX-License-Identifier: MIT                                            */
/***************************************************************************/

#ifndef UX_TEST_ACTIONS_H
#define UX_TEST_ACTIONS_H

#include "ux_test.h"

static UX_TEST_ACTION create_error_match_action(UINT system_level, UINT system_context, UINT error_code)
{

UX_TEST_ACTION action = { 0 };


    action.usbx_function = UX_TEST_OVERRIDE_ERROR_CALLBACK;
    action.system_level = system_level;
    action.system_context = system_context;
    action.error_code = error_code;
    action.no_return = 1;

    return action;
}

#endif