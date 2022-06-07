#!/bin/sh

set -eux

systemctl disable tai-dpll.service
systemctl stop tai-dpll.service
