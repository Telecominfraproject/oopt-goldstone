import asyncio
import argparse

from grpclib.client import Channel

from bcmd import bcmd_pb2
from bcmd import bcmd_grpc

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("command", nargs="*")

    args = parser.parse_args()

    async def _main():
        async with Channel('127.0.0.1', 50051) as ch:
            bcm = bcmd_grpc.BCMDStub(ch)
            reply = await bcm.Exec(bcmd_pb2.ExecRequest(command=" ".join(args.command)))
            print(reply.response)

    asyncio.run(_main())


if __name__ == "__main__":
    main()
