#!/usr/bin/env python3
import os
import shutil
import subprocess
import sys
import time


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


def wait_for_rp2_mount(timeout_s=25.0, poll_s=0.25):
    start = time.time()
    while (time.time() - start) < timeout_s:
        mountpoint = find_rp2_mountpoint()
        if mountpoint:
            return mountpoint
        time.sleep(poll_s)
    return None


def find_picotool_exe():
    # 1) User PATH
    path_exe = shutil.which("picotool")
    if path_exe:
        return path_exe

    # 2) Common PlatformIO package location
    home = os.path.expanduser("~")
    candidates = [
        os.path.join(home, ".platformio", "packages", "tool-picotool", "picotool"),
        os.path.join(home, ".platformio", "packages", "tool-picotool-rp2040earlephilhower", "picotool"),
    ]
    for candidate in candidates:
        if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
            return candidate

    return None


def try_picotool_upload(uf2_path):
    exe = find_picotool_exe()
    if not exe:
        print("[upload] picotool not found for fallback upload")
        return False

    print(f"[upload] Trying picotool fallback: {exe}")
    try:
        # -x: reboot after load
        result = subprocess.run([exe, "load", "-x", uf2_path], check=False)
        return result.returncode == 0
    except Exception as ex:
        print(f"[upload] picotool fallback failed: {ex}")
        return False


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
        print("[upload] RPI-RP2 mount not found yet.")
        print("[upload] Put Pico in BOOTSEL mode now... waiting up to 25s")
        mountpoint = wait_for_rp2_mount(timeout_s=25.0, poll_s=0.25)

    if not mountpoint:
        print("[upload] RPI-RP2 mount not found.")
        print("[upload] Trying picotool fallback upload...")
        if try_picotool_upload(uf2_path):
            print("[upload] picotool upload complete")
            return 0
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
