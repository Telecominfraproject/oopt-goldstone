Goldstone NOS
---

Goldstone NOS is an open source network OS for [TIP OOPT](https://telecominfraproject.com/oopt/) networking hardware

### Supported Hardware

- Wistron WTP-01-02-00 (Galileo 1)
- Wistron WTP-01-C1-00 (Galileo FlexT)
- Edgecore AS7716-24SC/XC (Cassini)

### How to build

#### Prerequisite

- Git
- Docker ( version >= 18.09, enable [buildkit](https://docs.docker.com/develop/develop-images/build_enhancements/) )
- Python2
- make

```
$ docker run --rm --privileged multiarch/qemu-user-static --reset -p yes # https://github.com/multiarch/qemu-user-static
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
RELEASE/buster/arm64/goldstone-ea520b9_ONL-OS10_2022-06-08.2311-ea520b9_ARM64.swi.md5sum
RELEASE/buster/arm64/goldstone-ea520b9_ONL-OS_2022-06-08.2311-ea520b9_ARM64_INSTALLER
RELEASE/buster/arm64/goldstone-ea520b9_ONL-OS_2022-06-08.2311-ea520b9_ARM64_INSTALLER.md5sum
RELEASE/buster/arm64/goldstone-ea520b9_ONL-OS10_2022-06-08.2311-ea520b9_ARM64.swi
RELEASE/buster/amd64
RELEASE/buster/amd64/goldstone-ea520b9_ONL-OS10_2022-06-08.2311-ea520b9_AMD64.swi.md5sum
RELEASE/buster/amd64/goldstone-ea520b9_ONL-OS_2022-06-08.2311-ea520b9_AMD64_INSTALLER
RELEASE/buster/amd64/goldstone-ea520b9_ONL-OS_2022-06-08.2311-ea520b9_AMD64_INSTALLER.md5sum
RELEASE/buster/amd64/goldstone-ea520b9_ONL-OS10_2022-06-08.2311-ea520b9_AMD64.swi
```
