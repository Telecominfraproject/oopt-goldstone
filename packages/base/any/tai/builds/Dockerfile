# syntax=docker/dockerfile:experimental
ARG BASE

FROM ${BASE}

ARG TARGETARCH

RUN --mount=type=cache,target=/var/cache/apt --mount=type=cache,target=/var/lib/apt,sharing=private \
    if [ $TARGETARCH = amd64 ]; then apt update && DEBIAN_FRONTEND=noninteractive apt install -qy --no-install-recommends libi2c0; fi
