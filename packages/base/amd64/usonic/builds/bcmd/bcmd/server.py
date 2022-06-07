import logging
import subprocess
import argparse
from parse import *
from bcmd import bcmd_pb2
from bcmd import bcmd_grpc
import asyncio

from grpclib.utils import graceful_exit
from grpclib.server import Server

logger = logging.getLogger(__name__)


def bcmcmd(cmds):
    logger.debug(f"cmds: {cmds}")
    output = subprocess.run(["bcmcmd", cmds], capture_output=True)
    output = "\n".join(line for line in output.stdout.decode().split("\r\n"))
    logger.debug(f"output: {output}")
    return output


class BCMd(bcmd_grpc.BCMDBase):
    async def Exec(self, stream):
        request = await stream.recv_message()
        response = bcmcmd(request.command)
        await stream.send_message(bcmd_pb2.ExecResponse(response=response))


def get_temp():
    output = bcmcmd("show temp")
    data = []
    for line in output.split("\n"):
        result = parse("temperature monitor {:d}: current= {:f}, peak= {:f}", line)
        if result:
            data.append((result[1], result[2]))

    return data


async def temp_loop(output_dir, interval):
    while True:
        d = get_temp()
        for i, v in enumerate(d):
            with open(f"{output_dir}/temp{i+1}_current", "w") as f:
                f.write(f"{int(v[0] * 1000)}\n")

            with open(f"{output_dir}/temp{i+1}_peak", "w") as f:
                f.write(f"{int(v[1] * 1000)}\n")

        with open(f"{output_dir}/temp_avg", "w") as f:
            v = sum(v[0] * 1000 for v in d) // len(d)
            f.write(f"{int(v)}\n")

        with open(f"{output_dir}/temp_max_peak", "w") as f:
            v = max(v[1] * 1000 for v in d)
            f.write(f"{int(v)}\n")

        await asyncio.sleep(interval)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--init-soc")
    parser.add_argument("--temp-capture-interval", default=10)
    parser.add_argument("--temp-output-dir", default="/run/bcm")
    parser.add_argument("-v", "--verbose", action="store_true")

    args = parser.parse_args()
    output_dir = args.temp_output_dir
    interval = args.temp_capture_interval

    fmt = "%(levelname)s %(module)s %(funcName)s l.%(lineno)d | %(message)s"
    if args.verbose:
        logging.basicConfig(level=logging.DEBUG, format=fmt)
        for noisy in ["parse", "hpack"]:
            l = logging.getLogger(noisy)
            l.setLevel(logging.INFO)

    else:
        logging.basicConfig(level=logging.INFO, format=fmt)

    if args.init_soc:
        print(bcmcmd(args.init_soc))

    async def _main():
        server = Server([BCMd()])
        with graceful_exit([server]):
            await server.start("0.0.0.0", "50051")
            tasks = [
                asyncio.create_task(server.wait_closed()),
                asyncio.create_task(temp_loop(output_dir, interval)),
            ]
            done, pending = await asyncio.wait(
                tasks, return_when=asyncio.FIRST_COMPLETED
            )
            logger.debug(f"done: {done}, pending: {pending}")
            for task in done:
                e = task.exception()
                if e:
                    raise e

    asyncio.run(_main())


if __name__ == "__main__":
    main()
