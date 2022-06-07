#!/bin/sh

set -eux

cd /var/lib/galileo/wheels && python -m pip install --no-deps *.whl
