#!/bin/bash
##############################################################################
# Copyright (c) 2024 Microsoft Corporation
# Copyright (c) 2026 Eclipse ThreadX contributors
#
# This program and the accompanying materials are made available under the
# terms of the MIT License which is available at
# https://opensource.org/licenses/MIT.
#
# SPDX-License-Identifier: MIT
##############################################################################

set -euo pipefail
export CTEST_PARALLEL_LEVEL=${CTEST_PARALLEL_LEVEL:-4}
export CTEST_REPEAT_FAIL=1
export CC=${CC:-gcc-14}
export GCOV=${GCOV:-gcov-14}
export CMAKE_BUILD_PARALLEL_LEVEL=${CMAKE_BUILD_PARALLEL_LEVEL:-4}
exec "$(dirname "$(realpath "$0")")/../test/cmake/usbx/run.sh" test "$@"
