import sys
import click
from click_shell import make_click_shell

from .base import cli

from .fpga import fpga
from .qsfp import qsfp
from .piu import piu

def main():
    if '-c' in sys.argv:
        sys.argv = [v for v in sys.argv if v != '-c']
        cli()
    else:
        shell = make_click_shell(click.Context(cli), prompt='Galileo > ', intro='Starting Galileo Diagnostics...')
        shell.cmdloop()

if __name__ == '__main__':
    main()
