#!/usr/bin/env python3
"""Offer to update the embedded payload from MiGoomba's latest stable release."""
from __future__ import annotations

import hashlib
import json
import re
import sys
import tempfile
from pathlib import Path
from urllib.parse import quote
from urllib.request import Request, urlopen

from embed_migoomba import METADATA, ROOT, render_header, validate_payload

RELEASE_API = "https://api.github.com/repos/michyprima/migoomba/releases/latest"
COMMIT_API = "https://api.github.com/repos/michyprima/migoomba/commits/"
DOWNLOAD_PREFIX = "https://github.com/michyprima/migoomba/releases/download/"
MAX_PAYLOAD_SIZE = 0x20000


def fetch_json(url: str) -> dict:
    request = Request(
        url,
        headers={
            "Accept": "application/vnd.github+json",
            "User-Agent": "omega-de-ds-style-kernel",
        },
    )
    with urlopen(request, timeout=10) as response:
        result = json.loads(response.read())
    if not isinstance(result, dict):
        raise ValueError("GitHub returned an unexpected JSON response")
    return result


def numeric_version(version: str) -> tuple[int, ...]:
    return tuple(int(part) for part in re.findall(r"\d+", version))


def find_ezode_asset(release: dict, tag: str) -> dict | None:
    assets = release.get("assets")
    if not isinstance(assets, list):
        return None
    expected_name = f"migoomba_{tag}_ezode.gba"
    return next(
        (
            asset
            for asset in assets
            if isinstance(asset, dict) and asset.get("name") == expected_name
        ),
        None,
    )


def download_asset(asset: dict) -> bytes:
    url = asset.get("browser_download_url")
    if not isinstance(url, str) or not url.startswith(DOWNLOAD_PREFIX):
        raise ValueError("Release asset has an unexpected download URL")
    request = Request(url, headers={"User-Agent": "omega-de-ds-style-kernel"})
    with urlopen(request, timeout=30) as response:
        payload = response.read(MAX_PAYLOAD_SIZE + 1)
    if len(payload) > MAX_PAYLOAD_SIZE:
        raise ValueError("Release asset exceeds the 128 KiB loader limit")
    if asset.get("size") != len(payload):
        raise ValueError("Downloaded size does not match the release asset metadata")
    expected_digest = asset.get("digest")
    if expected_digest is not None:
        if not isinstance(expected_digest, str) or not expected_digest.startswith("sha256:"):
            raise ValueError("Release asset has an unsupported digest format")
        if hashlib.sha256(payload).hexdigest() != expected_digest[7:].lower():
            raise ValueError("Downloaded asset does not match GitHub's SHA-256 digest")
    return payload


def write_bytes_atomically(path: Path, content: bytes) -> None:
    temporary_path = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="wb",
            dir=path.parent,
            prefix=f".{path.name}.",
            suffix=".tmp",
            delete=False,
        ) as temporary:
            temporary_path = Path(temporary.name)
            temporary.write(content)
        temporary_path.replace(path)
    finally:
        if temporary_path is not None and temporary_path.exists():
            temporary_path.unlink()


def replace_files_atomically(replacements: list[tuple[Path, bytes]]) -> None:
    originals = {path: path.read_bytes() for path, _ in replacements}
    replaced = []
    try:
        for path, content in replacements:
            write_bytes_atomically(path, content)
            replaced.append(path)
    except OSError as update_error:
        try:
            for path in reversed(replaced):
                write_bytes_atomically(path, originals[path])
        except OSError as rollback_error:
            raise OSError(
                f"MiGoomba update failed ({update_error}); rollback failed ({rollback_error})"
            ) from rollback_error
        raise


def install_release(release: dict, asset: dict, metadata: dict) -> None:
    tag = release.get("tag_name")
    name = asset.get("name")
    if not isinstance(tag, str) or not isinstance(name, str):
        raise ValueError("Release is missing its tag or asset name")

    payload = download_asset(asset)
    digest = hashlib.sha256(payload).hexdigest()
    commit = fetch_json(COMMIT_API + quote(tag, safe="")).get("sha")
    if not isinstance(commit, str) or not re.fullmatch(r"[0-9a-fA-F]{40}", commit):
        raise ValueError("Could not resolve the release tag to a commit")

    updated_metadata = dict(metadata)
    updated_metadata.update(
        version=tag,
        upstream_commit=commit.lower(),
        upstream_binary=name,
        bytes=len(payload),
        sha256=digest,
    )
    validate_payload(payload, updated_metadata)
    header = ROOT / updated_metadata["embedded_header"]
    generated_header = render_header(payload, updated_metadata)
    generated_metadata = json.dumps(updated_metadata, indent=2) + "\n"

    replace_files_atomically(
        [
            (header, generated_header.encode("ascii")),
            (METADATA, generated_metadata.encode("utf-8")),
        ]
    )
    print(f"Updated MiGoomba to {tag} ({len(payload)} bytes, SHA-256 {digest}).")


def main() -> int:
    metadata = json.loads(METADATA.read_text(encoding="utf-8"))
    try:
        release = fetch_json(RELEASE_API)
    except (OSError, TimeoutError, json.JSONDecodeError, ValueError) as error:
        print(
            f"WARNING: Could not check MiGoomba releases ({error}); "
            f"continuing with embedded {metadata['version']}.",
            file=sys.stderr,
        )
        return 0

    tag = release.get("tag_name")
    if not isinstance(tag, str):
        print("WARNING: Latest MiGoomba release has no tag; keeping current payload.")
        return 0
    asset = find_ezode_asset(release, tag)
    if asset is None:
        print(f"WARNING: Release {tag} has no matching ezode binary; keeping current payload.")
        return 0

    current_version = metadata["version"]
    current_digest = metadata["sha256"]
    release_digest = asset.get("digest")
    if tag == current_version and (
        release_digest is None
        or (
            isinstance(release_digest, str)
            and release_digest.lower() == f"sha256:{current_digest}".lower()
        )
    ):
        print(f"MiGoomba {current_version} is the latest release.")
        return 0

    if tag != current_version:
        current_numbers = numeric_version(current_version)
        release_numbers = numeric_version(tag)
        if current_numbers and release_numbers and release_numbers <= current_numbers:
            print(
                f"Latest stable release is {tag}; embedded version {current_version} "
                "is not older. No update offered."
            )
            return 0

    print(f"MiGoomba {tag} is available; embedded version is {current_version}.")
    print(f"Release asset: {asset.get('name')} ({asset.get('size')} bytes).")
    try:
        answer = input("Download and embed this release? [y/N]: ").strip().lower()
    except EOFError:
        print("No interactive console; keeping the current MiGoomba payload.")
        return 0
    if answer not in ("y", "yes"):
        print("Keeping the current MiGoomba payload.")
        return 0

    try:
        install_release(release, asset, metadata)
    except (OSError, TimeoutError, json.JSONDecodeError, ValueError) as error:
        raise SystemExit(f"MiGoomba update failed after approval: {error}") from error
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
