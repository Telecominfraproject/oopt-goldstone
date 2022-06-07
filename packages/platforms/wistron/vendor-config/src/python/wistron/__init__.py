#!/usr/bin/python

from onl.platform.base import *
class OnlPlatformWistron(OnlPlatformBase):
    MANUFACTURER='Wistron'
    PRIVATE_ENTERPRISE_NUMBER=11161

    def set_onie_mac_address(self, intfs):

        import onlp.onlp
        import ctypes
        import subprocess
        import logging

        libonlp = onlp.onlp.libonlp

        sys = onlp.onlp.onlp_sys_info()
        libonlp.onlp_sys_info_get(ctypes.byref(sys))
        mac = ":".join("{:02x}".format(v) for v in sys.onie_info.mac)

        for intf in intfs:
            try:
                subprocess.check_call(["ip", "link", "set", "dev", intf, "address", mac])
            except Exception as e:
                logging.warning("failed to set MAC address '{}' to {}: {}".format(mac, intf, e))
