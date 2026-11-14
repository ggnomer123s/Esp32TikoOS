"""Publish build outputs locally; never flash hardware or upload to a service."""
import argparse
import hashlib
import os
import json
from pathlib import Path
import shutil
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--build-log', type=Path, required=True)
p.add_argument('--test-log', type=Path, required=True)
args = p.parse_args()
assert 'TikoOS native build: SUCCESS' in args.build_log.read_text()
assert '557 assertions passed' in args.test_log.read_text()
assert 'Native target_input:' in args.test_log.read_text()
dist = root/'dist'
build = root/'firmware/build-tiko'
config=json.loads((build/'config/sdkconfig.json').read_text())
assert config.get('TIKO_LAUNCHER_APP') is True
shutil.copy2(build/'TikoOS.bin', dist/'firmware.bin')
shutil.copy2(args.build_log, dist/'build.log')
shutil.copy2(args.test_log, dist/'native-tests.log')
tools=Path(os.environ.get('IDF_TOOLS_PATH','/workspace/idf-tools'))
python_env=os.environ.get('IDF_PYTHON_ENV_PATH')
if not python_env:
    python_env=str(sorted((tools/'python_env').glob('idf5.4_py*_env'))[-1])
python=str(Path(python_env)/'bin/python')
nm=sorted((tools/'tools/xtensa-esp-elf').glob('*/xtensa-esp-elf/bin/xtensa-esp32s3-elf-nm'))[-1]
symbols=subprocess.check_output([str(nm),'-u',str(build/'esp-idf/fw_ota/libfw_ota.a')],text=True)
assert not any(name in symbols for name in ('esp_ota_begin','esp_ota_write','esp_ota_end','esp_ota_set_boot_partition'))
(dist/'launcher-check.txt').write_text('CONFIG_TIKO_LAUNCHER_APP=y\nApp image only; no merged factory image.\nIn-app OTA object imports no app-slot writing API.\nBootloader and partition table are not distributed or flashed.\n')
with (dist/'image-info.txt').open('w') as f:
    subprocess.run([python,'-m','esptool','--chip','esp32s3','image-info',str(dist/'firmware.bin')],stdout=f,check=True)
info=(dist/'image-info.txt').read_text()
assert info.count('(valid)')==2 and 'App version: 0.3.0' in info
with (dist/'size.json').open('w') as f:
    subprocess.run([python,'-m','esp_idf_size','--format','json2',str(build/'TikoOS.map')],stdout=f,check=True)
sha=hashlib.sha256((dist/'firmware.bin').read_bytes()).hexdigest()
(dist/'SHA256SUMS').write_text(sha+'  firmware.bin\n')
with zipfile.ZipFile(dist/'TikoOS-microSD.zip','w',zipfile.ZIP_DEFLATED) as z:
    for f in (root/'sdcard').rglob('*'):
        if f.is_file():z.write(f,f.relative_to(root/'sdcard'))
    z.write(dist/'firmware.bin','firmware.bin')
    z.write(dist/'SHA256SUMS','SHA256SUMS')
# Root ignore rules exclude the SDK/build caches, not the vendored firmware sources.
files=subprocess.check_output(['git','ls-files','-z','--cached','--others','--exclude-standard'],cwd=root).split(b'\0')
with zipfile.ZipFile(dist/'TikoOS-v0.3-source.zip','w',zipfile.ZIP_DEFLATED) as z:
    for name in files:
        if name:
            path=Path(name.decode())
            if path.parts[0]!='dist' and (root/path).is_file():z.write(root/path,path)
for name in ('TikoOS-microSD.zip','TikoOS-v0.3-source.zip'):
    with zipfile.ZipFile(dist/name) as z:
        assert z.testzip() is None
        if name=='TikoOS-microSD.zip':
            assert not any(Path(n).name in {'bootloader.bin','partitions.bin','partition-table.bin','boot_app0.bin'} for n in z.namelist())
print(f'TikoOS v0.3 app: {(dist/"firmware.bin").stat().st_size} bytes; SHA256 {sha}')
