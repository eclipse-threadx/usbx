#!/usr/bin/env python3
"""Collect one consistent source universe and verify every merge input."""
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
REPORT = HERE / 'coverage_report'
POLICY = r'common/[^/]+/src/[^/]+\.c$'


def profiles():
    """Read the same profile list used by CMake and the bootstrap."""
    text = (HERE / 'CMakeLists.txt').read_text()
    block = text.split('set(BUILD_CONFIGURATIONS', 1)[1].split(')', 1)[0]
    return re.findall(r'[a-z_]*build[a-z_]*', block)


def nonempty(path):
    """Reject absent and zero-byte artifacts before publishing."""
    if not path.is_file() or not path.stat().st_size:
        raise ValueError(f'Missing or empty coverage input: {path.name}')


def source_name(name):
    """Require repository-relative USBX source paths without traversal."""
    if PurePosixPath(name).is_absolute() or '..' in PurePosixPath(name).parts or not re.fullmatch(POLICY, name):
        raise ValueError(f'Invalid coverage source: {name}')


def trace_lines(path):
    """Return measured and covered source-line sets from an unfiltered trace."""
    nonempty(path)
    data = json.loads(path.read_text())
    measured, covered = set(), set()
    for entry in data['files']:
        name = entry['file']
        source_name(name)
        for line in entry['lines']:
            if line.get('gcovr/excluded', False) or line.get('gcovr/noncode', False):
                continue
            key = (name, line['line_number'])
            measured.add(key)
            if line['count'] > 0:
                covered.add(key)
    if not measured:
        raise ValueError(f'No measured source lines: {path.name}')
    return measured, covered


def xml_lines(path):
    """Validate Cobertura denominators and repository-relative filenames."""
    nonempty(path)
    root = ET.parse(path).getroot()
    measured, covered = set(), set()
    for entry in root.findall('.//class'):
        name = entry.attrib['filename']
        source_name(name)
        for line in entry.findall('./lines/line'):
            key = (name, int(line.attrib['number']))
            measured.add(key)
            if int(line.attrib['hits']) > 0:
                covered.add(key)
    if not measured or len(measured) != int(root.attrib['lines-valid']):
        raise ValueError(f'Invalid source-line denominator: {path.name}')
    if len(covered) != int(root.attrib['lines-covered']):
        raise ValueError(f'Invalid covered-line count: {path.name}')
    if any(source.text not in ('.', '') for source in root.findall('./sources/source')):
        raise ValueError('Coverage source root must be repository-relative')
    return measured, covered


def relative_xml(path):
    """Normalize gcovr's source root without changing file or line records."""
    tree = ET.parse(path)
    for source in tree.findall('./sources/source'):
        source.text = '.'
    tree.write(path, encoding='utf-8', xml_declaration=True)


def generate(base, inputs):
    """Generate all formats from the same gcovr invocation and policy."""
    base.mkdir(parents=True, exist_ok=True)
    args = ['gcovr', '--root', str(ROOT), '--filter', POLICY,
            '--gcov-executable', os.environ.get('GCOV', 'gcov-14'),
            '--merge-mode-functions', 'separate',
            # Standalone polling legitimately exceeds gcovr's 32-bit heuristic.
            '--gcov-suspicious-hits-threshold', str(2**40),
            '--json', str(base.with_suffix('.json')),
            '--xml', str(base.with_suffix('.xml')), '--xml-pretty',
            '--html-details', str(base / 'index.html'), *inputs]
    subprocess.run(args, cwd=ROOT, check=True)
    relative_xml(base.with_suffix('.xml'))
    verify(base)


def verify(base):
    """Check all expected formats and their exact line-set agreement."""
    nonempty(base / 'index.html')
    actual = trace_lines(base.with_suffix('.json'))
    if actual != xml_lines(base.with_suffix('.xml')):
        raise ValueError(f'JSON/XML line mismatch: {base.name}')
    return actual


def merge():
    """Merge only the current selection, retaining every measured input line."""
    # Remove old aggregate reports before checking inputs, including failure paths.
    for name in ('merged', 'subset'):
        shutil.rmtree(REPORT / name, ignore_errors=True)
        for suffix in ('.json', '.xml'):
            (REPORT / (name + suffix)).unlink(missing_ok=True)
    selection = (REPORT / 'profiles.txt').read_text().splitlines()
    available = profiles()
    if not selection or len(set(selection)) != len(selection) or not set(selection) <= set(available):
        raise ValueError('Invalid coverage profile selection')
    expected, hit = set(), set()
    inputs = []
    for profile in selection:
        base = REPORT / 'per_configuration' / profile
        lines, covered = verify(base)
        expected |= lines
        hit |= covered
        inputs += ['--add-tracefile', str(base.with_suffix('.json'))]
    complete = set(selection) == set(available) and (REPORT / 'full-suite').is_file()
    base = REPORT / ('merged' if complete else 'subset')
    generate(base, inputs)
    if verify(base) != (expected, hit):
        raise ValueError('Merged report differs from the exact source-line union')
    root = ET.parse(base.with_suffix('.xml')).getroot()
    summary = f"{'Complete' if complete else 'Subset'} coverage: {len(selection)}/{len(available)} profiles; "
    for label in ('lines', 'branches'):
        covered, valid = int(root.attrib[f'{label}-covered']), int(root.attrib[f'{label}-valid'])
        if valid == 0:
            raise ValueError(f'Empty {label} denominator')
        rate = covered * 100 / valid
        summary += f'{label} {covered}/{valid} ({rate:.4f}%); '
    print(summary)
    (REPORT / 'summary.txt').write_text(summary + '\n')
    if os.environ.get('GITHUB_STEP_SUMMARY'):
        with open(os.environ['GITHUB_STEP_SUMMARY'], 'a') as stream:
            stream.write(summary + '\n')
    if complete:
        floors = json.loads((HERE / 'coverage-floors.json').read_text())
        for label, minimum in floors.items():
            if int(root.attrib[f'{label}-covered']) * 100 < int(root.attrib[f'{label}-valid']) * minimum:
                raise ValueError(f'{label} coverage is below {minimum}%')


def main():
    """Dispatch collection or union validation with a failing exit on any error."""
    if sys.argv[1:] == ['--merge']:
        merge()
    elif len(sys.argv) == 2 and sys.argv[1] in profiles():
        profile = sys.argv[1]
        if profile not in (REPORT / 'profiles.txt').read_text().splitlines():
            raise ValueError('Profile does not belong to this test run')
        objects = HERE / 'build' / profile / 'usbx' / 'CMakeFiles' / 'usbx.dir' / 'common'
        if not any(objects.rglob('*.gcno')):
            raise ValueError(f'No instrumented objects: {profile}')
        generate(REPORT / 'per_configuration' / profile, [str(objects)])
    else:
        raise ValueError('Expected a known profile or --merge')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, KeyError, ET.ParseError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
