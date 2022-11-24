import os

from onl.install import BaseInstall as ONLBaseInstall
from onl.install.InstallUtils import MountContext

# Unlike the ONL default behavior, which just installs the SWI blob to the
# ONL-IMAGE partition and let the ONL initrd loader extract it to the ONL-DATA
# partition, installSwi() extract the SWI now without copying it to the ONL-IMAGE
# partition.
def installSwi(self):
    files = os.listdir(self.im.installerConf.installer_dir) + self.zf.namelist()
    swis = [x for x in files if x.endswith(".swi")]
    if not swis:
        self.log.info("No Goldstone Software Image available for installation.")
        self.log.info("Post-install ZTN installation will be required.")
        return
    if len(swis) > 1:
        self.log.warn("Multiple SWIs found in installer: %s", " ".join(swis))
        return

    base = swis[0]
    self.log.info("Installing Goldstone Software Image (%s)...", base)

    # populate /etc/onl/platform so that `swiprep` can copy to the rootfs
    onie_platform = None
    with open("/etc/machine.conf", "r") as f:
        for line in f.read().split():
            if line.startswith("onie_platform"):
                onie_platform = line.split("=")[-1]
                break

    if not onie_platform:
        self.log.error("Could not identify onie_platform")
        return

    self.log.info("Detected onie_platform: %s", onie_platform)

    with open("/etc/onl/platform", "w") as f:
        f.write(onie_platform.replace("_", "-"))

    self.installerCopy(base, "swi")

    data = self.blkidParts["ONL-DATA"]
    with MountContext(data.device, log=self.log) as ctx:
        self.check_call(("swiprep", "--install", "./swi", ctx.dir))


class GrubInstaller(ONLBaseInstall.GrubInstaller, object):
    def installSwi(self):
        return installSwi(self)


class UbootInstaller(ONLBaseInstall.UbootInstaller, object):
    def installSwi(self):
        return installSwi(self)
