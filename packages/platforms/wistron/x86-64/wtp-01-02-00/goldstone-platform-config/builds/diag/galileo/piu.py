import click
import ioctl
import ioctl.linux
from ctypes import *
import os
from .base import cli
import tabulate
from click import echo
from pathlib import Path
import json as j


class PIUCmd(Structure):
    _fields_ = [("reg", c_uint), ("offset", c_uint), ("val", c_uint)]


IOC_MAGIC = "p"

PIU_READ = ioctl.linux.IOWR(IOC_MAGIC, 1, sizeof(PIUCmd))
PIU_WRITE = ioctl.linux.IOWR(IOC_MAGIC, 2, sizeof(PIUCmd))
PIU_INIT = ioctl.linux.IOWR(IOC_MAGIC, 3, sizeof(PIUCmd))


@cli.group(invoke_without_command=True)
@click.pass_context
@click.option('--json', '-j', is_flag=True)
def piu(ctx, json):

    if ctx.invoked_subcommand:
        return

    p = Path('/sys/class/piu')
    table = []

    def r(path):
        try:
            with open(path, 'r') as f:
                return f.read().strip()
        except Exception:
            return 'n/a'

    for d in sorted(p.iterdir()):
        row = [str(d).split('/')[-1], r(d/'piu_type'), r(d/'piu_mcu_version'), r(d/'piu_temp'), r(d/'cfp2_cage_temp'), r(d/'cfp2_tx_laser_temp'), r(d/'cfp2_rx_laser_temp')]
        table.append(row)

    if json:
        echo(j.dumps({v[0]: {'type': v[1], 'mcu-version': v[2], 'piu-temp': v[3], 'cfp2-cage-temp': v[4], 'cfp2-tx-laser-temp': v[5], 'cfp2-rx-laser-temp': v[6]} for v in table}))
    else:
        echo(tabulate.tabulate(table, headers=['Name', 'Type', 'MCU Version', 'PIU Temp', 'CFP2 Cage Temp', 'CFP2 TX Laser Temp', 'CFP2 RX Laser Temp']))

@piu.command()
@click.argument("slot", type=click.IntRange(1, 4))
@click.argument("addr")
def read(slot, addr):
    fd = os.open(f"/dev/piu{slot}", os.O_RDWR)
    try:
        cmd = PIUCmd(reg=int(addr, 0))
        ioctl.ioctl(fd, PIU_READ, byref(cmd))
        print(hex(cmd.val))
    finally:
        os.close(fd)


@piu.command()
@click.argument("slot", type=click.IntRange(1, 4))
@click.argument("addr")
@click.argument("value")
def write(slot, addr, value):
    fd = os.open(f"/dev/piu{slot}", os.O_RDWR)
    try:
        cmd = PIUCmd(reg=int(addr, 0), val=int(value, 0))
        ioctl.ioctl(fd, PIU_WRITE, byref(cmd))
    finally:
        os.close(fd)
