import click
from click import echo
import smbus
from .base import cli

FPGA_I2C_ADDR = 0x30
FPGA_REG_SCRATCH = 0x0
FPGA_REG_BRD_ID_VER = 0x1
FPGA_REG_FPGA_VER1 = 0x2
FPGA_REG_FPGA_VER2 = 0x3
FPGA_REG_BCM56960_ROV = 0x4
FPGA_REG_BCM82391_INT1 = 0x5
FPGA_REG_PHY_RESET = 0x6
FPGA_REG_PIU_PRESENT = 0x7
FPGA_REG_PSU_STATUS = 0x8
FPGA_REG_CPLD_STATUS = 0x9
FPGA_REG_CPU_CONTROL1 = 0xa
FPGA_REG_CPU_CONTROL2 = 0xb
FPGA_REG_CPU_CONTROL3 = 0xc
FPGA_REG_PIU_CONTROL1 = 0xd
FPGA_REG_PIU_CONTROL2 = 0xe
FPGA_REG_MOD_INT = 0xf
FPGA_REG_MOD_PRESENT = 0x10
FPGA_REG_MOD_RESET = 0x11
FPGA_REG_Q28_INT1 = 0x12
FPGA_REG_Q28_INT2 = 0x13
FPGA_REG_Q28_PRESENT1 = 0x14
FPGA_REG_Q28_PRESENT2 = 0x15
FPGA_REG_Q28_RESET1 = 0x16
FPGA_REG_Q28_RESET2 = 0x17
FPGA_REG_PG1 = 0x18
FPGA_REG_PG2 = 0x19
FPGA_REG_PG3 = 0x1a
FPGA_REG_POWER_RAIL_LED = 0x1b
FPGA_REG_PIU_LED = 0x1c
FPGA_REG_FPGA_ALARM = 0x1d
FPGA_REG_SYS_LED_CONTROL1 = 0x1e
FPGA_REG_SYS_LED_CONTROL2 = 0x1f
FPGA_REG_P12V_HOTSWAP_PIU = 0x20

@cli.group()
def fpga():
    pass

RESET_BITS = {
    'dpll': (FPGA_REG_PHY_RESET, 0x40),
    '88e6320': (FPGA_REG_PHY_RESET, 0x20),
    '5696x': (FPGA_REG_PHY_RESET, 0x10),
    'phy4': (FPGA_REG_PHY_RESET, 0x08),
    'phy3': (FPGA_REG_PHY_RESET, 0x04),
    'phy2': (FPGA_REG_PHY_RESET, 0x02),
    'phy1': (FPGA_REG_PHY_RESET, 0x01),
    'm.2-pcie': (FPGA_REG_CPU_CONTROL1, 0x40),
    '5696x-pcie': (FPGA_REG_CPU_CONTROL1, 0x20),
    'pca9548-1': (FPGA_REG_CPU_CONTROL1, 0x4),
    'pca9548-2': (FPGA_REG_CPU_CONTROL1, 0x10),
    'pca9548-3': (FPGA_REG_CPU_CONTROL1, 0x8),
    'cpld': (FPGA_REG_CPU_CONTROL1, 0x2),
    'com-e': (FPGA_REG_CPU_CONTROL2, 0x80),
    'lpc': (FPGA_REG_CPU_CONTROL2, 0x20),
}


@fpga.command()
@click.argument('device', type=click.Choice(RESET_BITS.keys(), case_sensitive=False))
def reset(device):
    device = device.lower()
    mask = 0
    if device == 'dpll':
        mask |= 0x40
    elif device == '88e6320':
        mask |= 0x20
    elif device == '5696x':
        mask |= 0x10
    elif device == 'phy4':
        mask |= 0x08
    elif device == 'phy3':
        mask |= 0x04
    elif device == 'phy2':
        mask |= 0x02
    elif device == 'phy1':
        mask |= 0x01
    else:
        raise click.BadParameter("Invalid device")
    try:
        i2c = smbus.SMBus(0)
        i2c.write_byte_data(FPGA_I2C_ADDR, 0x6, mask)
    except OSError:
        click.echo("Write to FPGA register failed")


