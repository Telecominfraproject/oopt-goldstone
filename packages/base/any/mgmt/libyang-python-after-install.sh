#!/bin/sh

set -eux

cd /var/lib/goldstone/wheels/libyang && python -m pip install --only-binary :all: *.whl
