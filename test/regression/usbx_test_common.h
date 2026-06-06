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

#include <stdarg.h>

/* This is how ux_utility_memory_allocate calculates the final memory size. */
static UINT calculate_final_memory_request_size(UINT count, ...)
{

UINT original_memory_size_requested;
UINT memory_size_requested;
UINT total_final_memory_size = 0;


    va_list args;
    va_start(args, count);

    while(count-- > 0)
    {
        original_memory_size_requested = va_arg(args, UINT);

        memory_size_requested = (original_memory_size_requested + UX_ALIGN_MIN) & (UINT)~UX_ALIGN_MIN;
        memory_size_requested += sizeof(UX_MEMORY_BLOCK);

        total_final_memory_size += memory_size_requested;
    }

    total_final_memory_size += sizeof(UX_MEMORY_BLOCK);
    total_final_memory_size += 1;

    return total_final_memory_size;
}
