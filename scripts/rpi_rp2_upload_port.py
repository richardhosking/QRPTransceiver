Import("env")


def _find_rpi_rp2_mountpoint():
    label = "RPI-RP2"

    # Prefer actual mounted filesystems from /proc/mounts
    try:
        with open("/proc/mounts", "r", encoding="utf-8") as f:
            for line in f:
                parts = line.split()
                if len(parts) < 2:
                    continue
                mountpoint = parts[1]
                if mountpoint.endswith("/" + label) or mountpoint == "/" + label:
                    return mountpoint
    except Exception:
        pass

    # Common automount locations
    user = env.get("ENV", {}).get("USER", "")
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

    for candidate in candidates:
        if candidate and env.Dir(candidate).exists():
            return candidate

    return None


def _set_upload_port(*_args, **_kwargs):
    mountpoint = _find_rpi_rp2_mountpoint()
    if not mountpoint:
        print("\n[upload] RPI-RP2 mount not found.")
        print("[upload] Put Pico in BOOTSEL mode, wait for disk mount, then upload again.\n")
        env.Exit(1)

    env.Replace(UPLOAD_PORT=mountpoint)
    print(f"[upload] Using RP2 mount: {mountpoint}")


env.AddPreAction("upload", _set_upload_port)
