#!/usr/bin/python
############################################################
#
# Generate ZTN Manifest files
#
# This script converts ONL manifest.json files
# to zerotouch.json files for backwards compatibility purposes.
#
############################################################
import argparse
import json

ap=argparse.ArgumentParser(description="ZTN Manifest Generator.")

ap.add_argument("--manifest-version", help="Specify manifest version.",
                default="1")
ap.add_argument("--operation", help="Manifest operation.", required=True)
ap.add_argument("--manifest", help="ONL Source Manifest", required=True)

ops = ap.parse_args();

manifest = json.load(open(ops.manifest))

# Operation shortcuts
operations=dict(swi='ztn-runtime', installer='os-install')

if ops.operation in operations:
    ops.operation = operations[ops.operation]

zerotouch = dict(manifest_version=ops.manifest_version,
                 release=manifest['version']['RELEASE_ID'],
                 platform=','.join(manifest['platforms']),
                 operation=ops.operation,
                 sha1=manifest['version']['BUILD_SHA1'])

print json.dumps(zerotouch, indent=2)
