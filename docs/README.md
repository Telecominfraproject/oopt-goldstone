# Goldstone Introduction

Goldstone is a reference open-source Network Operating System (NOS) for disaggregated open optical networking hardware. Goldstone is developed as part of Telcom-infra-project’s [OOPT-NOS (Open Optical and Packet Transport)](https://telecominfraproject.com/oopt/) software project group. It is mainly aimed at accelerating the adoption of disaggregated networking optical systems.

Several open-source components developed as part of [OCP(Open Compute Project)](https://www.opencompute.org/projects/networking), [TIP(Telcom Infra project)](https://telecominfraproject.com/oopt/) and [ONL(Open Network Linux)](http://opennetlinux.org/) are used as the basic building blocks of Goldstone NOS.
1. ONL &emsp; &ensp; &ensp; &nbsp;: [ONL](https://github.com/oopt-goldstone/OpenNetworkLinux/tree/aa72eb5e1b1ba717033e9072afc5b488659a9bf8) is used as base OS and build-system in Goldstone to support wide range of open networking hardware.
2. SONIC &emsp; &ensp; : To provision switching and routing support for networking platforms.
3. SAI &emsp; &ensp; &ensp; &ensp; : Hardware Abstraction layer to control/manage forwarding ASICs/NPUs.
4. TAI &emsp; &ensp; &ensp; &ensp; : Hardware abstraction layer to control/manage coherent optics of CFP2ACO and CFP2DCO.
5. Docker &emsp; &nbsp; : Simplifies the process of building, running, managing, and distributing applications.
6. Kubernetes : Enables containerized application management.


# System-Architecture

![image](https://github.com/HarshaF1/goldstone-buildimage/assets/36222193/e15ebe79-f438-41b9-b7ad-e5be89bed652)

This modular architecture brings in the ease of assimilating new technologies and feature-advancements of open-source projects mentioned above into Goldstone-NOS. And also enables Goldstone to extend its support for other networking devices [L0/L1 Transponders, ROADMs.. etc.] in future.

### _Goldstone Management Layer_
The Goldstone management layer provides a common APIs towards different north bound management protocols like CLI, netconf, restconf, gNMI SNMP etc.

![image](https://github.com/HarshaF1/goldstone-buildimage/assets/36222193/ead19580-02eb-4102-a77b-09f4a8b73c81)

Goldstone uses 'Sysrepo', a library for configuration and monitoring based on the YANG model as a core component of the management layer.
Goldstone-management-layer's architecture is such that each component that caters to the northbound API is an independent process and does not require changes to existing parts when adding a new northbound API.
Goldstone-mgmt framework uses sysrepo as a central configuration infrastructure and has four kinds of daemons which interact with sysrepo datastore.
- north daemon
    - provides northbound API (CLI, NETCONF, SNMP, RESTCONF, gNMI etc..)
    - source code under [`src/north`](https://github.com/oopt-goldstone/goldstone-mgmt/tree/master/src/north)
- south daemon
    - control/monitor hardware (ONLP, SONiC/SAI, TAI, System)
    - uses native YANG models to interact with sysrepo
    - source code under [`src/south`](https://github.com/oopt-goldstone/goldstone-mgmt/tree/master/src/south)
- translation daemon
    - translator of the standarized YANG models and Goldstone YANG models
    - source code under [`src/xlate`](https://github.com/oopt-goldstone/goldstone-mgmt/tree/master/src/xlate)
- system daemon
    - provides system utility services for north daemons
    - optionally uses native YANG models to interact with sysrepo
    - source code under [`src/system`](https://github.com/oopt-goldstone/goldstone-mgmt/tree/master/src/system)

More information <[here](https://github.com/oopt-goldstone/goldstone-mgmt)>.

### _ONL & ONLP_
Open Network Linux (ONL) is an open-source, foundational platform software layer for next-generation, modular NOS architecture on open networking hardware.

More information <[here](http://opennetlinux.org/)>.

The Open Network Linux Platform (ONLP) APIs provide a common, consistent abstraction interface for accessing important platform assets such as SFPs, PSUs, Fans, Thermals, LEDs, and ONIE TLV storage devices.

More information <[here](https://opencomputeproject.github.io/OpenNetworkLinux/onlp/)>.

### _TAI [Transponder Abstraction Interface]_
Transponder Abstraction Interface [TAI], defines API’s to provide a vendor independent way of programming the transponders from various vendors. 
TAI acts as hardware abstraction interface between the system software (NOS) and the coherent optical devices. There by allowing all the TAI complaint transponders to operate with any system software having TAI layer integrated.

![image](https://github.com/HarshaF1/goldstone-buildimage/assets/36222193/52470397-8319-4f71-8d17-99444d3d76a6)

More information <[here](https://github.com/Telecominfraproject/oopt-tai)>.

### _SONIC & SAI_
SONiC is an open source network operating system based on Linux that runs on switches from multiple vendors and ASICs. SONiC offers a full-suite of network functionality, like BGP and other L2 & L3 protocols.

More information <[here](https://azure.github.io/SONiC/)>.

The Switch Abstraction Interface defines the API to provide a vendor-independent way of controlling forwarding elements, such as a switching ASIC, an NPU or a software switch in a uniform manner.

More information <[here](https://github.com/opencomputeproject/SAI/wiki)>.

### _OcNOS_
Containarized OcNOS is a commercial offering from Ipinfusion that can be used as a Ethernet-asic-controller instead of SONiC + SAI. OcNOS offers an extensive suite of switching(L2) and routing(L3) protocols and also provides data-plane layer to control/manage the switch-asic.

# Supported platforms

#### Following is the list of platforms that supports Goldstone NOS.

| S.No | Vendor   | Platform                      | ASIC Vendor | Switch ASIC | Port Configuration      | Comments                           |
| ---- | -------- | ----------------------------- | ----------- | ----------- | ----------------------- | ---------------------------------- |
| 1    | Wistron  | WTP-01-02-00 (Galileo 1)      | Broadcom    | TH+         |                         | ACO, DCO, QSFP28 PIU are supported |
| 2    | Wistron  | WTP-01-C1-00 (Galileo Flex T) | Broadcom    | TH+         |                         |                                    |
| 3    | Edgecore | AS7716-24SC/XC  (Cassini)     | Broadcom    | TH+         | 16x100G + 8x200G        | only ACO PIU is supported          |
| 4    | Edgecore | AS7716-32X                    | Broadcom    | TH+         | 32x100G                 |                                    |
| 5    | Edgecore | AS7316-26XB (CSR320)          | Broadcom    | Qumran-AX   | 2x100G + 8x25G + 16x10G |                                    |
| 4    | Edgecore | AS7946-30XB (AGR400)          | Broadcom    | Qumran2C    | 4x400G + 22x100G + 4x25G|                                    |


# Building Guide

### Getting Started
You can download pre-built Goldstone ONIE installer from the [release](https://github.com/oopt-goldstone/goldstone-buildimage/releases) page. Otherwise you can follow the below instructions to build an image locally.

### How to build

#### Hardware:
- Multiple cores to increase build speed.
- 8GB+ RAM.
- 200G+ of free disk space.
- KVM Virtualization Support.
- Ubuntu 20.04.

#### Prerequisites:
Packages that needs to be installed on the build server for building Goldstone NOS
- ca-certificates curl gnupg
- make  dpkg-dev python-is-python3
- [Docker](https://docs.docker.com/engine/install/ubuntu/) ( version >= 20.10.12, enable [buildkit](https://docs.docker.com/develop/develop-images/build_enhancements/) )
  - Enable an execution of different multi-architecture containers by QEMU and binfmt_misc
    - You can do this by running `docker run --rm --privileged multiarch/qemu-user-static --reset -p`
    - See https://github.com/multiarch/qemu-user-static for details

#### Build steps:
```
# Clone the repository
$ git clone https://github.com/oopt-goldstone/goldstone-buildimage.git

# Enter the source directory
$ cd goldstone-buildimage

# (Optional) Checkout a specific branch. By default, it uses master branch.
# For example, to checkout the branch sm-sonic-202205, use "git checkout sm-sonic-202205"
$ git checkout [branch_name]

# Initialize and update the submodules (sonic-buildimage & ONL)
$ git submodule update --init

$ make builder

$ make docker

```

This will build [ONIE](https://opencomputeproject.github.io/onie/) installers that can be installed on the supported hardware under `RELEASE` directory.
```
$ find RELEASE
RELEASE
RELEASE/buster
RELEASE/buster/arm64
RELEASE/buster/arm64/goldstone-v0.8.0_ONL-OS10_2022-06-13.0547-02419c5_ARM64.swi.md5sum
RELEASE/buster/arm64/goldstone-v0.8.0_2022-06-13.0547-02419c5_ARM64_INSTALLER
RELEASE/buster/arm64/goldstone-v0.8.0_ONL-OS10_2022-06-13.0547-02419c5_ARM64.swi
RELEASE/buster/arm64/goldstone-v0.8.0_2022-06-13.0547-02419c5_ARM64_INSTALLER.md5sum
RELEASE/buster/amd64
RELEASE/buster/amd64/goldstone-v0.8.0_ONL-OS10_2022-06-13.0547-02419c5_AMD64.swi
RELEASE/buster/amd64/goldstone-v0.8.0_ONL-OS10_2022-06-13.0547-02419c5_AMD64.swi.md5sum
RELEASE/buster/amd64/goldstone-v0.8.0_2022-06-13.0547-02419c5_AMD64_INSTALLER
RELEASE/buster/amd64/goldstone-v0.8.0_2022-06-13.0547-02419c5_AMD64_INSTALLER.md5sum    
```
And all the packages(*.deb) built can be found under `REPO` directory
```
goldstone-buildimage/REPO$ find * -type d
buster
buster/packages
buster/packages/binary-amd64
buster/packages/binary-all
buster/packages/binary-armhf
buster/packages/binary-arm64
buster/packages/binary-armel
```

# Installation Guide
Goldstone NOS uses [ONIE](https://opencomputeproject.github.io/onie/) to install/uninstall the images. Please follow the below mentioned steps to intall a new image. 

1. Uninstall the exising image on the box. This step is optional and need to be done only if the box has a NOS installed other than goldstone.

     ```
                         GNU GRUB  version 2.02~beta2+e4a1fe391
     +----------------------------------------------------------------------------+
     |*ONIE: Install OS                                                           | 
     | ONIE: Rescue                                                               |
     | ONIE: Uninstall OS  <----- Select this one                                 |
     | ONIE: Update ONIE                                                          |
     | ONIE: Embed ONIE                                                           |
     +----------------------------------------------------------------------------+

          Use the ^ and v keys to select which entry is highlighted.          
          Press enter to boot the selected OS, `e' to edit the commands       
          before booting or `c' for a command-line.                           
     ```
2. Reboot the switch into ONIE and select Install OS:
     ```
                         GNU GRUB  version 2.02~beta2+e4a1fe391
     +----------------------------------------------------------------------------+
     |*ONIE: Install OS    <----- Select this one                                 | 
     | ONIE: Rescue                                                               |
     | ONIE: Uninstall OS                                                         |
     | ONIE: Update ONIE                                                          |
     | ONIE: Embed ONIE                                                           |
     +----------------------------------------------------------------------------+

          Use the ^ and v keys to select which entry is highlighted.          
          Press enter to boot the selected OS, `e' to edit the commands       
          before booting or `c' for a command-line.                           
     ```

3. There are many methods to install the installer-onie-image and all those methods are explained [here](https://opencomputeproject.github.io/onie/user-guide/index.html#user-guide) in detail.
   Following example shows how to copy the image over the network and install it.
   Configure the management Ip and default-gateway as per your network settings.
   ```
   ONIE:/ # ifconfig eth0 192.168.0.2 netmask 255.255.255.0
   ONIE:/ # ip route add default via 192.168.0.1
   ```
   And then you can copy the image from remote/build server to the box using the newly assigned Ip address and install using `onie-nos-install` cnmd.
   ```
   ONIE:/ # onie-nos-install goldstone-v0.8.0-17-ge5139a9_2023-08-08.1053-e5139a9_AMD64_INSTALLER
   ```
   When installation finishes, the box will boot into Goldstone by default.
   ```
                        GNU GRUB  version 2.02~beta2+e4a1fe391
   +----------------------------------------------------------------------------+
   |*Goldstone                                                                  |
   | ONIE                                                                       |
   |                                                                            |
   +----------------------------------------------------------------------------+

        Use the ^ and v keys to select which entry is highlighted.
        Press enter to boot the selected OS, `e' to edit the commands
        before booting or `c' for a command-line.
   The highlighted entry will be executed automatically in 2s.
   ```
#### Initial setup/checks
It will take several minutes for k3s to set up kubernetes cluster and load all container images to the local repository. Once the Goldstone NOS boots up , use `root/x1` or `admin/admin` to login to the shell.
```
localhost login: root
Password:
Linux localhost 5.4.40-OpenNetworkLinux #1 SMP Mon Aug 28 08:25:37 UTC 2023 x86_64
root@localhost:~#
```
Use 'kubectl get pods' to check the status of the pods and make sure all the pods are in `running` status.
```
root@localhost:~# k get pods
NAME                               READY   STATUS    RESTARTS   AGE
tai-56cd5cd86c-27q6q               1/1     Running   0          5m
usonic-cli                         1/1     Running   0          5m1s
redis                              1/1     Running   0          5m1s
south-onlp-gkzdg                   1/1     Running   0          5m
south-tai-d56zd                    1/1     Running   0          4m40s
usonic-core-5978f477b8-fktz4       2/2     Running   0          2m41s
usonic-frr-74497cdcc9-596b4        4/4     Running   0          2m39s
usonic-lldpd-8fbdbc7bf-8vnfk       1/1     Running   0          3m12s
usonic-bcm-77d6d865df-mkvpb        1/1     Running   0          2m42s
usonic-port-68f796f997-hlcbm       1/1     Running   0          2m38s
usonic-teamd-c778f7f85-tt24x       2/2     Running   0          2m40s
usonic-neighbor-75d9c49667-gkj22   1/1     Running   0          2m42s
usonic-mgrd-67f996695b-sr497       4/4     Running   0          2m42s
south-sonic-cspbx                  1/1     Running   0          3m17s
svclb-north-netconf-dd4zw          1/1     Running   0          91s
svclb-north-snmp-tfkqw             1/1     Running   0          91s
svclb-north-gnmi-rmlsq             2/2     Running   0          91s
north-gnmi-lgd8g                   1/1     Running   0          91s
system-telemetry-n4pl2             1/1     Running   0          91s
xlate-oc-4225j                     1/1     Running   0          91s
north-notif-jpgrz                  1/1     Running   0          89s
north-netconf-7hjvx                1/1     Running   0          91s
north-snmp-gs86s                   2/2     Running   0          91s
```

Use `systemctl status <service>` to check the status of all the services 

- Mgmt services
  ```
  root@localhost:~# systemctl status gs-
  gs-mgmt-north.target              gs-north-netconf-yang.service     gs-south-gearbox-yang.service     gs-south-system-yang.service      gs-xlate-oc-yang.service
  gs-mgmt-south.target              gs-north-netconf.service          gs-south-gearbox.service          gs-south-system.service           gs-xlate-oc.service
  gs-mgmt-system.target             gs-north-notif.service            gs-south-onlp-yang.service        gs-south-tai-yang.service         gs-yang.service
  gs-mgmt-xlate.target              gs-north-snmp.service             gs-south-onlp.service             gs-south-tai.service              
  gs-mgmt.target                    gs-south-dpll-yang.service        gs-south-sonic-yang.service       gs-system-telemetry-yang.service  
  gs-north-gnmi.service             gs-south-dpll.service             gs-south-sonic.service            gs-system-telemetry.service
  root@localhost:~# systemctl status gs-south-sonic.service
  ● gs-south-sonic.service - Goldstone Management South SONiC daemon
     Loaded: loaded (/etc/systemd/system/gs-south-sonic.service; enabled; vendor preset: enabled)
     Active: active (exited) since Mon 2023-08-21 06:36:45 UTC; 1 weeks 2 days ago
   Process: 20970 ExecStartPre=/bin/sh -c while [ true ]; do ( kubectl get nodes | grep " Ready" ) && exit 0; sleep 1; done (code=exited, status=0/SUCCESS)
   Process: 20990 ExecStart=/usr/bin/gs-mgmt.py start south-sonic (code=exited, status=0/SUCCESS)
   Main PID: 20990 (code=exited, status=0/SUCCESS)

  Aug 21 06:35:46 localhost systemd[1]: Starting Goldstone Management South SONiC daemon...
  Aug 21 06:35:46 localhost sh[20970]: localhost   Ready    control-plane,master   45s   v1.22.2+k3s-
  Aug 21 06:35:48 localhost gs-mgmt.py[20990]: serviceaccount/south-sonic-svc created
  Aug 21 06:35:48 localhost gs-mgmt.py[20990]: clusterrole.rbac.authorization.k8s.io/usonic-manage created
  Aug 21 06:35:48 localhost gs-mgmt.py[20990]: clusterrolebinding.rbac.authorization.k8s.io/south-sonic-svc created
  Aug 21 06:35:48 localhost gs-mgmt.py[20990]: daemonset.apps/south-sonic created
  Aug 21 06:36:45 localhost gs-mgmt.py[20990]: pod/south-sonic-zbpbt condition met
  Aug 21 06:36:45 localhost systemd[1]: Started Goldstone Management South SONiC daemon.
  ```
- ONLP service
  ```
  root@localhost:~# systemctl status onlpd.service 
  ● onlpd.service - LSB: Start ONLP Platform Agent
     Loaded: loaded (/etc/init.d/onlpd; generated)
     Active: active (running) since Mon 2023-08-21 06:30:47 UTC; 1 weeks 2 days ago
       Docs: man:systemd-sysv-generator(8)
    Process: 630 ExecStart=/etc/init.d/onlpd start (code=exited, status=0/SUCCESS)
      Tasks: 3 (limit: 4915)
     Memory: 3.6M
     CGroup: /system.slice/onlpd.service
             ├─708 /bin/onlpd -M /var/run/onlpd.pid
             └─709 /bin/onlpd -M /var/run/onlpd.pid

  Aug 21 06:30:47 localhost onlpd[630]: Starting ONLP Platform Agent: onlpd.
  Aug 21 06:30:47 localhost systemd[1]: Started LSB: Start ONLP Platform Agent.
  Aug 21 06:30:48 localhost onlpd[709]: [onlp] Fan 1 has been inserted.
  Aug 21 06:30:48 localhost onlpd[709]: [onlp] Fan 2 has been inserted.
  Aug 21 06:30:48 localhost onlpd[709]: [onlp] Fan 3 has been inserted.
  Aug 21 06:30:48 localhost onlpd[709]: [onlp] Fan 4 has been inserted.
  Aug 21 06:30:48 localhost onlpd[709]: [onlp] PSU 1 has failed.
  Aug 21 06:30:48 localhost onlpd[709]: [onlp] PSU 1 has been inserted.
  Aug 21 06:30:48 localhost onlpd[709]: [onlp] PSU 1 has failed.
  Aug 21 06:30:48 localhost onlpd[709]: [onlp] PSU 2 has been inserted.
  ```
- uSonic service
  ```
  root@localhost:~# systemctl status usonic.service 
  ● usonic.service - uSONiC
     Loaded: loaded (/etc/systemd/system/usonic.service; enabled; vendor preset: enabled)
     Active: active (exited) since Mon 2023-08-21 06:35:46 UTC; 1 weeks 2 days ago
    Process: 636 ExecStartPre=/bin/sh -c while [ true ]; do ( kubectl get nodes | grep " Ready" ) && exit 0; sleep 1; done (code=exited, status=0/SUCCESS)
    Process: 16903 ExecStart=/usr/bin/usonic.sh start (code=exited, status=0/SUCCESS)
   Main PID: 16903 (code=exited, status=0/SUCCESS)

  Aug 21 06:35:45 localhost usonic.sh[16903]: + for pod in $(kubectl get pod -l app=usonic -o jsonpath='{.items[*].metadata.name}')
  Aug 21 06:35:45 localhost usonic.sh[16903]: + kubectl wait --for=condition=ready pod/usonic-lldpd-58d8565bf8-h6wgz --timeout 5m
  Aug 21 06:35:45 localhost usonic.sh[16903]: pod/usonic-lldpd-58d8565bf8-h6wgz condition met
  Aug 21 06:35:45 localhost usonic.sh[16903]: + for pod in $(kubectl get pod -l app=usonic -o jsonpath='{.items[*].metadata.name}')
  Aug 21 06:35:45 localhost usonic.sh[16903]: + kubectl wait --for=condition=ready pod/usonic-bcm-5c48dd5bf9-sl6ph --timeout 5m
  Aug 21 06:35:46 localhost usonic.sh[16903]: pod/usonic-bcm-5c48dd5bf9-sl6ph condition met
  Aug 21 06:35:46 localhost usonic.sh[16903]: + for pod in $(kubectl get pod -l app=usonic -o jsonpath='{.items[*].metadata.name}')
  Aug 21 06:35:46 localhost usonic.sh[16903]: + kubectl wait --for=condition=ready pod/usonic-core-76498db4d-69flp --timeout 5m
  Aug 21 06:35:46 localhost usonic.sh[16903]: pod/usonic-core-76498db4d-69flp condition met
  Aug 21 06:35:46 localhost systemd[1]: Started uSONiC.
  ```
- Tai service
  ```
  root@localhost:~# systemctl status tai.service 
  ● tai.service - TAI shell service
     Loaded: loaded (/etc/systemd/system/tai.service; enabled; vendor preset: enabled)
     Active: active (exited) since Mon 2023-08-21 06:36:46 UTC; 1 weeks 2 days ago
    Process: 642 ExecStartPre=/bin/sh -c while [ true ]; do ( kubectl get nodes | grep " Ready" ) && exit 0; sleep 1; done (code=exited, status=0/SUCCESS)
    Process: 16910 ExecStart=/usr/bin/tai.sh start (code=exited, status=0/SUCCESS)
   Main PID: 16910 (code=exited, status=0/SUCCESS)

  Aug 21 06:35:14 localhost tai.sh[16910]: clusterrolebinding.rbac.authorization.k8s.io/tai created
  Aug 21 06:35:14 localhost tai.sh[16910]: service/taish-server created
  Aug 21 06:35:14 localhost tai.sh[16910]: deployment.apps/tai created
  Aug 21 06:35:14 localhost tai.sh[16910]: + sleep 1
  Aug 21 06:35:15 localhost tai.sh[16910]: + kubectl wait --timeout=5m --for=condition=available deploy/tai
  Aug 21 06:36:46 localhost tai.sh[16910]: deployment.apps/tai condition met
  Aug 21 06:36:46 localhost tai.sh[16910]: ++ kubectl get pod -l app=tai -o 'jsonpath={.items[0].metadata.name}'
  Aug 21 06:36:46 localhost tai.sh[16910]: + kubectl wait --timeout=5m --for=condition=ready pod/tai-5868b8c5dc-7qgcv
  Aug 21 06:36:46 localhost tai.sh[16910]: pod/tai-5868b8c5dc-7qgcv condition met
  Aug 21 06:36:46 localhost systemd[1]: Started TAI shell service.
  ```

# Configuration Guide

### CLI
### Netconf
### SNMP
 
# CLI Reference Guide
gscli is the CLI(CommandLineInterface) for Goldstone NOS. 
Completion works like normal network device. Typing ? shows candidates for the input.
```
root@localhost:~# gscli 
> show version
goldstone-v0.8.0-17-g933ce93
>
```
### Table of Contents

* [AAA & TACACS+](#aaa--tacacs)
  * [AAA](#aaa)
    * [AAA show commands](#aaa-show-commands)
    * [AAA config commands](#aaa-config-commands)
  * [TACACS+](#tacacs)
    * [TACACS+ show commands](#tacacs-show-commands)
    * [TACACS+ config commands](#tacacs-config-commands)
* [ARP](#arp)
   * [ARP show commands](#tacacs-show-commands)
* [Interface](#interface)
  * [Interface show commands](#interface-show-commands)
  * [Interface config commands](#interface-config-commands)
* [Netconf](netconf)
* [Portchannels](#portchannels)
  * [Portchannel show commands](#portchannel-show-commands)
  * [Portchannel config commands](#portchannel-config-commands)
* [Ping](#ping)
* [Save, Reboot and clear](#save-reboot-clear)
  * [Save](#save)
  * [Reboot](#reboot)
  * [Clear](#clear)
* [Traceroute](traceroute)
* [Transponder](#transponder)
* [UFD](#ufd)
* [Vlan](#vlan)


# Appendix

## Handy build tips for developers
### Building goldstone-buildimage
1. Currently [OpenNetworkLinux](https://github.com/palcnetworks/OpenNetworkLinux) and [sonic-buildimage](https://github.com/sonic-net/sonic-buildimage) are built as submodules while building goldstone-buildimage and after the successful build, all the packages are put under 'REPO' directory. While debugging/developing a feature, the developer instead of building the whole image can only build the required packages using interative docker-debug shell.
    - `make docker-debug` command will start an interactive docker shell to build packages.
    - `onlpm --build --list-all` can be used to list all the packages that can be built.
    - `onlpm --build <package>:<arch>` to build a package for a specified architecture.
         eg : `gs-tai:arm64` , `onl-platform-build-x86-64-accton-as7716-24sc-r0:amd64`
2. <Add more>

### Building other NOS components
As explained in the system-architecture , Goldstone NOS constitutes of other components like uSonic, TAI, goldstone-management which are built as part of separate projects and then the built docker container images are uploaded to GitHub container registery [here](https://github.com/orgs/oopt-goldstone/packages). And these images are then packaged in to ONIE-installer image while building goldstone-builimage. 
If there a requirement to modify/add any functionality to these components, the following section will explain in brief how to build these components and load them on to DUT running goldstone-buildimage.
1. [Building Goldstone-mgmt](https://github.com/oopt-goldstone/goldstone-mgmt)
   ```
   $ git clone https://github.com/oopt-goldstone/goldstone-mgmt.git
   
   $ cd goldstone-mgmt
   
   $ git submodule update --init
   
   $ make all
   ```
   This will build all Goldstone management components as container images.
   ```
   $ docker images | grep oopt-goldstone/mgmt
   ghcr.io/oopt-goldstone/mgmt/south-ocnos        latest             838559aee13e   7 hours ago    317MB
   ghcr.io/oopt-goldstone/mgmt/south-onlp         latest             af8f0aa3aa82   7 hours ago    325MB
   ghcr.io/oopt-goldstone/mgmt/xlate-or           latest             1e5914cceffe   7 hours ago    311MB
   ghcr.io/oopt-goldstone/mgmt/host-packages      latest             8cdb10e5b754   7 hours ago    1.72GB
   ghcr.io/oopt-goldstone/mgmt/south-dpll         latest             f27b80481213   7 hours ago    413MB
   ghcr.io/oopt-goldstone/mgmt/south-gearbox      latest             44a5950b74a5   7 hours ago    413MB
   ghcr.io/oopt-goldstone/mgmt/north-gnmi         latest             5ffeb39a346a   7 hours ago    235MB
   ghcr.io/oopt-goldstone/mgmt/north-snmp         latest             a9cb097f1789   7 hours ago    216MB
   ghcr.io/oopt-goldstone/mgmt/north-netconf      latest             d8ad5c5a1c1e   7 hours ago    476MB
   ghcr.io/oopt-goldstone/mgmt/xlate-oc           latest             e9253419f31c   7 hours ago    215MB
   ghcr.io/oopt-goldstone/mgmt/south-sonic        latest             90345dbc3b13   7 hours ago    289MB
   ghcr.io/oopt-goldstone/mgmt/south-tai          latest             ac16572109e9   7 hours ago    233MB
   ghcr.io/oopt-goldstone/mgmt/system-telemetry   latest             6d22ae9f88b8   7 hours ago    215MB
   ghcr.io/oopt-goldstone/mgmt/north-notif        latest             d78d9585a406   7 hours ago    215MB
   ghcr.io/oopt-goldstone/mgmt/snmpd              latest             ac1d81abd6d2   7 hours ago    174MB
   ```
   Following example will show how to load the south-sonic docker image on to the DUT and same steps are valid for other mgmt images.
     - On the build VM : Save south-sonic image to a tar archive.
         - `docker save ghcr.io/oopt-goldstone/mgmt/south-sonic:latest > south-sonic-debug.tar`
     - On the DUT :
         - `ctr images import south-sonic-debug.tar`
         - update `south-sonic.yaml` yaml file with the new image name
         - restart the service `systemctl restart gs-mgmt`
      - Changes in the python source code files can be directly done on the DUT and then restart the service `systemctl restart gs-mgmt`.
           
2. [Building TAI](https://github.com/Telecominfraproject/oopt-tai-implementations/tree/master)
   
   Refer [here](https://github.com/Telecominfraproject/oopt-tai-implementations/tree/master) for detailed information.

3. [Building usonic](https://github.com/oopt-goldstone/usonic)
   Usonic builds all the container images required to run the SONiC components in Goldstone NOS.
   ```
   $ git clone https://github.com/oopt-goldstone/usonic.git
   
   #By default 202205 is checkedout, use git checkout to get other branches
   $ git checkout [branch_name]
   
   $ git submodule update --init --recursive sm/sonic-py-swsssdk sm/sonic-sairedis sm/sonic-swss sm/sonic-swss-common sm/sonic-utilities sm/sonic-frr sm/sonic-dbsyncd sm/sonic-platform-common

   $ git submodule update --init sm/sonic-buildimage

   $ make all
   ```
   This will build all usonic components as container images.
   ```
   ghcr.io/oopt-goldstone/usonic-debug            202205             49ec7896664c   2 hours ago     2.15GB
   ghcr.io/oopt-goldstone/usonic                  202205             657e0b10b74d   3 hours ago     1.28GB
   ghcr.io/oopt-goldstone/usonic-cli              202205             9c741170bb66   4 hours ago     590MB
   ```
   Following example will show how to load the usonic-debug docker image on to the DUT.
     - On the build VM : Save usonic-debug image to a tar archive.
         - `docker save ghcr.io/oopt-goldstone/usonic-debug:202205 > usonic-debug.tar`
     - On the DUT :
         - `ctr images import usonic-debug.tar`
         - update `usonic.yaml` yaml file with the new image name
         - restart the service `systemctl restart usonic`
           
## Sysrepo
sysrepo is the YANG based datastore which is used for the central management framework in Goldstone. DEtailed info [here](https://netopeer.liberouter.org/doc/sysrepo/master/html/)
sysrepo utility commands:
- `sysrepoctl` : YANG model management 
- `sysrepocfg` : YANG model manipulation
All the YANG modules can be found on the DUT @
```
root@localhost:~# ls /var/lib/sysrepo/yang/
goldstone-aaa@2020-10-13.yang			    goldstone-xconnect@2023-01-30.yang		ietf-netconf@2013-09-29.yang		       ietf-x509-cert-to-name@2014-12-10.yang
goldstone-component-connection@2021-11-01.yang	    iana-crypt-hash@2014-08-06.yang		ietf-network-instance@2019-01-21.yang	       ietf-yang-library@2019-01-04.yang
goldstone-interfaces@2020-10-13.yang		    ietf-crypto-types@2019-07-02.yang		ietf-origin@2018-02-14.yang		       ietf-yang-patch@2017-02-22.yang
goldstone-lldp@2023-06-02.yang			    ietf-datastores@2018-02-14.yang		ietf-restconf@2017-01-26.yang		       ietf-yang-push@2019-09-09.yang
goldstone-platform@2019-11-01.yang		    ietf-interfaces@2018-02-20.yang		ietf-ssh-common@2019-07-02.yang		       ietf-yang-schema-mount@2019-01-14.yang
goldstone-portchannel@2021-05-30.yang		    ietf-ip@2018-02-22.yang			ietf-ssh-server@2019-07-02.yang		       nc-notifications@2008-07-14.yang
goldstone-snmp@2023-04-20.yang			    ietf-keystore@2019-07-02.yang		ietf-subscribed-notifications@2019-09-09.yang  notifications@2008-07-14.yang
goldstone-sonic@2021-12-06.yang			    ietf-netconf-acm@2018-02-14.yang		ietf-tcp-client@2019-07-02.yang		       sysrepo-monitoring@2022-04-08.yang
goldstone-system@2020-11-23.yang		    ietf-netconf-monitoring@2010-10-04.yang	ietf-tcp-common@2019-07-02.yang		       sysrepo-plugind@2022-03-10.yang
goldstone-telemetry@2022-05-25.yang		    ietf-netconf-nmda@2019-01-07.yang		ietf-tcp-server@2019-07-02.yang		       sysrepo@2021-10-08.yang
goldstone-transponder@2019-11-01.yang		    ietf-netconf-notifications@2012-02-06.yang	ietf-tls-common@2019-07-02.yang		       yang@2022-06-16.yang
goldstone-uplink-failure-detection@2021-05-28.yang  ietf-netconf-server@2019-07-02.yang		ietf-tls-server@2019-07-02.yang
goldstone-vlan@2022-11-29.yang			    ietf-netconf-with-defaults@2011-06-01.yang	ietf-truststore@2019-07-02.yang
```
Get the list of all the YANG models installed on the DUT `sysrepoctl -l`
```
root@localhost:~# sysrepoctl -l
Sysrepo repository: /var/lib/sysrepo

Module Name                        | Revision   | Flags | Owner       | Startup Perms | Submodules | Features                                                                                
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
goldstone-aaa                      | 2020-10-13 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-component-connection     | 2021-11-01 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-interfaces               | 2020-10-13 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-lldp                     | 2023-06-02 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-platform                 | 2019-11-01 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-portchannel              | 2021-05-30 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-snmp                     | 2023-04-20 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-sonic                    | 2021-12-06 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-system                   | 2020-11-23 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-telemetry                | 2022-05-25 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-transponder              | 2019-11-01 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-uplink-failure-detection | 2021-05-28 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-vlan                     | 2022-11-29 | I     | root:gsmgmt | 664           |            |                                                                                         
goldstone-xconnect                 | 2023-01-30 | I     | root:gsmgmt | 664           |            |                                                                                         
iana-crypt-hash                    | 2014-08-06 | i     |             |               |            |                                                                                         
ietf-crypto-types                  | 2019-07-02 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-datastores                    | 2018-02-14 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-inet-types                    | 2013-07-15 | i     |             |               |            |                                                                                         
ietf-interfaces                    | 2018-02-20 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-ip                            | 2018-02-22 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-keystore                      | 2019-07-02 | I     | root:gsmgmt | 664           |            | keystore-supported                                                                      
ietf-netconf                       | 2013-09-29 | I     | root:gsmgmt | 664           |            | writable-running candidate confirmed-commit rollback-on-error validate startup url xpath
ietf-netconf-acm                   | 2018-02-14 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-netconf-monitoring            | 2010-10-04 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-netconf-nmda                  | 2019-01-07 | I     | root:gsmgmt | 664           |            | origin with-defaults                                                                    
ietf-netconf-notifications         | 2012-02-06 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-netconf-server                | 2019-07-02 | I     | root:gsmgmt | 664           |            | ssh-listen tls-listen ssh-call-home tls-call-home                                       
ietf-netconf-with-defaults         | 2011-06-01 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-network-instance              | 2019-01-21 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-origin                        | 2018-02-14 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-restconf                      | 2017-01-26 | i     |             |               |            |                                                                                         
ietf-ssh-common                    | 2019-07-02 | i     |             |               |            |                                                                                         
ietf-ssh-server                    | 2019-07-02 | I     | root:gsmgmt | 664           |            | local-client-auth-supported                                                             
ietf-subscribed-notifications      | 2019-09-09 | I     | root:gsmgmt | 664           |            | encode-xml replay subtree xpath                                                         
ietf-tcp-client                    | 2019-07-02 | i     |             |               |            |                                                                                         
ietf-tcp-common                    | 2019-07-02 | I     | root:gsmgmt | 664           |            | keepalives-supported                                                                    
ietf-tcp-server                    | 2019-07-02 | i     |             |               |            |                                                                                         
ietf-tls-common                    | 2019-07-02 | i     |             |               |            |                                                                                         
ietf-tls-server                    | 2019-07-02 | I     | root:gsmgmt | 664           |            | local-client-auth-supported                                                             
ietf-truststore                    | 2019-07-02 | I     | root:gsmgmt | 664           |            | truststore-supported x509-certificates                                                  
ietf-x509-cert-to-name             | 2014-12-10 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-yang-library                  | 2019-01-04 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-yang-metadata                 | 2016-08-05 | i     |             |               |            |                                                                                         
ietf-yang-patch                    | 2017-02-22 | i     |             |               |            |                                                                                         
ietf-yang-push                     | 2019-09-09 | I     | root:gsmgmt | 664           |            | on-change                                                                               
ietf-yang-schema-mount             | 2019-01-14 | I     | root:gsmgmt | 664           |            |                                                                                         
ietf-yang-types                    | 2013-07-15 | i     |             |               |            |                                                                                         
nc-notifications                   | 2008-07-14 | I     | root:gsmgmt | 664           |            |                                                                                         
notifications                      | 2008-07-14 | I     | root:gsmgmt | 664           |            |                                                                                         
sysrepo-monitoring                 | 2022-04-08 | I     | root:gsmgmt | 664           |            |                                                                                         
sysrepo-plugind                    | 2022-03-10 | I     | root:gsmgmt | 664           |            |                                                                                         
yang                               | 2022-06-16 | I     | root:gsmgmt | 664           |            |                                                                                         

Flags meaning: I - Installed/i - Imported; R - Replay support
```
Read/edit the sysrepo datastore using `sysrepocfg`
`sysrepocfg -X -m goldstone-interfaces --format json -d running`
`sysrepocfg -X -m goldstone-interfaces --format json -d operational`
