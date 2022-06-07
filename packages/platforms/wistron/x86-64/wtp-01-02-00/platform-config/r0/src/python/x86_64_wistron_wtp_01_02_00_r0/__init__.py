from onl.platform.base import *
from onl.platform.wistron import *
import time

class OnlPlatform_x86_64_wistron_wtp_01_02_00_r0(
    OnlPlatformWistron, OnlPlatformPortConfig_32x100
):
    PLATFORM = "x86-64-wistron-wtp-01-02-00-r0"
    MODEL = "WTP-01-02-00"
    SYS_OBJECT_ID = ".1.1"

    def baseconfig(self):

        self.insmod_platform()
        self.insmod("optoe")
        self.insmod("cfp2piu")
        self.insmod("kernel/drivers/misc/eeprom/at24")

        self.new_i2c_devices([("pca9548", 0x70, 0)])

        # initialize PIU 1-4
        self.new_i2c_devices(
            [
                ("piu1", 0x6A, 3),
                ("piu2", 0x6A, 5),
                ("piu3", 0x6A, 7),
                ("piu4", 0x6A, 9),
            ]
        )

        self.new_i2c_devices([("pca9548", 0x71, 0)])

        # initialize QSFP28 ports 1-8
        self.new_i2c_devices(
            [
                ("optoe1", 0x50, 11),
                ("optoe1", 0x50, 12),
                ("optoe1", 0x50, 13),
                ("optoe1", 0x50, 14),
                ("optoe1", 0x50, 15),
                ("optoe1", 0x50, 16),
                ("optoe1", 0x50, 17),
                ("optoe1", 0x50, 18),
            ]
        )

        self.new_i2c_devices([("pca9548", 0x72, 0)])

        # initialize QSFP28 ports 9-12
        self.new_i2c_devices(
            [
                ("optoe1", 0x50, 19),
                ("optoe1", 0x50, 20),
                ("optoe1", 0x50, 21),
                ("optoe1", 0x50, 22),
            ]
        )

        # instantiate sys-eeprom
        self.new_i2c_devices([("24c64", 0x54, 0), ("sys_fpga", 0x30, 0)])

        # Linux 5.4
        # https://github.com/torvalds/linux/commit/f1fb64b04bf414ab04e31ac107bb28137105c5fd
        for bus in ["0-0070", "0-0071", "0-0072"]:
            with open("/sys/bus/i2c/devices/{}/idle_state".format(bus), "w") as f:
                f.write("-2")
            time.sleep(0.5)

        self.set_onie_mac_address(["eth0"])

        return True