@fpga.command()
@click.argument('device', type=click.Choice(['sys1', 'sys2', 'sys3', 'sys4', 'piu1', 'piu2', 'piu3', 'piu4',
                                             'piu5', 'piu6', 'piu7', 'piu8']))
@click.argument('color', type=click.Choice(['green', 'red']))
@click.argument('status', type=click.Choice(['on', 'blink', 'off']))
def led(device, color, status):
    SYS_LED_TABLE = {
        'sys1': (FPGA_REG_SYS_LED_CONTROL1, 2, 0),  # sys
        'sys2': (FPGA_REG_SYS_LED_CONTROL1, 6, 4),  # bmc
        'sys3': (FPGA_REG_SYS_LED_CONTROL2, 2, 0),  # fan
        'sys4': (FPGA_REG_SYS_LED_CONTROL2, 6, 4),  # power

    }
    i2c = smbus.SMBus(0)
    if device.startswith('sys'):
        reg, r_offset, g_offset = SYS_LED_TABLE[device]
        offset = g_offset if color == 'green' else r_offset
        if status == 'on':
            pattern = 0x3
        elif status == 'blink':
            pattern = 0x1
        else:
            pattern = 0x0

        cur = i2c.read_byte_data(FPGA_I2C_ADDR, reg)
        cur &= ~(0x3 << offset)
        cur |= (pattern << offset)
        i2c.write_byte_data(FPGA_I2C_ADDR, reg, cur)
    else:
        piu_no = int(device[-1])
        cur = i2c.read_byte_data(FPGA_I2C_ADDR, FPGA_REG_PIU_LED)
        if status == 'off':
            i2c.write_byte_data(FPGA_I2C_ADDR, FPGA_REG_PIU_LED, cur & ~(1 << (piu_no - 1)))
        else:
            i2c.write_byte_data(FPGA_I2C_ADDR, FPGA_REG_PIU_LED, cur | (1 << (piu_no - 1)))


@fpga.command()
def version():
    i2c = smbus.SMBus(0)
    fpga_ver1 = i2c.read_byte_data(FPGA_I2C_ADDR, 2)
    fpga_ver2 = i2c.read_byte_data(FPGA_I2C_ADDR, 3)
    echo("FPGA version:")
    echo("      Major: 0x%01x" % int(((fpga_ver1 & 0xff) >> 0)))
    echo("      Minor: 0x%01x" % int(((fpga_ver2 & 0xff) >> 0)))


@fpga.command()
def board():
    i2c = smbus.SMBus(0)
    brd_id_ver = i2c.read_byte_data(FPGA_I2C_ADDR, FPGA_REG_BRD_ID_VER)
    echo("Board version:")
    echo("    PCBA ID: 0x%01x" % int(((brd_id_ver & 0xc0) >> 6)))
    echo("    CHIP ID: 0x%01x" % int(((brd_id_ver & 0x20) >> 5)))
    echo("   BOARD ID: 0x%01x" % int(((brd_id_ver & 0x18) >> 3)))
    echo("   reserved: 0x%01x" % int(((brd_id_ver & 0x04) >> 2)))
    echo("     HW REV: 0x%01x" % int(((brd_id_ver & 0x03) >> 0)))


@fpga.command()
def psu():
    def psu_bit_msg(fmt, bit, one_msg, zero_msg):
        echo(fmt % (one_msg if (int(psu_status & (0x1 << bit)) > 0) else zero_msg))

    i2c = smbus.SMBus(0)
    psu_status = i2c.read_byte_data(FPGA_I2C_ADDR, FPGA_REG_PSU_STATUS)
    echo("PSU status: 0x%02x" % psu_status)
    psu_bit_msg("    PSU#1 PMBus Alert: %s", 0, "normal", "fail")
    psu_bit_msg("    PSU#2 PMBus Alert: %s", 1, "normal", "fail")
    psu_bit_msg("    PSU#1 SMB Alert: %s", 2, "normal", "alert")
    psu_bit_msg("    PSU#1 Present: %s", 3, "absent", "present")
    psu_bit_msg("    PSU#1 Power OK: %s", 4, "valid", "off/fail")
    psu_bit_msg("    PSU#2 SMB Alert: %s", 5, "normal", "alert")
    psu_bit_msg("    PSU#2 Present: %s", 6, "absent", "present")
    psu_bit_msg("    PSU#2 Power OK: %s", 7, "valid", "off/fail")


