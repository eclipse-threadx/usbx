#!/bin/bash
CTEST_PARALLEL_LEVEL=4 $(dirname `realpath $0`)/../test/cmake/usbx/run.sh test default_build_coverage
