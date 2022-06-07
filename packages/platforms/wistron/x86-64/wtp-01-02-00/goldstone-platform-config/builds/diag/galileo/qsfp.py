import smbus
import click
from click import echo
from .base import cli


@cli.group()
def qsfp():
    pass


@qsfp.command()
@click.argument('port', type=click.IntRange(1, 12))
def reset(port):
    def q28_reset(port):
        i2c_0 = smbus.SMBus(0)
        if 1 <= port <= 8:
            reset_bit = (0x1 << (port - 1))
            i2c_0.write_byte_data(0x30, 0x16, reset_bit)
        else:
            reset_bit = (0x1 << (port - 9))
            i2c_0.write_byte_data(0x30, 0x17, reset_bit)
    print("Resetting QSFP on port %d..." % port)
    q28_reset(port)


@qsfp.command()
def status():
    def get_string(s):  # copy the cbuffer into a string
        result = ''
        for c in s:
            result += chr(c)
        return result

    def q28_present(port) -> bool:
        i2c_0 = smbus.SMBus(0)
        if 1 <= port <= 8:
            present_bits = i2c_0.read_byte_data(0x30, 0x14)
            return not ((present_bits & (0x1 << (port - 1))) > 0)
        else:
            present_bits = i2c_0.read_byte_data(0x30, 0x15)
            return not ((present_bits & (0x1 << (port - 9))) > 0)

    def q28_interrupt(port) -> bool:
        i2c_0 = smbus.SMBus(0)
        if 1 <= port <= 8:
            present_bits = i2c_0.read_byte_data(0x30, 0x12)
            return not ((present_bits & (0x1 << (port - 1))) > 0)
        else:
            present_bits = i2c_0.read_byte_data(0x30, 0x13)
            return not ((present_bits & (0x1 << (port - 9))) > 0)

    print("Port\tPresent\tInt\tVendor\t\t\tPart #\t\t\tSerial #")
    for x in range(1, 13):
        present = "Yes" if q28_present(x) else "No"
        interrupt = "Yes" if q28_interrupt(x) else "No"
        try:
            i2c = smbus.SMBus(x + 8)
            block = i2c.read_i2c_block_data(0x50, 148, (163-148+1))
            vendor_name = get_string(block)
            block = i2c.read_i2c_block_data(0x50, 168, (183-168+1))
            vendor_pn = get_string(block)
            block = i2c.read_i2c_block_data(0x50, 196, (211-196+1))
            vendor_sn = get_string(block)

            print("%d\t%s\t%s\t%s\t%s\t%s" % (x, present, interrupt, vendor_name, vendor_pn, vendor_sn))
            i2c.close()
        except OSError:
            print("%d\t%s\t%s\tNo module" % (x, present, interrupt))


@qsfp.command()
def temp():
    def q28_temp(port):
        i2c = smbus.SMBus(port)
        try:
            block = i2c.read_i2c_block_data(0x50, 22, 2)
            if block[0] >= 128:
                reading = -(block[0] - 128 + block[1] / 256)
            else:
                reading = block[0] + block[1] / 256
        except OSError:
            return None
        return reading

    echo("Port\tTemperature (degC)")
    for x in range(1, 13):
        t = q28_temp(x + 8)
        if t is None:
            echo("%d\tNo module")
        else:
            print("%d\t%5.2f" % (x, t))


@qsfp.group()
def allbest():
    pass


@allbest.command()
@click.argument('port', type=click.IntRange(1, 12))
@click.argument('watt', type=click.Choice(['0.5', '5.0']))
def power(port, watt):
    i2c = smbus.SMBus(port + 8)
    # set power without detecting LPMode pin
    if watt == '0.5':
        orig = i2c.read_byte_data(0x50, 93)
        i2c.write_byte_data(0x50, 93, 0x3 | orig)
        orig = i2c.read_byte_data(0x50, 110)
        i2c.write_byte_data(0x50, 110, 0x30 | orig)
        d1 = i2c.read_byte_data(0x50, 93)
        d2 = i2c.read_byte_data(0x50, 110)
        print(f"byte 93: 0x{d1:x} , byte 110: 0x{d2:x}")

    else:
        orig = i2c.read_byte_data(0x50, 93)
        i2c.write_byte_data(0x50, 93, 0x5 | (orig & ~0x7))
        orig = i2c.read_byte_data(0x50, 129)
        i2c.write_byte_data(0x50, 129, 0x3 | orig)
        d1 = i2c.read_byte_data(0x50, 93)
        d2 = i2c.read_byte_data(0x50, 129)
        print(f"byte 93: 0x{d1:x} , byte 129: 0x{d2:x}")
