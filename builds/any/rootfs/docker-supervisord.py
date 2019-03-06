#!/usr/bin/python

import os
import sys
import fileinput

rfsd = sys.argv[1]
overlays = os.path.join(rfsd, "var/lib/docker/overlay")

replacements = [ ("var/run/supervisor.sock", "dev/shm/supervisor.sock") ]

for (root, dirs, files) in os.walk(overlays):
    for f in files:
        if f == 'supervisord.conf':
            fname = os.path.join(root, f)
            sys.stderr.write("checking %s...\n" % (fname))
            for line in fileinput.input(fname, inplace=True):
                for (a, b) in replacements:
                    if a in line:
                        sys.stderr.write("%s: replacing '%s' with '%s'...\n" % (fname, a, b))
                        line = line.replace(a, b)
                print line,
