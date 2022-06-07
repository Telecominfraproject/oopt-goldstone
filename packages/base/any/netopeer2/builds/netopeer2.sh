#!/bin/bash

set -eux

NP2=/var/lib/netopeer2/scripts
export NP2_MODULE_OWNER=root
export NP2_MODULE_GROUP=gsmgmt
export NP2_MODULE_PERMS="664"
export NP2_MODULE_DIR=/var/lib/netopeer2/yang

install() {
    $NP2/setup.sh && $NP2/merge_hostkey.sh && $NP2/merge_config.sh
}

remove() {
    echo remove
}

start() {
    gs-mgmt.py start north-netconf --manifest-dir /var/lib/netopeer2/k8s
}

stop() {
    gs-mgmt.py stop north-netconf --manifest-dir /var/lib/netopeer2/k8s
}

case "$1" in
    install|remove|start|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {install|remove|start|stop}"
        exit 1
        ;;
esac
