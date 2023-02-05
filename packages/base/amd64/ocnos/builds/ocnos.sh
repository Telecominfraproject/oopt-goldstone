#!/bin/bash

set -eux

if [ $# != 1 ]; then
        echo "Usage: $0 {start|stop}"
        exit 1
fi

start() {
    kubectl apply -f /var/lib/ocnos/k8s
    for pod in $(kubectl get pod -l name=ocnos -o jsonpath='{.items[*].metadata.name}'); do
        kubectl wait --for=condition=ready pod/$pod --timeout 5m || error
    done
}

stop() {
    kubectl delete -f /var/lib/ocnos/k8s
}

error() {
    stop
    echo "Failed to start OcNOS service. Please get OcNOS image from L2/L3 NOS vendor and make sure it is imported."
    exit 1
}

case "$1" in
    start|stop)
        $1
        ;;
    *)
esac
