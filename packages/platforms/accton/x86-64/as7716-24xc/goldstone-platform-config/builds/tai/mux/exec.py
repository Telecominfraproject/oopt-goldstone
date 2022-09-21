#!/usr/bin/env python3

import sys
from pathlib import Path
import time

def main():
    if len(sys.argv) != 2:
        sys.exit(1)

    arg = sys.argv[1]

    if arg == 'list':
        dev = Path('/dev')
        print('\n'.join(str(v) for v in dev.glob('piu*')))
        return

    piu = arg.split('/')[-1]

    with open(f'/sys/class/piu/{piu}/piu_type') as f:
        type_ = f.read().strip()

    if type_ == 'ACO':
        print('libtai-aco.so')
        return
    elif type_ == 'DCO':

        try:
            with open(f'/sys/class/piu/{piu}/cfp2_vendor') as f:
                cfp2_vendor = f.read().strip()
        except:
            cfp2_vendor = None

        time.sleep(1)

        if cfp2_vendor == 'MENARA NETWORKS':
            print('libtai-menara.so')
        elif cfp2_vendor == 'LUMENTUM':
            print('libtai-lumentum.so')

if __name__ == '__main__':
    main()
