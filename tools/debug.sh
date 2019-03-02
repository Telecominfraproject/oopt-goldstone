#!/bin/bash

cmd="$@"
toolsdir=$( cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)

set -x
. setup.env

make versions

bash -e -x $ONL/tools/scripts/submodule-updated.sh

make modules

if test $# -eq 0; then
  exec bash -i
fi

cmd="IFS=; set -e; set -x; $cmd; exit 0"
exec bash -c "$cmd"

set +x
