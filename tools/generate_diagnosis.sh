#!/usr/bin/env bash

set -uo pipefail   # ← removed -e so we can see failures

MINE="mine"
REFERENCE="reference"
DIAGNOSIS="diagnosis.json"

rm -rf "$MINE" "$REFERENCE" "$DIAGNOSIS"

echo "Running mos6502..."
../cmake-build-debug/mos6502 > "$MINE"
echo "✔ mine created"

echo "Running py65_test.py..."
if python3 py65_test.py > "$REFERENCE"; then
    echo "✔ reference created"
else
    echo "✘ py65_test.py FAILED"
    exit 1
fi

echo "Running diff..."
if python3 diff.py "$MINE" "$REFERENCE" > "$DIAGNOSIS"; then
    echo "✔ diagnosis.json created"
else
    echo "✘ diff.py FAILED"
    exit 1
fi

echo "Done."