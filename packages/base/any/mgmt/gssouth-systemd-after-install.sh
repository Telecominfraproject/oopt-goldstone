#!/bin/sh

set -eux

cd /var/lib/goldstone/wheels/system && python -m pip install *.whl
