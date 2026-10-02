# Full-matrix validation

GCC/gcov 14.3.0, gcovr 8.6, CMake and Ninja: 3,043/3,043 tests passed across
all 17 configurations, with no skips or retries. Summed CTest wall time was
674.14 seconds. The generic profile contributes one build-only placeholder.
The compiler errors and original coverage are recorded in [BASELINE.md](BASELINE.md).

| Profile | Passed / tests | Build s | Test s | Lines hit / valid | Branches hit / valid |
| --- | ---: | ---: | ---: | ---: | ---: |
| default_build_coverage | 430/430 | 11.879 | 13.99 | 14228/24018 | 5741/11162 |
| error_check_build_full_coverage | 430/430 | 12.779 | 15.38 | 14914/24084 | 6385/11250 |
| tracex_enable_build | 430/430 | 12.593 | 14.91 | 14698/24806 | 5768/11272 |
| device_buffer_owner_build | 430/430 | 12.885 | 14.69 | 15337/25269 | 5994/11474 |
| device_zero_copy_build | 430/430 | 12.952 | 14.87 | 15291/25242 | 5974/11474 |
| nofx_build_coverage | 46/46 | 7.399 | 3.96 | 3302/23982 | 1179/11208 |
| optimized_build | 129/129 | 8.226 | 3.71 | 3604/22895 | 1329/10645 |
| standalone_device_build_coverage | 57/57 | 7.577 | 190.04 | 8520/24155 | 3131/11106 |
| standalone_device_buffer_owner_build | 57/57 | 8.021 | 179.81 | 9044/25436 | 3256/11381 |
| standalone_device_zero_copy_build | 57/57 | 7.503 | 180.82 | 8904/25214 | 3225/11319 |
| standalone_host_build_coverage | 44/44 | 6.951 | 20.78 | 7659/24626 | 2725/11307 |
| standalone_build_coverage | 20/20 | 5.490 | 0.25 | 2965/24467 | 1037/11107 |
| generic_build | 1/1 | 6.589 | 0.00 | 0/24070 | 0/11310 |
| otg_support_build | 430/430 | 11.547 | 13.46 | 14289/24140 | 5789/11292 |
| memory_management_build_coverage | 11/11 | 7.130 | 4.53 | 5527/24073 | 1966/11204 |
| msrc_rtos_build | 33/33 | 7.019 | 0.86 | 7271/24595 | 2602/11492 |
| msrc_standalone_build | 8/8 | 5.786 | 2.08 | 3567/24991 | 1189/11363 |

Build times are Ninja execution spans, excluding configuration and dependency
setup. Coverage includes all compiled common C sources under the same policy
in every profile. The merge contains exactly the raw source-line union:
**20,991/31,649 lines (66.3244%) and 12,118/22,004 branches (55.0718%)**.
Every expected JSON, XML and HTML input was nonempty. All source paths were
repository-relative. An independent raw-JSON set comparison also matched both
the measured and covered line unions. Standalone transfer/task sources absent
from the default build contribute to the merged denominator.

Floors are 66% lines and 54.5% branches, with small margins below the measured
results for simulator scheduling variation. They guard against regression; they
do not meet the project target of 100%. The broader denominator must not be
compared to the baseline's filtered default report as if they measured the same
sources.

Remaining uncovered source lines are included, not excluded:

| Source area | Covered / measured lines | Uncovered lines |
| --- | ---: | ---: |
| Core | 4707/5425 | 718 |
| Device classes | 6568/8962 | 2394 |
| Host classes | 7995/12407 | 4412 |
| Hardware host controllers | 0/2317 | 2317 |
| Network integration | 164/195 | 31 |
| PictBridge | 1557/2343 | 786 |

Closing these gaps requires controller simulation or hardware tests, additional
class/protocol scenarios, and error-path tests. Hardware tests were not run.
The descriptor-size regression covers zero, three-byte and eight-byte invalid
fields, empty layouts and alignment boundaries; the two previously unhit lines
are now covered and that source has 100% measured line coverage in the default
profile.

Runner tests exercise failed fetch/checkout, a real CMake dependency error, a
real failing CTest case, missing/empty coverage inputs, unsafe manual selectors,
and a real gcovr merge with overlapping and disjoint lines. Eight tests pass.
The complete report also passes the configured line and branch floors.
An additional failing CTest case injected into the built generic profile returned
failure while retaining real gcovr JSON/XML/HTML and JUnit diagnostics. A following
manual generic-only run passed 1/1 and produced exactly one profile's reports,
labelled subset, with no stale merged report.

The baseline standalone printer assertion failed once and passed in the complete
revised run. No printer behavior change or retry is claimed as its fix.

Actionlint, ShellCheck and whitespace checks pass. The Ubuntu installer completes
with matching GCC/gcov and the pinned gcovr environment. The staged dev ruleset
remains disabled until workflow rollout; passing CI is not merge enforcement.
