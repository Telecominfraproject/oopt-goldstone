# syntax=docker/dockerfile:1.3

FROM python:3.10-slim AS builder

RUN pip install --upgrade pip
RUN pip install wheel grpcio-tools grpclib
RUN --mount=type=bind,target=/src,rw cd /src && python -m grpc_tools.protoc -Iproto --python_out=. --python_grpc_out=. ./proto/bcmd/bcmd.proto && mkdir /dist && pip wheel . -w /dist && find /dist

FROM python:3.10-slim

ADD https://sonicstorage.blob.core.windows.net/packages/20190307/bcmcmd?sv=2015-04-05&sr=b&sig=sUdbU7oVbh5exbXXHVL5TDFBTWDDBASHeJ8Cp0B0TIc%3D&se=2038-05-06T22%3A34%3A19Z&sp=r /usr/bin/bcmcmd
RUN chmod +x /usr/bin/bcmcmd
RUN --mount=type=bind,from=builder,source=/dist,target=/dist pip install /dist/*.whl
