#!/bin/sh

set -eux

cd /var/lib/goldstone/wheels/cli && python -m pip install *.whl
cd /var/lib/goldstone/wheels/lib && python -m pip install *.whl
