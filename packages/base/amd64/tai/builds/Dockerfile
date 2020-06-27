# syntax=docker/dockerfile:experimental
ARG BASE

FROM ${BASE}

RUN --mount=type=cache,target=/var/cache/apt --mount=type=cache,target=/var/lib/apt \
apt update && DEBIAN_FRONTEND=noninteractive apt install -qy --no-install-recommends libi2c0
