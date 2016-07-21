#!/usr/bin/bash

DEFAULT_INPUT=inputs.txt

inputs=${1:-$DEFAULT_INPUT}

cat ${inputs} | ./test-script.py | ./load | awk '/Throughput/ {print($3)}' | sort -nr
