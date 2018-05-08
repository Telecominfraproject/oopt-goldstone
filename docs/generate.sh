#!/bin/bash

if [ "$1" == "clean" ]; then
    sudo rm -rf RELEASE/ REPO/ builds/ make/ packages/ setup.env  tools/ .gitignore
    exit 0
fi

#
# This repository was originally generated using the following command
# run from the root of the repository:

sm/ONL/tools/onl-nos-create.py \
    --root . \
    --name "SuperSONiC Network OS" \
    --prefix x1 \
    --arches amd64 \
    --debian stretch \
    --copyright "Copyright 2018 Big Switch Networks" \
    --maintainer "support@bigswitch.com" \
    --version 1.0.0 \
    --changelog "Initial" \
    --support support@bigswitch.com \
    --csr-C US \
    --csr-ST CA \
    --csr-O "Open Compute Project" \
    --csr-localityName "Santa Clara" \
    --csr-commonName Networking \
    --csr-organizationUnitName "Open Network Linux" \
    --csr-emailAddress support@bigswitch.com \
    --write-files \
    $@ \
    # BLANK
