#!/usr/bin/env bash
# Compiles the production nanite generator to one self-contained ES module for lab.html.
# Requires an activated Emscripten SDK (source emsdk_env.sh). Output: $1 (default build/nanite_lab).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
out="${1:-$root/build/nanite_lab}"
mkdir -p "$out"
em++ -std=c++23 -O2 -I"$root/include" \
    "$root/src/presentation/production/nanite_model.cpp" "$here/lab_bindings.cpp" \
    -sMODULARIZE=1 -sEXPORT_ES6=1 -sSINGLE_FILE=1 -sSINGLE_FILE_BINARY_ENCODE=0 -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web \
    -sEXPORTED_RUNTIME_METHODS=HEAPF32,HEAPU32,UTF8ToString \
    -o "$out/nanite_model.mjs"
# lab.html is the page body (the artifact host adds the document skeleton); wrap it for local use.
{
    printf '<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
    printf '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n'
    printf '<style>body{margin:0}</style>\n</head>\n<body>\n'
    cat "$here/lab.html"
    printf '</body>\n</html>\n'
} > "$out/index.html"
cp "$here/lab.html" "$out/lab.html"
echo "built $out"
