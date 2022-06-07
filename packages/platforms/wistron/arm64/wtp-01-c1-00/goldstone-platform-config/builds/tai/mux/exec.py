#!/usr/bin/env python3

import sys
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

    with open(f"/sys/class/cfp2dco/cfp2dco{dev}/part_number") as f:
        try:
            part = f.read().strip()
        except Exception as e:
            print(f"failed to read part_number: {e}")
            return

        print(f"dev: {dev}, part: {part}", file=sys.stderr)

        if part == "LDC040-DO":
            print("libtai-ldc.so")
        elif part == "TRB100DAA-01" or part == "TRB200DAA-01":
            print("libtai-lumentum.so")


if __name__ == "__main__":
    main()
