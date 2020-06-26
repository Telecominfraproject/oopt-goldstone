#!/bin/sh
for index in $(seq 4);
do
    ip link add vEthernet$index type veth peer name veth$index
    ip link set up dev vEthernet$index
    ip link set up dev veth$index
done

