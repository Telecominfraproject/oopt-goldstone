#!/usr/bin/env python3

import sys
import json
from pathlib import Path

def main():
    if len(sys.argv) != 2:
        sys.exit(1)

    arg = sys.argv[1]

    if arg == "list":
        dev = Path("/dev")
        l = "\n".join(str(v).replace("/dev/cfp2dco", "") for v in dev.glob("cfp2dco*"))
        print(f"list: {l}", file=sys.stderr)
        print(l)
        return

    dev = arg.split("/")[-1]
    
    with open(f'/etc/tai/mux/cfp2dco.json') as f:
        cfp2dco = json.load(f)
    
    with open(f"/sys/class/cfp2dco/cfp2dco{dev}/part_number") as f:
        try:
            part = f.read().strip()
        except Exception as e:
            print(f"failed to read part_number: {e}")
            return

        print(f"dev: {dev}, part: {part}", file=sys.stderr)

        if part in cfp2dco:
            print("{}".format(cfp2dco[part]))

if __name__ == "__main__":
    main()
