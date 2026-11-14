"""Cloud fallback: registry unavailable; use verified local packages and PyPI SCons."""
import os
from pathlib import Path
os.environ.setdefault('PLATFORMIO_CORE_DIR', '/workspace/.platformio')
from platformio.package.manager import core
original = core.get_core_package_dir
def package_dir(name, *args, **kwargs):
    if name == 'tool-scons':
        return '/workspace/local-tools/tool-scons'
    return original(name, *args, **kwargs)
core.get_core_package_dir = package_dir
from platformio.__main__ import main
main()
