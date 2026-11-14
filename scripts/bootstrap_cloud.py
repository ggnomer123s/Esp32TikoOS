"""Reproducible Linux x86_64 cloud setup, no USB access or project-file edits.
Run with /workspace/pio-venv/bin/python after installing requirements-cloud.txt.
Official release checksums are from Espressif's package_esp32_index.json 2.0.17.
"""
import configparser
import hashlib
import json
import os
from pathlib import Path
import subprocess
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
WORK = Path('/workspace')
CACHE = WORK / 'tool-downloads'
LOCAL = WORK / 'local-tools'
CACHE.mkdir(exist_ok=True)
LOCAL.mkdir(exist_ok=True)

def run(*args):
    subprocess.run(args, check=True)

def clone(url, path, commit):
    if not (path / '.git').exists():
        run('git', 'clone', url, str(path))
        run('git', '-C', str(path), 'checkout', '--detach', commit)
    actual = subprocess.check_output(['git', '-C', str(path), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != commit:
        raise RuntimeError(f'{path}: expected {commit}; preserving existing checkout')

clone('https://github.com/platformio/platform-espressif32.git', WORK/'platform-espressif32', '3988de311b6416edc74e610645849b2810d4b574')
clone('https://github.com/Bodmer/TFT_eSPI.git', WORK/'tft-library', '317ce97e598c004d158b87d01940a31acfb3eced')
items = [
 ('framework-arduinoespressif32','3.20017.0','esp32-2.0.17','https://github.com/espressif/arduino-esp32/releases/download/2.0.17/esp32-2.0.17.zip','1f8658d4b18a8001ce782142ad08164af2991d70b83a147c3437a6ee30a9b225'),
 ('toolchain-xtensa-esp32s3','8.4.0+2021r2-patch5','toolchain-xtensa-esp32s3','https://github.com/espressif/crosstool-NG/releases/download/esp-2021r2-patch5/xtensa-esp32s3-elf-gcc8_4_0-esp-2021r2-patch5-linux-amd64.tar.gz','8aa17a6adf01efa5b1628c8ac578063a44d26ae9581d39486b92223a41ef262f'),
 ('toolchain-riscv32-esp','8.4.0+2021r2-patch5','toolchain-riscv32-esp','https://github.com/espressif/crosstool-NG/releases/download/esp-2021r2-patch5/riscv32-esp-elf-gcc8_4_0-esp-2021r2-patch5-linux-amd64.tar.gz','f7d73e5f9e2df3ea6ca8e2c95d6ca6d23d6b38fd101ea5d3012f3cb3cd59f39f')]
for name, version, folder, url, sha in items:
    archive = CACHE / url.split('/')[-1]
    if not archive.exists():
        urllib.request.urlretrieve(url, archive)
    digest = hashlib.file_digest(archive.open('rb'), 'sha256').hexdigest()
    if digest != sha:
        raise RuntimeError(f'Checksum mismatch: {archive}')
    dest = LOCAL / folder
    if not (dest/'package.json').exists():
        if archive.suffix == '.zip':
            with zipfile.ZipFile(archive) as z:
                z.extractall(LOCAL)
        else:
            dest.mkdir(exist_ok=True)
            run('tar','-xzf',str(archive),'--strip-components=1','-C',str(dest))
        (dest/'package.json').write_text(json.dumps({'name':name,'version':version,'description':'Official checksum-verified Espressif artifact'}))
    print(f'{name}: archive SHA-256 verified', flush=True)

wrappers = {
 'tool-esptoolpy': ('2.41100.0', 'esptool.py', "import sys\nfrom pathlib import Path\nsys.path=[p for p in sys.path if Path(p or '.').resolve()!=Path(__file__).parent.resolve()]\nimport esptool\nesptool._main()\n"),
 'tool-scons': ('4.40801.0', 'scons.py', 'from SCons.Script import main\nmain()\n')}
for name,(version, filename, code) in wrappers.items():
    dest=LOCAL/name
    dest.mkdir(exist_ok=True)
    (dest/'package.json').write_text(json.dumps({'name':name,'version':version,'description':'Local wrapper for pinned PyPI tool'}))
    (dest/filename).write_text(code)
config=configparser.ConfigParser(interpolation=None)
config.read(ROOT/'platformio.ini')
env=config['env:tiko']
env['platform']='file:///workspace/platform-espressif32'
env['lib_deps']='file:///workspace/tft-library'
env['platform_packages']='\n'+'\n'.join(name+' @ file://'+str(LOCAL/folder) for name,_,folder,_,_ in items)+'\ntool-esptoolpy @ file:///workspace/local-tools/tool-esptoolpy\ntool-scons @ file:///workspace/local-tools/tool-scons'
with (WORK/'tiko-cloud.ini').open('w') as f:config.write(f)
print('Run: python scripts/cloud_pio.py run -c /workspace/tiko-cloud.ini -j 4')
