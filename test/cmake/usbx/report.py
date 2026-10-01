#!/usr/bin/env python3
"""Summarize the actual JUnit results, including failed or missing runs."""
import os
import re
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

rows = ['| Profile | Passed / tests | Seconds |', '| --- | ---: | ---: |']
failed = False
for profile in sys.argv[1:]:
    path = Path('build') / profile / f'{profile}.xml'
    try:
        root = ET.parse(path).getroot()
        cases = root.findall('.//testcase')
        passed = sum(case.get('status') == 'run' and
                     case.find('failure') is None and case.find('skipped') is None
                     for case in cases)
        log = path.with_suffix('.txt').read_text()
        duration = re.search(r'Total Test time \(real\) =\s*([0-9.]+) sec', log)
        if duration is None:
            raise ValueError(f'Missing duration: {profile}')
        seconds = float(duration.group(1))
        rows.append(f'| {profile} | {passed} / {len(cases)} | {seconds:.2f} |')
        failed |= not cases or passed != len(cases)
    except (OSError, ET.ParseError, ValueError):
        rows.append(f'| {profile} | missing results | — |')
        failed = True
text = '\n'.join(rows) + '\n'
print(text)
Path('build/results.md').write_text(text, encoding='utf-8')
# The reusable workflow uploads build/*.txt on failure as well as success.
Path('build/results.txt').write_text(text, encoding='utf-8')
if os.environ.get('GITHUB_STEP_SUMMARY'):
    with open(os.environ['GITHUB_STEP_SUMMARY'], 'a', encoding='utf-8') as stream:
        stream.write(text)
sys.exit(int(failed))
