#!/usr/bin/env python3
"""Exercise failure propagation in isolated runner trees using real CTest."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('coverage_checks', HERE / 'coverage.py')
coverage = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(coverage)


class RunnerFailures(unittest.TestCase):
    """Fault injections leave the production build and dependency trees intact."""

    def setUp(self):
        """Create a disposable runner with the same pinned dependencies."""
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.runner = self.root / 'test/cmake/usbx'
        self.runner.mkdir(parents=True)
        externals = self.root / 'test/externals'
        externals.mkdir()
        for name in ('threadx', 'filex', 'netxduo'):
            (externals / name).symlink_to(HERE.parents[1] / 'externals' / name)
        for name in ('run.sh', 'dependencies.txt', 'report.py'):
            shutil.copy2(HERE / name, self.runner / name)
        (self.runner / 'CMakeLists.txt').write_text('set(BUILD_CONFIGURATIONS\n  generic_build\n  )\n')
        self.env = dict(os.environ, CC='gcc-14', GCOV='gcov-14',
                        CTEST_PARALLEL_LEVEL='1', TX_COVERAGE='OFF')

    def run_command(self, *arguments):
        """Capture diagnostics and return the runner's actual exit status."""
        return subprocess.run(['bash', './run.sh', *arguments], cwd=self.runner,
                              env=self.env, text=True, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, timeout=60)

    def test_manual_input_is_data(self):
        """Shell metacharacters in a manual selector cannot execute commands."""
        event = self.root / 'event.json'
        event.write_text(json.dumps({'inputs': {'tests_to_run': 'generic_build; touch injected'}}))
        self.env['GITHUB_EVENT_PATH'] = str(event)
        result = self.run_command('test', '--manual')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('Unknown profile', result.stdout)
        self.assertFalse((self.runner / 'injected').exists())

    def test_checkout_failure(self):
        """A failed fetch cannot continue into configuration or testing."""
        (self.root / 'test/externals/threadx').unlink()
        bin_dir = self.root / 'bin'
        bin_dir.mkdir()
        git = shutil.which('git')
        wrapper = bin_dir / 'git'
        wrapper.write_text(f'#!/bin/bash\nfor arg in "$@"; do\n'
                           '  if [ "$arg" = fetch ]; then exit 71; fi\ndone\n'
                           f'exec {git} "$@"\n')
        wrapper.chmod(0o755)
        self.env['PATH'] = str(bin_dir) + os.pathsep + self.env['PATH']
        result = self.run_command('build', 'all')
        self.assertEqual(result.returncode, 71, result.stdout)
        self.assertFalse((self.runner / 'build').exists())

        self.env['USBX_TEST_DEPENDENCY'] = str(HERE.parents[1] / 'externals/threadx')
        wrapper.write_text(
            '#!/bin/bash\n'
            'if [ "${3:-}" = checkout ]; then exit 72; fi\n'
            f'if [ "${{3:-}}" = fetch ]; then exec {git} -C "$2" fetch '
            '--depth 1 "$USBX_TEST_DEPENDENCY" "$7"; fi\n'
            f'exec {git} "$@"\n')
        result = self.run_command('build', 'all')
        self.assertEqual(result.returncode, 72, result.stdout)
        self.assertFalse((self.runner / 'build').exists())

    def test_dependency_build_failure(self):
        """An actual CMake dependency error propagates through the bootstrap."""
        libs = self.runner / 'libs'
        libs.mkdir()
        (libs / 'CMakeLists.txt').write_text(
            'cmake_minimum_required(VERSION 3.13)\nproject(failure NONE)\n'
            'message(FATAL_ERROR "injected dependency failure")\n')
        result = self.run_command('build_libs')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('injected dependency failure', result.stdout)

        (libs / 'CMakeLists.txt').write_text(
            'cmake_minimum_required(VERSION 3.13)\nproject(failure NONE)\n'
            'add_custom_target(failing_dependency ALL COMMAND ${CMAKE_COMMAND} -E false)\n')
        result = self.run_command('build_libs')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('failing_dependency', result.stdout)
        self.assertIn('FAILED', result.stdout)

    def test_ctest_failure_keeps_diagnostics_and_collects_coverage(self):
        """A failing CTest case stays red while collection still executes."""
        source = self.root / 'fixture'
        source.mkdir()
        (source / 'CMakeLists.txt').write_text(
            'cmake_minimum_required(VERSION 3.13)\nproject(failure NONE)\n'
            'enable_testing()\nadd_test(NAME injected_failure COMMAND '
            '${CMAKE_COMMAND} -E false)\n')
        build = self.runner / 'build/generic_build'
        subprocess.run(['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja',
                        '-DUSBX_CI_COVERAGE:BOOL=ON'], check=True, capture_output=True)
        subprocess.run(['cmake', '--build', str(build)], check=True, capture_output=True)
        (build.parent / 'built-profiles.txt').write_text('generic_build\n')
        report = self.runner / 'coverage_report'
        report.mkdir()
        (report / 'merged.xml').write_text('stale')
        # Record both bootstrap coverage callbacks independently of gcovr.
        collector = self.runner / 'coverage.sh'
        collector.write_text('#!/bin/bash\nprintf "%s\\n" "$1" >> coverage_report/collected.txt\n')
        collector.chmod(0o755)
        self.env['TX_COVERAGE'] = 'ON'
        result = self.run_command('test', 'all')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('injected_failure', (build / 'generic_build.xml').read_text())
        self.assertTrue(list((build / 'Testing/Temporary').glob('LastTest*.log')))
        self.assertEqual((report / 'collected.txt').read_text(), 'generic_build\n--merge\n')
        self.assertFalse((report / 'merged.xml').exists())

    def test_missing_coverage_fails_runner(self):
        """Passing CTest cannot hide missing coverage objects or tracefiles."""
        source = self.root / 'fixture'
        source.mkdir()
        (source / 'CMakeLists.txt').write_text(
            'cmake_minimum_required(VERSION 3.13)\nproject(passing NONE)\n'
            'enable_testing()\nadd_test(NAME passing COMMAND '
            '${CMAKE_COMMAND} -E true)\n')
        build = self.runner / 'build/generic_build'
        subprocess.run(['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja',
                        '-DUSBX_CI_COVERAGE:BOOL=ON'], check=True, capture_output=True)
        subprocess.run(['cmake', '--build', str(build)], check=True, capture_output=True)
        (build.parent / 'built-profiles.txt').write_text('generic_build\n')
        for name in ('coverage.sh', 'coverage.py', 'coverage-floors.json'):
            shutil.copy2(HERE / name, self.runner / name)
        self.env['TX_COVERAGE'] = 'ON'
        result = self.run_command('test', 'all')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('100% tests passed', result.stdout)
        self.assertIn('No instrumented objects', result.stdout)
        self.assertIn('Missing or empty coverage input', result.stdout)

        collector = self.runner / 'coverage.sh'
        collector.write_text(
            '#!/bin/bash\nset -e\n'
            'if [ "$1" = --merge ]; then exec python3 coverage.py --merge; fi\n'
            'mkdir -p coverage_report/per_configuration/generic_build\n'
            'touch coverage_report/per_configuration/generic_build/index.html\n'
            'touch coverage_report/per_configuration/generic_build.json\n'
            'touch coverage_report/per_configuration/generic_build.xml\n')
        result = self.run_command('test', 'all')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('100% tests passed', result.stdout)
        self.assertIn('Missing or empty coverage input', result.stdout)


