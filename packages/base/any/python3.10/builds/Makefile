IMAGE_NAME ?= python:3.10-slim-buster

all: python pip python3.10 libpython3.10.so.1.0

python:
	docker run --rm -v `pwd`:/data -w /data --platform linux/$(ARCH) $(IMAGE_NAME) cp /usr/local/bin/$@ $@

pip:
	docker run --rm -v `pwd`:/data -w /data --platform linux/$(ARCH) $(IMAGE_NAME) cp /usr/local/bin/$@ $@

python3.10:
	docker run --rm -v `pwd`:/data -w /data --platform linux/$(ARCH) $(IMAGE_NAME) cp -r /usr/local/lib/$@ $@

libpython3.10.so.1.0:
	docker run --rm -v `pwd`:/data -w /data --platform linux/$(ARCH) $(IMAGE_NAME) cp -r /usr/local/lib/$@ $@
