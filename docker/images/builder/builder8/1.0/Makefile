VERSION=1.0
USER=opennetworklinux
REPO=x1-builder8

build: check_version
	docker build -t $(USER)/$(REPO):$(VERSION) .
	docker tag $(USER)/$(REPO):$(VERSION) $(USER)/$(REPO):latest

#
# Todo: Query remote repository to see if the request version already exists to avoid accidental overwrites
# when a new image is built but the VERSION variable is not updated.
#
check_version:

push:
	docker push $(USER)/$(REPO):$(VERSION)
