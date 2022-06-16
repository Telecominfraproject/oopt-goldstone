Goldstone NOS
---

Goldstone NOS is a modular, container-based open source network OS for [TIP OOPT](https://telecominfraproject.com/oopt/) networking hardware

<img width="900" alt="Architecture" src="https://user-images.githubusercontent.com/5915117/174015089-7b4d7c71-e07d-4468-8a94-a9f19368516d.png">

### Supported Hardware

- Wistron WTP-01-02-00 (Galileo 1)
- Wistron WTP-01-C1-00 (Galileo Flex T)
- Edgecore AS7716-24SC/XC (Cassini)

### Getting Started

You can download pre-built Goldstone ONIE installer from the [release](https://github.com/oopt-goldstone/goldstone-buildimage/releases) page.

### How to build

#### Prerequisite

- Git
- make
- Docker ( version >= 18.09, enable [buildkit](https://docs.docker.com/develop/develop-images/build_enhancements/) )
- Enable an execution of different multi-architecture containers by QEMU and binfmt_misc
    - You can do this by running `docker run --rm --privileged multiarch/qemu-user-static --reset -p`
    - See https://github.com/multiarch/qemu-user-static for details
- Python2 (!!)
    - Goldstone uses the ONL build system to build the NOS. ONL build system only supports Python2.

```
$ git clone https://github.com/oopt-goldstone/goldstone-buildimage.git
$ cd goldstone-buildimage
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
RELEASE/buster/arm64/goldstone-v0.5.0_ONL-OS10_2022-06-13.0547-02419c5_ARM64.swi.md5sum
RELEASE/buster/arm64/goldstone-v0.5.0_2022-06-13.0547-02419c5_ARM64_INSTALLER
RELEASE/buster/arm64/goldstone-v0.5.0_ONL-OS10_2022-06-13.0547-02419c5_ARM64.swi
RELEASE/buster/arm64/goldstone-v0.5.0_2022-06-13.0547-02419c5_ARM64_INSTALLER.md5sum
RELEASE/buster/amd64
RELEASE/buster/amd64/goldstone-v0.5.0_ONL-OS10_2022-06-13.0547-02419c5_AMD64.swi
RELEASE/buster/amd64/goldstone-v0.5.0_ONL-OS10_2022-06-13.0547-02419c5_AMD64.swi.md5sum
RELEASE/buster/amd64/goldstone-v0.5.0_2022-06-13.0547-02419c5_AMD64_INSTALLER
RELEASE/buster/amd64/goldstone-v0.5.0_2022-06-13.0547-02419c5_AMD64_INSTALLER.md5sum
```
