#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
TASK_TEST_DIR=$(mktemp -d /tmp/tiko-tests.XXXXXX)
trap 'rm -rf "$TASK_TEST_DIR"' EXIT
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined tests/core_test.cpp -o "$TASK_TEST_DIR/core"
"$TASK_TEST_DIR/core"
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc tests/native_policy_test.cpp firmware/components/tiko/policy.cpp -o "$TASK_TEST_DIR/policy"
"$TASK_TEST_DIR/policy"
cc -std=gnu17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Itests/native_input/stubs -Ifirmware/components/input -I. \
  tests/native_input/test_input.c firmware/targets/lilygo_t_embed_cc1101/target_input.c -o "$TASK_TEST_DIR/input"
"$TASK_TEST_DIR/input"
