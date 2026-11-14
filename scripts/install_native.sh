#!/usr/bin/env bash
# Install tools outside the existing isolated project checkout; never flash a device.
set -euo pipefail
TASK_ROOT=$(cd "$(dirname "$0")/.." && pwd)
export IDF_PATH=${IDF_PATH:-/workspace/esp-idf-5.4.1}
export IDF_TOOLS_PATH=${IDF_TOOLS_PATH:-/workspace/idf-tools}
if [[ ! -d "$IDF_PATH" ]]; then
    git clone --depth 1 --branch v5.4.1 --recursive --shallow-submodules \
        https://github.com/espressif/esp-idf.git "$IDF_PATH"
fi
TASK_IDF_REV=$(git -C "$IDF_PATH" rev-parse HEAD)
if [[ "$TASK_IDF_REV" != 4c2820d377d1375e787bcef612f0c32c1427d183 ]]; then
    echo 'Expected ESP-IDF v5.4.1; preserving the existing SDK without changing it.' >&2
    exit 1
fi
python3 "$IDF_PATH/tools/idf_tools.py" install --targets esp32s3
# The cloud policy blocks dl.espressif.com constraints/extra index. Use PyPI and
# the exact, tested dependency snapshot below. Artifact/TLS checks stay enabled.
if ! compgen -G "$IDF_TOOLS_PATH/python_env/idf5.4_py*_env/bin/python" >/dev/null; then
    IDF_PIP_WHEELS_URL='' python3 "$IDF_PATH/tools/idf_tools.py" install-python-env --no-constraints
fi
TASK_PYTHON=$(compgen -G "$IDF_TOOLS_PATH/python_env/idf5.4_py*_env/bin/python" | sort | tail -n 1)
"$TASK_PYTHON" -m pip install -r "$TASK_ROOT/scripts/requirements-idf.txt"
