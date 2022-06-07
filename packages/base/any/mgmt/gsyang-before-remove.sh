#!/bin/sh

set -eux

systemctl disable gs-yang.service
systemctl stop gs-yang.service

groupdel gsmgmt
