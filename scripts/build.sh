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
export CC=${CC:-gcc-14}
export GCOV=${GCOV:-gcov-14}
export CMAKE_BUILD_PARALLEL_LEVEL=${CMAKE_BUILD_PARALLEL_LEVEL:-4}
"$(dirname "$(realpath "$0")")/../test/cmake/usbx/run.sh" build "$@"
echo "Checking runner and coverage failure handling in isolated fixtures."
python3 "$(dirname "$(realpath "$0")")/../test/cmake/usbx/test_runner.py"
