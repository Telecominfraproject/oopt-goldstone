#!/usr/bin/env python

import subprocess
import sys
import time
import argparse
from pathlib import Path

APPS = [
    "north-notif",
    "north-netconf",
    "north-snmp",
    "north-gnmi",
    "south-sonic",
    "south-gearbox",
    "south-dpll",
    "south-tai",
    "south-onlp",
    "xlate-oc",
    "system-telemetry",
]


def stop(app, manifest_dir, kustomize):
    pod = subprocess.run(
        f"kubectl get pod -l app={app} -o jsonpath='{{.items[0].metadata.name}}' 2>/dev/null || echo 'dummy'",
        capture_output=True,
        check=True,
        shell=True,
    )
    pod = pod.stdout.decode()
    flag = "-k" if kustomize else "-f"
    subprocess.run(
        f"kubectl delete --ignore-not-found {flag} {manifest_dir} -l gs-mgmt={app}",
        check=True,
        shell=True,
    )
    subprocess.run(
        f"kubectl delete --ignore-not-found pods/{pod}", check=True, shell=True
    )


def start(app, manifest_dir, kustomize):
    flag = "-k" if kustomize else "-f"
    subprocess.run(
        f"kubectl apply {flag} {manifest_dir} -l gs-mgmt={app}",
        shell=True,
        check=True,
    )

    for _ in range(5):
        proc = subprocess.run(
            f"kubectl get pod -l app={app} -o jsonpath='{{.items[0].metadata.name}}'",
            capture_output=True,
            shell=True,
        )
        if proc.returncode == 0:
            pod = proc.stdout.decode()
            break
    else:
        print("timeout.")
        stop(app)
        sys.exit(1)

    subprocess.run(
        f"kubectl wait --timeout=5m --for=condition=ready pod/{pod}",
        shell=True,
        check=True,
    )


def usage():
    return f"usage: {sys.argv[0]} {{start|stop}} <app>"


def main(action, app, manifest_dir):
    if not Path(manifest_dir).exists():
        print(f"{manifest_dir} not found")
        sys.exit(1)

    kustomize = len(list(Path(manifest_dir).glob("kustomization.yaml"))) > 0

    if action == "start":
        start(app, manifest_dir, kustomize)
    elif action == "stop":
        stop(app, manifest_dir, kustomize)
    else:
        print(usage())
        sys.exit(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Goldstone Management Launcher")
    parser.add_argument("action", choices=["stop", "start"])
    parser.add_argument("app", choices=APPS)
    parser.add_argument("--manifest-dir", default="/var/lib/goldstone/k8s")

    args = parser.parse_args()
    main(args.action, args.app, args.manifest_dir)
