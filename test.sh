#!/bin/bash
set -e
./configure CFLAGS='-Wall -Wextra -Werror -Wno-unused-parameter -O0 -g' --enable-log-level=LOG_DEBUG
make clean
make
if ! make check; then
  echo "Test failure!" 1>&2
  find . -name '*.log' | sort | while read i; do echo "$i:"; cat "$i"; echo; done 1>&2
  exit 1
fi
echo "Success! All tests passed."

