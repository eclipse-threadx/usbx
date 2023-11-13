#!/bin/bash
CTEST_PARALLEL_LEVEL=4 $(dirname `realpath $0`)/../Test/cmake/usbx/run.sh test all
