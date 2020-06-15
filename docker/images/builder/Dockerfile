FROM  opennetworklinux/builder10:1.2

# TODO Quick hack to make https work
# remove this after ONL builder has https support
# https://github.com/opennetworklinux/X1/pull/3/files#r201783677
RUN apt update && sudo apt install -qy apt-transport-https ed libelf-dev python-pip ca-certificates
RUN pip install j2cli
# Install Docker cli
RUN wget -O /tmp/docker.tgz https://get.docker.com/builds/Linux/x86_64/docker-17.05.0-ce.tgz && tar xf /tmp/docker.tgz -C /tmp && mv /tmp/docker/docker /usr/bin/ && rm -rf /tmp/*

RUN git clone https://github.com/AgentD/squashfs-tools-ng.git && cd squashfs-tools-ng && ./autogen.sh && ./configure && make && make install && ldconfig
