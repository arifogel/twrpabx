#!/bin/bash
set -e
set -x
autoreconf -vfi ./configure.ac
./test.sh

