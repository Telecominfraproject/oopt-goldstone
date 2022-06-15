#!/usr/bin/env python3

import sys

def main():
    if len(sys.argv) != 2:
        sys.exit(1)

    arg = sys.argv[1]

    if arg == 'list':
        return

if __name__ == '__main__':
    main()
