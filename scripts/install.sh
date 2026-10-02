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

retry() {
    local attempt
    for attempt in 1 2 3; do
        if "$@"; then
            return 0
        fi
        if [ "$attempt" -lt 3 ]; then
            sleep $((attempt * 5))
        fi
    done
    return 1
}

apt_options=(-o Acquire::Retries=3 -o DPkg::Lock::Timeout=60)
if ! retry sudo timeout 150 apt-get "${apt_options[@]}" update; then
    echo "Package index update failed; package installation will verify availability." >&2
fi
retry sudo timeout 150 apt-get "${apt_options[@]}" install -y \
    cmake gcc-14 gcc-14-multilib git ninja-build python3-venv \
    unifdef p7zip-full tofrodos gawk

venv_dir="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/usbx-ci-venv"
python3 -m venv "$venv_dir"
retry timeout 120 "$venv_dir/bin/python" -m pip install \
    --retries 3 --timeout 30 gcovr==8.6

cc="${CC:-gcc-14}"
gcov="${GCOV:-gcov-14}"
cc_version=$("$cc" -dumpfullversion)
gcov_version=$("$gcov" --version | sed -n '1{s/.* \([0-9][0-9]*\.[0-9][0-9]*\(\.[0-9][0-9]*\)\?\).*/\1/p;}')
if [ -z "$gcov_version" ] || [ "$cc_version" != "$gcov_version" ]; then
    echo "Compiler $cc and coverage tool $gcov have different versions." >&2
    exit 1
fi

if [ -n "${GITHUB_ENV:-}" ]; then
    printf 'CC=%s\nGCOV=%s\n' "$cc" "$gcov" >> "$GITHUB_ENV"
    printf '%s\n' "$venv_dir/bin" >> "$GITHUB_PATH"
fi

"$venv_dir/bin/gcovr" --version | head -1
"$cc" --version | head -1
"$gcov" --version | head -1

printf 'Coverage environment: %s\n' "$venv_dir/bin/activate"