class CoverageInputs(unittest.TestCase):
    """Reject missing, empty, invalid and non-relative raw inputs."""

    def setUp(self):
        """Allocate a temporary tracefile independent of matrix outputs."""
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.path = Path(self.temporary.name) / 'trace.json'

    def test_missing_and_empty(self):
        """Both missing and zero-byte files fail before merging."""
        with self.assertRaises(ValueError):
            coverage.trace_lines(self.path)
        self.path.touch()
        with self.assertRaises(ValueError):
            coverage.trace_lines(self.path)
        self.path.write_text('{"files": []}')
        with self.assertRaises(ValueError):
            coverage.trace_lines(self.path)

    def test_source_policy_and_lines(self):
        """Keep host and device sources, including their uncovered lines."""
        files = [{'file': f'common/{area}/src/probe.c', 'lines': [
            {'line_number': 10, 'count': 1}, {'line_number': 20, 'count': 0}]
        } for area in ('core', 'usbx_host_classes', 'usbx_device_classes')]
        self.path.write_text(json.dumps({'files': files}))
        measured, covered = coverage.trace_lines(self.path)
        self.assertEqual(len(measured), 6)
        self.assertEqual(len(covered), 3)
        for name in ('/absolute/probe.c', '../probe.c', 'test/probe.c'):
            files[0]['file'] = name
            self.path.write_text(json.dumps({'files': files}))
            with self.assertRaises(ValueError):
                coverage.trace_lines(self.path)


class CoverageMerge(unittest.TestCase):
    """Exercise gcovr's real merge with overlapping and disjoint source lines."""

    def test_union_and_missing_input_invalidation(self):
        """The union keeps unhit lines and invalidates stale aggregate reports."""
        with tempfile.TemporaryDirectory() as directory:
            previous = coverage.REPORT
            coverage.REPORT = Path(directory)
            self.addCleanup(setattr, coverage, 'REPORT', previous)
            selection = coverage.profiles()[:2]
            (coverage.REPORT / 'profiles.txt').write_text('\n'.join(selection) + '\n')
            source = 'common/core/src/ux_dcd_sim_slave_endpoint_create.c'
            expected = set()
            for profile, numbers in zip(selection, ((87, 90), (87, 93))):
                lines = []
                for number in numbers:
                    expected.add((source, number))
                    lines.append({'line_number': number, 'count': int(number == 87),
                                  'branches': [{'count': 1, 'fallthrough': True,
                                                'throw': False, 'source_block_id': 2,
                                                'destination_block_id': 3}]})
                raw = coverage.REPORT / f'{profile}-input.json'
                raw.write_text(json.dumps({'gcovr/format_version': '0.14', 'files': [
                    {'file': source, 'lines': lines, 'functions': []}]}))
                coverage.generate(coverage.REPORT / 'per_configuration' / profile,
                                  ['--add-tracefile', str(raw)])
            coverage.merge()
            measured, covered = coverage.verify(coverage.REPORT / 'subset')
            self.assertEqual(measured, expected)
            self.assertEqual(covered, {(source, 87)})
            self.assertFalse((coverage.REPORT / 'merged.xml').exists())
            (coverage.REPORT / 'per_configuration' / (selection[1] + '.json')).write_text('')
            with self.assertRaises(ValueError):
                coverage.merge()
            self.assertFalse((coverage.REPORT / 'subset.xml').exists())


if __name__ == '__main__':
    os.environ.pop('GITHUB_STEP_SUMMARY', None)
    unittest.main()
