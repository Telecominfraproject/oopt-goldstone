# syntax=docker/dockerfile:experimental
ARG BASE

FROM ${BASE}

ARG TARGETARCH

RUN if [ $TARGETARCH = amd64 ]; then apt update && DEBIAN_FRONTEND=noninteractive apt install -qy --no-install-recommends libi2c0; fi