@fpga.command()
def cpld():
    def bit_msg(fmt, bit, one_msg, zero_msg):
        echo(fmt % (one_msg if (int(cpld_status & (0x1 << bit)) > 0) else zero_msg))

    i2c = smbus.SMBus(0)
    cpld_status = i2c.read_byte_data(FPGA_I2C_ADDR, FPGA_REG_CPLD_STATUS)
    echo("CPLD status: 0x%02x" % cpld_status)
    bit_msg("    CPLD interrupt: %s", 0, "normal", "interrupt")
    bit_msg("    PCIe interrupt: %s", 1, "normal", "interrupt")
    bit_msg("    PCIe WAKE: %s", 2, "1", "0")
    bit_msg("    Voltage Detector reset: %s", 3, "1", "0")
    bit_msg("    Fan fail: %s", 4, "normal", "alert")
    bit_msg("    TPM module present: %s", 5, "absent", "present")


@fpga.group()
def fan():
    pass


@fan.command()
@click.argument('m', type=click.Choice(['max', 'norm', 'show']))
def mode(m):
    i2c = smbus.SMBus(0)
    if m == 'max':
        echo("Setting fan to 100% speed:")
        i2c.write_byte_data(FPGA_I2C_ADDR, FPGA_REG_CPU_CONTROL3, 0x4)
    elif m == 'norm':
        echo("Setting fan to norm mode:")
        i2c.write_byte_data(FPGA_I2C_ADDR, FPGA_REG_CPU_CONTROL3, 0x0)
    else:
        out = i2c.read_byte_data(FPGA_I2C_ADDR, FPGA_REG_CPU_CONTROL3)
        if out & 0x4 > 0:
            echo("Forced max speed")
        else:
            echo("Normal operation")


@fan.command()
def test():
    import time
    from .sysinfo import _parse_ipmitool_sdr_type_fan
    i2c = smbus.SMBus(0)

    echo("Setting fan to 100% speed:")
    i2c.write_byte_data(FPGA_I2C_ADDR, FPGA_REG_CPU_CONTROL3, 0x4)

    all_max = False
    count = 1
    while not all_max:
        echo("=== Cycle %d ===:" % count)
        all_max = True
        fs = _parse_ipmitool_sdr_type_fan()
        for f in range(1, 6):
            for x in ['Front', 'Rear']:
                try:
                    idx = "Fan%d %s" % (f, x)
                    fan_speed = int(fs[idx][3].split()[0])
                    if x == 'Front':
                        max_speed = 24200
                    else:
                        max_speed = 22000
                    if max_speed * 0.9 <= fan_speed <= max_speed * 1.1:
                        echo("%s %d RPM --- Max speed reached" % (idx, fan_speed))
                    else:
                        echo("%s %d RPM" % (idx, fan_speed))
                        all_max = False
                except IndexError:
                    pass
        time.sleep(1)
        count += 1
        if count > 60:
            break

    echo("Setting fan back to normal speed:")
    i2c.write_byte_data(FPGA_I2C_ADDR, FPGA_REG_CPU_CONTROL3, 0x0)

    if all_max:
        echo("Test Passed")
    else:
        echo("Test failed (timed out)")

@fpga.group()
def piu():
    pass


@piu.command()
def status():
    i2c = smbus.SMBus(0)
    piu_status = i2c.read_byte_data(FPGA_I2C_ADDR, FPGA_REG_PIU_PRESENT)
    for x in range(1, 5):
        echo("PIU %d: %s" % (x, "Not Present" if (piu_status & (0x1 << (x - 1))) > 0 else "Present"))
