"""Package the current successful build. Run from any directory after pio run.
Usage: python scripts/package.py --build-log /workspace/build-v02-final.log \
    --test-log /workspace/native-v02.log
"""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--build-log', type=Path, required=True)
p.add_argument('--test-log', type=Path, required=True)
args = p.parse_args()
assert '[SUCCESS]' in args.build_log.read_text(), 'Build has not passed'
assert 'assertions passed' in args.test_log.read_text(), 'Tests have not passed'
dist = root / 'dist'
dist.mkdir(exist_ok=True)
shutil.copy(root / '.pio/build/tiko/firmware.bin', dist / 'firmware.bin')
shutil.copy(args.build_log, dist / 'build.log')
shutil.copy(args.test_log, dist / 'native-tests.log')
with (dist / 'image-info.txt').open('w') as out:
    subprocess.run(['esptool.py', '--chip', 'esp32s3', 'image_info', '--version', '2',
                    str(dist / 'firmware.bin')], stdout=out, check=True)
assert '(valid)' in (dist / 'image-info.txt').read_text()
digest = hashlib.sha256((dist / 'firmware.bin').read_bytes()).hexdigest()
(dist / 'SHA256SUMS').write_text(digest + '  firmware.bin\n')
with zipfile.ZipFile(dist / 'microsd.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    for f in (root / 'sdcard').rglob('*'):
        if f.is_file():
            z.write(f, f.relative_to(root / 'sdcard'))
    z.write(dist / 'firmware.bin', 'firmware.bin')
    z.write(dist / 'SHA256SUMS', 'SHA256SUMS')
# Use Git's ignore rules; include current untracked source files as well as tracked ones.
files = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others',
                                 '--exclude-standard'], cwd=root).split(b'\0')
with zipfile.ZipFile(dist / 'TikoOS-v0.2-source.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    for name in files:
        if not name:
            continue
        relative = Path(name.decode())
        file = root / relative
        if file.is_file() and relative.parts[0] != 'dist':
            z.write(file, relative)
for name in ('microsd.zip', 'TikoOS-v0.2-source.zip'):
    with zipfile.ZipFile(dist / name) as z:
        assert z.testzip() is None
print(f'firmware.bin: {(dist / "firmware.bin").stat().st_size} bytes, SHA-256 {digest}')
