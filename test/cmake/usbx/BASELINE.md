# GCC 14 baseline

Measured at USBX commit `ffc0f79d951c716a03addcf7255020328e0d5b47`, with the
dependency revisions in `dependencies.txt`, GCC/gcov 14.3.0 and gcovr 8.6.
All 17 configurations were configured and built from clean trees with CMake and
Ninja. CTest ran every configuration once, including configurations with build
failures: 3,019/3,043 cases passed in 666.17 seconds of summed CTest wall time.

Twenty-three cases could not run because five regression sources had incompatible
pointer arguments or missing declarations. The sample also lacked device-stack
declarations. One standalone printer transfer assertion failed. These results
are retained as failures; a subsequent isolated printer pass does not erase them.

Coverage below uses the existing profile-specific exclusions, so percentages are
not comparable across profiles and cannot be averaged or merged. A dash means
the profile was not instrumented. Standalone-device collection initially failed
gcovr's 2^32 suspicious-hit heuristic; the same raw counters were then collected
with the documented 2^40 threshold and unchanged exclusions. No tests were rerun
to produce that coverage. Build times are Ninja execution spans and exclude
configuration and dependency setup.

| Profile | Passed / tests | Build s | Test s | Lines hit / valid | Branches hit / valid |
| --- | ---: | ---: | ---: | ---: | ---: |
| default_build_coverage | 427/430 | 10.838 | 13.89 | 7701/8014 | 3420/3738 |
| error_check_build_full_coverage | 425/430 | 11.190 | 13.53 | 14767/24084 | 6238/11250 |
| tracex_enable_build | 427/430 | 11.390 | 13.48 | — | — |
| device_buffer_owner_build | 427/430 | 11.371 | 11.18 | — | — |
| device_zero_copy_build | 427/430 | 11.271 | 16.53 | — | — |
| nofx_build_coverage | 46/46 | 6.778 | 5.57 | 2751/7955 | 993/3738 |
| optimized_build | 129/129 | 6.893 | 3.46 | — | — |
| standalone_device_build_coverage | 55/57 | 6.669 | 185.88 | 2625/3098 | 880/1279 |
| standalone_device_buffer_owner_build | 56/57 | 6.782 | 185.39 | — | — |
| standalone_device_zero_copy_build | 56/57 | 6.896 | 177.48 | — | — |
| standalone_host_build_coverage | 44/44 | 6.633 | 20.78 | 4005/5255 | 1464/2289 |
| standalone_build_coverage | 20/20 | 5.412 | 0.25 | 2011/8277 | 720/3567 |
| generic_build | 1/1 | 6.047 | 0.01 | — | — |
| otg_support_build | 427/430 | 10.614 | 11.31 | — | — |
| memory_management_build_coverage | 11/11 | 6.619 | 4.63 | 4748/8069 | 1715/3780 |
| msrc_rtos_build | 33/33 | 6.482 | 0.76 | — | — |
| msrc_standalone_build | 8/8 | 5.069 | 2.04 | — | — |
