#!/bin/bash
wc -l `find . \( -name "*.c" -o -name "*.h" -o -name "*.cpp" \) -print` | \
         tail -1 | awk '{print $1}'