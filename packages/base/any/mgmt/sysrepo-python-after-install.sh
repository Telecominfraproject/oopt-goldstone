#!/bin/sh

set -eux

cd /var/lib/goldstone/wheels/sysrepo && python -m pip install --only-binary :all: *.whl
