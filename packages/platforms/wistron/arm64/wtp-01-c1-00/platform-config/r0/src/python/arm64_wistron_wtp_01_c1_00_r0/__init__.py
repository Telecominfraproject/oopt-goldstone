from onl.platform.base import *
from onl.platform.wistron import *

import subprocess

class OnlPlatform_arm64_wistron_wtp_01_c1_00_r0(OnlPlatformWistron,
                                                 OnlPlatformPortConfig_32x100):
    PLATFORM='arm64-wistron-wtp-01-c1-00-r0'
    MODEL="WTP-01-C1-00"
    SYS_OBJECT_ID=".1.1"

    def baseconfig(self):

        self.insmod("optoe")

        # initialize QSFP28 ports 1-16
        self.new_i2c_devices(
            [("optoe1", 0x50, i) for i in range(2,18)]
        )

        self.set_onie_mac_address(["eth0", "swp0", "swp1", "swp2"])

        command = 'ip link set can0 type can bitrate 1000000'
        subprocess.check_call(command.split())

        command = 'ifconfig can0 up'
        subprocess.check_call(command.split())

        return True

    def onie_boot_mode_set(self, mode):
        subprocess.check_call(["fw_setenv", "onie_boot_reason", mode])

    def boot_onie(self):
        subprocess.check_call(["fw_setenv", "boot_onie", "1"])
