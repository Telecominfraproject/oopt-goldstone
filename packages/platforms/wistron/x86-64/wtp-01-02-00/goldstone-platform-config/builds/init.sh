#!/bin/sh

set -eux

systemctl enable gs-south-onlp
systemctl enable gs-south-tai
systemctl enable gs-south-system
systemctl enable gs-south-sonic
