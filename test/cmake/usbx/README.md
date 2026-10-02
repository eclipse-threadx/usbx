# Linux regression and coverage

The regression workflow builds and tests all 17 profiles on every pull request to
`dev` or `master`. Its check name is `auto_tests / run_tests`. Workflow
success alone does not enforce merging: repository rules must separately require
that check. The staged Otterdog ruleset must remain disabled until this workflow
is available on `dev`.

Run `scripts/install.sh` on Ubuntu 24.04, then use GCC 14 and its matching gcov with
CMake and Ninja:

```sh
TX_COVERAGE=ON ./scripts/build.sh all
TX_COVERAGE=ON ./scripts/test.sh all
python3 test/cmake/usbx/test_runner.py
```

The installer installs gcovr 8.6 in a virtual environment and exports its location
through GitHub Actions environment files. Outside Actions, activate the reported
virtual environment before running coverage. `CC` and `GCOV` can select matching
compiler and coverage executables. The default build and test concurrency is four.
CTest runs each case once; failing cases are not retried into a passing result.

`dependencies.txt` records immutable ThreadX, FileX and NetX Duo revisions. FileX
PR #106 is open at the pinned revision; this setup does not assume it has merged.
Existing dependency checkouts must be clean and match their pins. Remove an old
checkout explicitly when updating a pin. The ThreadX bootstrap supplies JUnit
output without runtime edits to dependency files.

Pass profile names instead of `all` to either script, or select them in the manual
workflow. Build and test output names the exact selection. JUnit files, test logs
and `build/results.txt` report actual counts and elapsed test times. The generic
profile's single CTest case is a build-only placeholder, not a functional USB test.

Every selected profile is instrumented when `TX_COVERAGE=ON`, including the generic
build. Raw JSON, Cobertura XML and HTML use one inclusion policy: all compiled C
sources matching `common/*/src/*.c`. Host controllers, simulators, optional classes
and untested source lines remain in the denominator. Dependency and test code are
outside that policy. No profile-specific diagnostic exclusion affects the union.

Each input must have nonempty JSON, XML and HTML, measured source lines, and
repository-relative paths. The merger checks that JSON and XML agree and that its
measured and covered source-line sets equal the exact unions of its inputs.
Reports and execution counters are cleared before each test selection. A subset
cannot consume an earlier full run's reports or publish complete coverage.
Available diagnostics and coverage survive failing tests; missing coverage itself
fails the runner. Full-suite line and branch floors are in `coverage-floors.json`.
Standalone polling produces more than six billion positive hits in a single
profile. Direct GCC 14 output confirms balanced loop counts. Collection uses a
2^40 suspicious-hit threshold instead of gcovr's default 2^32 heuristic; negative
counts and parse errors remain fatal, and no lines are ignored to obtain a report.

Only successful complete runs on `master` can deploy Pages. Callers grant the
permissions required for GitHub to validate the pinned reusable workflow's
separate deployment job; its test job retains narrower permissions. The template
bounds installation, build and test steps and uploads diagnostics on failure.
The manual selector is parsed from the event file, never interpolated as shell
code. All pull requests retain the full matrix; there is no conditional routing.

Dependabot targets `dev` once its configuration is present on the default branch.
The cross-repository workflow/bootstrap revision requires a separate review and
is excluded from routine action updates.
