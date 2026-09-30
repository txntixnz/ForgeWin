#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
python3 tools/make_demo.py
cxx="${CXX:-c++}"
"$cxx" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -g tests/tests.cpp -o build/tests
./build/tests
"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror core/main.cpp -o build/forgewin
./build/forgewin tests/sum55.exe

./build/forgewin tests/winapi71.exe
