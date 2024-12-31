#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [[ ! -d jerryscript-git ]]; then
  git clone --depth=1 https://github.com/jerryscript-project/jerryscript.git jerryscript-git
fi

cd jerryscript-git
python3 tools/amalgam.py --jerry-core --output-dir ../src/jerryscript/

