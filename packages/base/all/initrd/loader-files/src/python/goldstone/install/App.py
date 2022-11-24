import os
import logging

from onl.install.App import App as ONLApp
from onl.install.ShellApp import OnieBootContext, OnieSysinfo
from onl.install import ConfUtils

import BaseInstall


class App(ONLApp):
    def runLocalOrChroot(self):

        if self.machineConf is None:
            self.log.error("missing onie-sysinfo or machine.conf")
            return 1
        if self.installerConf is None:
            self.log.error("missing installer.conf")
            return 1

        self.log.info("ONL Installer %s", self.installerConf.onl_version)

        code = self.findPlatform()
        if code:
            return code

        try:
            import onl.platform.current
        except:
            self.log.exception("cannot find platform config")
            code = 1
            if self.log.level < logging.INFO:
                self.post_mortem()
        if code:
            return code

        self.onlPlatform = onl.platform.current.OnlPlatform()

        if "grub" in self.onlPlatform.platform_config:
            self.log.info("trying a GRUB based installer")
            iklass = BaseInstall.GrubInstaller
        elif "flat_image_tree" in self.onlPlatform.platform_config:
            self.log.info("trying a U-Boot based installer")
            iklass = BaseInstall.UbootInstaller
        else:
            self.log.error("cannot detect installer type")
            return 1

        self.grubEnv = None

        if "grub" in self.onlPlatform.platform_config:
            ##self.log.info("using native GRUB")
            ##self.grubEnv = ConfUtils.GrubEnv(log=self.log.getChild("grub"))

            with OnieBootContext(log=self.log) as self.octx:

                self.octx.ictx.attach()
                self.octx.ictx.unmount()
                self.octx.ictx.detach()
                # XXX roth -- here, detach the initrd mounts

                self.octx.detach()

            if self.octx.onieDir is not None:
                self.log.info(
                    "using native ONIE initrd+chroot GRUB (%s)", self.octx.onieDir
                )
                self.grubEnv = ConfUtils.ChrootGrubEnv(
                    self.octx.initrdDir,
                    bootDir=self.octx.onieDir,
                    path="/grub/grubenv",
                    log=self.log.getChild("grub"),
                )
                # direct access using ONIE initrd as a chroot
                # (will need to fix up bootDir and bootPart later)

        if self.grubEnv is None:
            self.log.info("using proxy GRUB")
            self.grubEnv = ConfUtils.ProxyGrubEnv(
                self.installerConf,
                bootDir="/mnt/onie-boot",
                path="/grub/grubenv",
                chroot=False,
                log=self.log.getChild("grub"),
            )
            # indirect access through chroot host
            # (will need to fix up bootDir and bootPart later)

        if os.path.exists(ConfUtils.UbootEnv.SETENV):
            self.ubootEnv = ConfUtils.UbootEnv(log=self.log.getChild("u-boot"))
        else:
            self.ubootEnv = None

        # run the platform-specific installer
        self.installer = iklass(
            machineConf=self.machineConf,
            installerConf=self.installerConf,
            platformConf=self.onlPlatform.platform_config,
            grubEnv=self.grubEnv,
            ubootEnv=self.ubootEnv,
            force=self.force,
            log=self.log,
        )
        try:
            code = self.installer.run()
        except:
            self.log.exception("installer failed")
            code = 1
            if self.log.level < logging.INFO:
                self.post_mortem()
        if code:
            return code

        if getattr(self.installer, "grub", False):
            code = self.finalizeGrub()
            if code:
                return code
        if getattr(self.installer, "uboot", False):
            code = self.finalizeUboot()
            if code:
                return code

        self.log.info("Install finished.")
        return 0


main = App.main
