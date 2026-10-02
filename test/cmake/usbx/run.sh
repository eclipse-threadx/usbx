#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

# Resolve only the reviewed revisions; never alter an existing checkout.
mkdir -p ../../externals
while read -r dependency revision; do
    [[ "$revision" =~ ^[0-9a-f]{40}$ ]] || exit 1
    destination="../../externals/$dependency"
    if [ ! -e "$destination" ]; then
        temporary=$(mktemp -d "../../externals/.${dependency}.XXXXXX")
        trap 'rm -rf -- "$temporary"' EXIT
        git init -q "$temporary"
        git -C "$temporary" remote add origin "https://github.com/eclipse-threadx/$dependency.git"
        timeout 180 git -C "$temporary" fetch --depth 1 origin "$revision"
        git -C "$temporary" checkout -q --detach "$revision"
        mv -- "$temporary" "$destination"
        trap - EXIT
    fi
    if [ "$(git -C "$destination" rev-parse HEAD)" != "$revision" ] ||
       [ -n "$(git -C "$destination" status --porcelain --untracked-files=no)" ]; then
        echo "Dependency $dependency must be clean at $revision." >&2
        exit 1
    fi
    printf 'Dependency: %s %s\n' "$dependency" "$revision"
done < dependencies.txt

bootstrap=../../externals/threadx/scripts/cmake_bootstrap.sh
[ -s "$bootstrap" ]
ln -sfn "$bootstrap" .run.sh
if [ "${1:-}" = build_libs ]; then
    exec ./.run.sh build_libs
fi

command=${1:-}
shift || true
manual_full=true
if [ "${1:-}" = --manual ]; then
    manual_full=$(python3 -c 'import json, os; print(str(json.load(open(os.environ["GITHUB_EVENT_PATH"]))["inputs"].get("tests_to_run", "all") == "all").lower())')
    mapfile -t manual < <(python3 -c 'import json, os; print("\n".join(json.load(open(os.environ["GITHUB_EVENT_PATH"]))["inputs"].get("tests_to_run", "all").split()))')
    [ "${#manual[@]}" -gt 0 ] || exit 1
    set -- "${manual[@]}"
fi
mapfile -t available < <(sed -n '/^set(BUILD_CONFIGURATIONS/,/^  )/p' CMakeLists.txt | grep -oE '[a-z_]*build[a-z_]*')
if [ "$#" -eq 0 ]; then
    selected=("${available[0]}")
elif [ "$*" = all ]; then
    selected=("${available[@]}")
else
    selected=("$@")
fi
for profile in "${selected[@]}"; do
    if ! printf '%s\n' "${available[@]}" | grep -qxF "$profile"; then
        echo "Unknown profile: $profile" >&2
        exit 1
    fi
done
if [ "$(printf '%s\n' "${selected[@]}" | sort -u | wc -l)" -ne "${#selected[@]}" ]; then
    echo 'Duplicate profiles are not allowed.' >&2
    exit 1
fi
full_suite=false
if [ "$*" = all ] && "$manual_full"; then full_suite=true; fi
printf 'Selected profiles (%s): %s\n' "${#selected[@]}" "${selected[*]}"
export CTEST_REPEAT_FAIL=1
export CTEST_PARALLEL_LEVEL=${CTEST_PARALLEL_LEVEL:-4}

case "$command" in
    build)
        # Fresh build trees prevent compiler, instrumentation and gcda reuse.
        rm -rf coverage_report
        rm -f build/built-profiles.txt
        for profile in "${selected[@]}"; do
            rm -rf -- "build/$profile"
        done
        ./.run.sh build "${selected[@]}"
        mkdir -p build
        printf '%s\n' "${selected[@]}" > build/built-profiles.txt
        printf 'Built profiles (%s): %s\n' "${#selected[@]}" "${selected[*]}"
        ;;
    test)
        # Clear reports even for subsets, before validating their build inputs.
        rm -rf coverage_report
        mkdir -p coverage_report
        printf '%s\n' "${selected[@]}" > coverage_report/profiles.txt
        if "$full_suite"; then touch coverage_report/full-suite; fi
        for profile in "${selected[@]}"; do
            grep -qxF "$profile" build/built-profiles.txt
            [ -s "build/$profile/CTestTestfile.cmake" ]
            if [ "${TX_COVERAGE:-OFF}" = ON ]; then
                grep -qxF 'USBX_CI_COVERAGE:BOOL=ON' "build/$profile/CMakeCache.txt"
            fi
            find "build/$profile" -name '*.gcda' -delete
            rm -f -- "build/$profile/$profile.xml"
        done
        status=0
        ./.run.sh test "${selected[@]}" || status=$?
        python3 report.py "${selected[@]}" || status=1
        printf 'Tested profiles (%s): %s\n' "${#selected[@]}" "${selected[*]}"
        exit "$status"
        ;;
    *)
        echo 'Usage: run.sh {build|test|build_libs} [all|profile ...]' >&2
        exit 1
        ;;
esac
