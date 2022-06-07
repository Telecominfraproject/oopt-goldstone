#!/bin/bash

set -eux

# For SONiC hostname should be "localhost", so its hardcoded below.
# Seems some places of SONiC has been hardcoded with hostname as "localhost",
# so vlanmgrd container fails to start when hostname is different.
#
# Even on official SONiC builds, system hostname and hostname in Redis are
# different
create_config_db() {
    exec 3<<< $(jq -n "{DEVICE_METADATA: {localhost: {hwsku: \"$(cat /etc/goldstone/platform)\",\
                                            mac: \"$(cat /sys/class/net/eth0/address)\",\
                                            platform: \"$(cat /etc/goldstone/platform)\"}}}")
}

create_sonic_version() {
    exec 4<<< "build_version: $(jq .version.PRODUCT_ID_VERSION /etc/goldstone/rootfs/manifest.json)
asic_type: broadcom
commit_id: $(jq .version.BUILD_SHORT_SHA1 /etc/goldstone/rootfs/manifest.json)
build_date: $(jq .version.BUILD_TIMESTAMP /etc/goldstone/rootfs/manifest.json)
build_number: 0
built_by: '-'
debian_version: $(jq '."os-release".VERSION' /etc/goldstone/rootfs/manifest.json)
kernel_version: $(uname -r)
"
}


start() {
    mkdir -p /var/lib/usonic/redis
    create_config_db
    create_sonic_version
    kubectl create configmap usonic-config \
        --from-file=config_db.json=/dev/fd/3 \
        --from-file=sonic_version.yml=/dev/fd/4 \
        --from-file="/var/lib/goldstone/device/current/usonic" \
        --from-literal=macaddress="$(cat /sys/class/net/eth0/address)" \
        --dry-run=client -o yaml | kubectl apply -f -
    kubectl apply -f /var/lib/usonic/k8s
    kubectl wait --for=condition=available deploy/usonic-core --timeout=5m
    for pod in $(kubectl get pod -l app=usonic -o jsonpath='{.items[*].metadata.name}'); do
        kubectl wait --for=condition=ready pod/$pod --timeout 5m
    done
}


stop() {
    kubectl delete -f /var/lib/usonic/k8s
    kubectl delete configmap usonic-config
    rm -rf /var/lib/usonic/redis
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
