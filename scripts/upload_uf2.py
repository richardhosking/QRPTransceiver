#!/usr/bin/env python3
import os
import shutil
import sys


def find_rp2_mountpoint():
    label = "RPI-RP2"

    # Best source: currently mounted filesystems
    try:
        with open("/proc/mounts", "r", encoding="utf-8") as f:
            for line in f:
                parts = line.split()
                if len(parts) >= 2:
                    mountpoint = parts[1]
                    if mountpoint.endswith("/" + label) or mountpoint == "/" + label:
                        return mountpoint
    except Exception:
        pass

    user = os.environ.get("USER", "")
    candidates = []
    if user:
        candidates.extend([
            f"/media/{user}/{label}",
            f"/run/media/{user}/{label}",
        ])
    candidates.extend([
        f"/media/{label}",
        f"/mnt/{label}",
    ])

    for p in candidates:
        if os.path.isdir(p):
            return p

    return None


def main():
    if len(sys.argv) < 2:
        print("Usage: upload_uf2.py <firmware.uf2>")
        return 2

    uf2_path = sys.argv[1]
    if not os.path.isfile(uf2_path):
        print(f"[upload] UF2 not found: {uf2_path}")
        return 2

    mountpoint = find_rp2_mountpoint()
    if not mountpoint:
        print("[upload] RPI-RP2 mount not found.")
        print("[upload] Put Pico in BOOTSEL mode, wait for disk mount, then retry.")
        return 1

    dst = os.path.join(mountpoint, os.path.basename(uf2_path))
    print(f"[upload] Copying {uf2_path} -> {dst}")
    shutil.copy2(uf2_path, dst)

    # Flush writes so copy is complete before process exits
    try:
        with open(dst, "rb") as f:
            os.fsync(f.fileno())
    except Exception:
        pass

    print("[upload] UF2 copy complete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
