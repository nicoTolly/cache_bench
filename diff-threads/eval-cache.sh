#!/usr/bin/bash

DEFAULT_INPUT=inputs.txt

inputs=${1:-$DEFAULT_INPUT}

cat ${inputs} | ./test-script.py | cat cachesize.txt - | ./calculate_cache.py | awk '/full hits/ {print $1}' 
