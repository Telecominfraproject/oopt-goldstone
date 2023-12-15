# syntax=docker/dockerfile:1.4
FROM opennetworklinux/builder10:1.2 AS onlp

SHELL ["/bin/bash", "-c"]
RUN --mount=type=bind,from=sm_onl_dir,source=./,target=/root/sm/ONL,rw \
    --mount=type=bind,from=git_modules_sm_onl_dir,source=./,target=/root/.git/modules/sm/ONL,rw \
    cd /root/sm/ONL && . ./setup.env && onlpm --rebuild-pkg-cache && mkdir -p /usr/share/onlp && \
    onlpm --build onlp:amd64 onlp-dev:amd64 onlp-py3:amd64 onlp-x86-64-kvm-x86-64-r0:amd64 && \
    cp -r REPO/buster/packages/binary-amd64/* /usr/share/onlp && ls /usr/share/onlp

FROM debian:bullseye

RUN --mount=type=cache,target=/var/cache/apt --mount=type=cache,target=/var/lib/apt \
    apt update && apt install -qy make g++ graphviz autotools-dev autoconf doxygen libnl-3-dev libnl-genl-3-dev \
    libnl-route-3-dev libnl-nf-3-dev libhiredis-dev perl libxml-simple-perl aspell swig libgtest-dev dh-exec debhelper \
    libtool pkg-config python3-all libpython3-dev quilt patchelf libboost-dev python3-lxml libxslt-dev python3-dev \
    python3-pip libpython2.7 libi2c0

RUN pip install --upgrade pip
RUN pip install pytest pytest-cov pytest-runner pathlib pyyaml==5.4.1

RUN --mount=type=bind,from=onlp,source=/usr/share/onlp,target=/src ls /src/*.deb | awk '$0 !~ /python/ && $0 !~ /-dbg_/ && $0 !~ /-dev_/ { print $0 }' | xargs dpkg -i

RUN ldconfig

# install swss-common
RUN --mount=type=cache,target=/var/cache/apt --mount=type=cache,target=/var/lib/apt \
    apt update && apt install -qy python-all libpython2-dev google-mock libgmock-dev \
    libzmq3-dev uuid-dev libyang-cpp-dev libboost-serialization-dev
RUN --mount=type=bind,from=sonic_buildimage_xcvrd_dir,source=src/sonic-swss-common,target=/root/src/sonic-swss-common,rw \
    --mount=type=tmpfs,target=/root/.pc,rw \
    --mount=type=bind,source=swss-common,target=/root/swss-common \
    cd /root && make -C /root/swss-common

# install sonic-py-common, sonic-yang-models, sonic-yang-mgmt
RUN pip install redis-dump-load
RUN --mount=type=bind,from=sonic_buildimage_xcvrd_dir,source=src/sonic-py-common,target=/root,rw \
    cd /root && python3 setup.py install

RUN pip install jinja2==2.11.2 zipp==1.2.0 importlib-resources==3.3.1 markupsafe==2.0.1 yang
RUN --mount=type=bind,from=sonic_buildimage_xcvrd_dir,source=src/sonic-yang-models,target=/root,rw \
    cd /root && python3 setup.py install
RUN --mount=type=bind,from=sonic_buildimage_xcvrd_dir,source=src/sonic-yang-mgmt,target=/root,rw \
    cd /root && python3 setup.py install

# install sonic-cfggen and sonic-platform
RUN --mount=type=bind,from=sonic_buildimage_xcvrd_dir,source=src/sonic-config-engine,target=/root/src/sonic-config-engine,rw \
    --mount=type=bind,from=sonic_buildimage_xcvrd_dir,source=device,target=/root/device,rw \
    --mount=type=bind,from=sonic_buildimage_xcvrd_dir,source=platform,target=/root/platform,rw \
    --mount=type=bind,source=patches/sonic-cfggen-and-sonic-platform,target=/root/patches \
    cd /root && quilt upgrade && quilt push -a && \
    cd /root/src/sonic-config-engine && python3 setup.py install && \
    cd /root/platform/broadcom/sonic-platform-modules-accton/as7946-30xb && python3 sonic_platform_setup.py install

# install sonic-platform-common
RUN --mount=type=bind,from=sonic_buildimage_xcvrd_dir,source=src/sonic-platform-common,target=/root,rw \
    --mount=type=bind,source=patches/sonic-platform-common,target=/root/patches \
    cd /root && quilt upgrade && quilt push -a && \
    python3 setup.py install

# install xcvrd
RUN --mount=type=bind,from=sonic_buildimage_xcvrd_dir,source=src/sonic-platform-daemons,target=/root,rw \
    --mount=type=bind,source=patches/sonic-platform-daemons,target=/root/patches \
    cd /root && quilt upgrade && quilt push -a && \
    cd /root/sonic-xcvrd && python3 setup.py install
# define this env to display xcvrd log with "kubectl logs"
ENV PYTHONUNBUFFERED=1

COPY swssconfig_xcvrd.py /usr/bin/swssconfig_xcvrd.py

RUN cd /root && rm -r .cache .pc src
