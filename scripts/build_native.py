"""Build TikoOS on ESP-IDF 5.4.1. No device access or flashing."""
import os
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
idf = Path(os.environ.get('IDF_PATH', '/workspace/esp-idf-5.4.1'))
tools = Path(os.environ.get('IDF_TOOLS_PATH', '/workspace/idf-tools'))
env = os.environ.copy()
env.update(IDF_PATH=str(idf), IDF_TOOLS_PATH=str(tools),
           IDF_COMPONENT_MANAGER='0', IDF_PYTHON_CHECK_CONSTRAINTS='no')
python_env = os.environ.get('IDF_PYTHON_ENV_PATH')
if not python_env:
    candidates = sorted((tools / 'python_env').glob('idf5.4_py*_env'))
    if not candidates:
        sys.exit('Run bash scripts/install_native.sh first, or set IDF_PYTHON_ENV_PATH.')
    python_env = str(candidates[-1])
env['IDF_PYTHON_ENV_PATH'] = python_env
python = str(Path(python_env) / 'bin/python')
exports = subprocess.check_output([python, str(idf / 'tools/idf_tools.py'),
                                    'export', '--format=key-value'], env=env, text=True)
allowed = {'PATH', 'OPENOCD_SCRIPTS', 'ESP_ROM_ELF_DIR', 'ESP_IDF_VERSION',
           'IDF_DEACTIVATE_FILE_PATH', 'IDF_PYTHON_ENV_PATH'}
for line in exports.splitlines():
    key, sep, value = line.partition('=')
    if sep and key in allowed:
        env[key] = value.replace('$PATH', env['PATH'])
env['PATH'] = str(Path(python_env) / 'bin') + os.pathsep + env['PATH']
work = root / 'firmware'
subprocess.run([python, str(idf / 'tools/idf.py'), '-B', 'build-tiko',
                '-DFLIPPER_BOARD=lilygo_t_embed_cc1101', '-DIDF_TARGET=esp32s3',
                '-DSDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.tiko.defaults',
                'reconfigure'], cwd=work, env=env, check=True)
subprocess.run(['cmake', '--build', 'build-tiko', '--target', 'app', '--parallel',
                os.environ.get('TIKO_BUILD_JOBS', '4')], cwd=work, env=env, check=True)
assert (work / 'build-tiko/TikoOS.bin').is_file()
print('TikoOS native build: SUCCESS', flush=True)
