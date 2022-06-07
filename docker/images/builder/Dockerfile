FROM alpine:latest AS buildx
RUN apk add --no-cache curl jq
RUN curl -sS https://api.github.com/repos/docker/buildx/releases/latest | \
    jq -r '.assets[].browser_download_url' | grep linux-amd64 | xargs curl -sSL > /docker-buildx
RUN chmod a+x /docker-buildx

FROM  opennetworklinux/builder10:1.2

RUN apt update --allow-releaseinfo-change && DEBIAN_FRONTEND=noninteractive apt install -qy apt-transport-https ed libelf-dev python-pip ca-certificates apt-cacher-ng curl
# Install Docker cli
COPY --from=docker:20.10 /usr/local/bin/docker /usr/local/bin/
COPY --from=buildx /docker-buildx /usr/local/lib/docker/cli-plugins/

COPY --from=python:3-buster-amd64 /usr/local/bin/python3 /usr/local/bin/python3
COPY --from=python:3-buster-amd64 /usr/local/lib/python3.10 /usr/local/lib/python3.10
COPY --from=python:3-buster-amd64 /usr/local/lib/libpython3.10.so.1.0 /usr/local/lib/libpython3.10.so.1.0
COPY --from=python:3-buster-amd64 /usr/local/include/python3.10 /usr/local/include/python3.10

RUN ldconfig

RUN git clone https://github.com/AgentD/squashfs-tools-ng.git && cd squashfs-tools-ng && ./autogen.sh && ./configure && make && make install && ldconfig

RUN cd /usr/bin/ && curl -s "https://raw.githubusercontent.com/kubernetes-sigs/kustomize/master/hack/install_kustomize.sh"  | bash

RUN curl -LO https://github.com/oras-project/oras/releases/download/v0.12.0/oras_0.12.0_linux_amd64.tar.gz && mkdir -p oras-install/ && tar -zxf oras_0.12.0_*.tar.gz -C oras-install/ && mv oras-install/oras /usr/local/bin/ && rm -rf oras_0.12.0_*.tar.gz oras-install/

