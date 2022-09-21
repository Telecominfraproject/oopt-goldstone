#!/bin/bash

set -eux

start() {
    kubectl apply -k /var/lib/tai/k8s
    sleep 1
    kubectl wait --timeout=5m --for=condition=available deploy/tai
    kubectl wait --timeout=5m --for=condition=ready pod/$(kubectl get pod -l app=tai -o jsonpath='{.items[0].metadata.name}')
}


stop() {
    pod=$(kubectl get pod -l app=tai -o jsonpath='{.items[0].metadata.name}' 2>/dev/null || echo 'dummy' )
    kubectl delete --ignore-not-found -k /var/lib/tai/k8s
    kubectl delete --ignore-not-found pods/$pod
}

case "$1" in
    start|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac
