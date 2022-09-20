ARCH ?= amd64

DOCKER_IMAGE ?= ansible-galileo
DOCKER_CMD ?= bash

PLAYBOOK ?= main.yaml

PASSWORD ?= password

PING_TARGET ?= all

image:
	DOCKER_BUILDKIT=1 docker build --platform linux/$(ARCH) -t $(DOCKER_IMAGE) .

cmd:
	docker run $(DOCKER_RUN_OPTION) -v `pwd`:/data -w /data $(DOCKER_IMAGE) $(DOCKER_CMD)

bash:
	DOCKER_RUN_OPTION='-it' $(MAKE) cmd

ping: .ssh/id_rsa
	ansible -i ./hosts.yaml -m ping $(PING_TARGET)

play: .ssh/id_rsa
	ansible-playbook -i ./hosts.yaml $(PLAYBOOK) --extra-vars "ansible_become_pass=$(PASSWORD)"

.ssh:
	mkdir -m 700 .ssh

.ssh/id_rsa: .ssh
	ssh-keygen -t rsa -N "" -f .ssh/id_rsa
