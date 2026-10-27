#!/usr/bin/env python3
"""Fetch complete, immutable official source archives (maintainer action only)."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tarfile
import tempfile
import urllib.request
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parent
REPOSITORIES = {
    "mqtbench": "munich-quantum-toolkit/bench",
    "qasmbench": "pnnl/QASMBench",
    "benchpress": "Qiskit/benchpress",
}


def request(url, target=None):
    req = urllib.request.Request(url, headers={"User-Agent": "QLLVM-official-benchmarks"})
    with urllib.request.urlopen(req, timeout=180) as response:
        if target is not None:
            with Path(target).open("wb") as handle:
                shutil.copyfileobj(response, handle)
        else:
            return json.load(response)


def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def tree_inventory(path):
    return {str(p.relative_to(path)): {"sha256": sha(p), "size": p.stat().st_size}
            for p in sorted(path.rglob("*")) if p.is_file()}


def load_lock():
    data = json.loads((ROOT / "sources.lock.json").read_text())
    if data.get("schema_version") != 1 or set(data["sources"]) != set(REPOSITORIES):
        raise ValueError("Invalid official source lock")
    return data["sources"]


def fetch(name, revision, reference, expected=None):
    repo = REPOSITORIES[name]
    target = ROOT / "upstream" / name
    if target.exists():
        raise ValueError(f"Refusing to overwrite {target}; verify it or use a new source directory")
    target.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=name + "-", dir=target.parent) as temporary:
        temporary = Path(temporary)
        archive = temporary / "source.tar.gz"
        url = f"https://codeload.github.com/{repo}/tar.gz/{revision}"
        print(f"Downloading full {name}@{revision}", flush=True)
        request(url, archive)
        archive_sha = sha(archive)
        if expected and archive_sha != expected["archive_sha256"]:
            raise ValueError("Downloaded archive does not match sources.lock.json: " + name)
        # These repositories are trusted upstreams. Still reject traversal and links
        # before extraction, and do not execute any source during source collection.
        with tarfile.open(archive) as tar:
            members = tar.getmembers()
            top = members[0].name.split("/")[0]
            for member in members:
                rel = Path(member.name)
                if rel.is_absolute() or ".." in rel.parts or rel.parts[0] != top:
                    raise ValueError("Unsafe archive path: " + member.name)
                if member.issym() or member.islnk() or member.isdev():
                    raise ValueError("Archive link/device needs explicit handling: " + member.name)
            tar.extractall(temporary / "extract")
        extracted = temporary / "extract" / top
        submodules = extracted / ".gitmodules"
        if submodules.exists() and submodules.read_text().strip():
            raise ValueError("Source has submodules; fetch them before claiming completeness")
        inventory = tree_inventory(extracted)
        pointers = [n for n in inventory if inventory[n]["size"] < 1024
                    and (extracted / n).read_bytes().startswith(b"version https://git-lfs.github.com/spec/v1")]
        if pointers:
            raise ValueError("Unresolved Git LFS files: " + repr(pointers))
        if expected and (len(inventory) != expected["file_count"] or
                         sum(x["size"] for x in inventory.values()) != expected["bytes"]):
            raise ValueError("Downloaded source inventory differs from lock: " + name)
        extracted.rename(target)
    record = {"repository": f"https://github.com/{repo}", "reference": reference,
              "revision": revision, "archive_url": url, "archive_sha256": archive_sha,
              "fetched_at": datetime.now(timezone.utc).isoformat(),
              "files": inventory, "file_count": len(inventory),
              "bytes": sum(x["size"] for x in inventory.values()),
              "scope": "Complete official source archive, including generators, tests, documentation and licenses; no file selection"}
    (ROOT / (name + ".source.json")).write_text(json.dumps(record, indent=2) + "\n")
    print(f"{name}: {len(inventory)} original files, {record['bytes']} bytes", flush=True)
    return record


def verify(name):
    receipt = ROOT / (name + ".source.json")
    if not receipt.is_file() or not (ROOT / "upstream" / name).is_dir():
        raise ValueError(f"Optional suite {name} is not downloaded. Run: python benchmark/official_suites/sources.py --fetch --source {name}")
    record = json.loads(receipt.read_text())
    if name in REPOSITORIES:
        expected = load_lock()[name]
        for key in ("repository", "revision", "archive_sha256", "file_count", "bytes"):
            if record.get(key) != expected[key]:
                raise ValueError(f"{name} receipt differs from sources.lock.json: {key}")
    actual = tree_inventory(ROOT / "upstream" / name)
    if actual != record["files"]:
        changed = sorted(k for k in actual.keys() | record["files"].keys()
                         if actual.get(k) != record["files"].get(k))
        raise ValueError(f"{name} source snapshot changed: {changed[:20]}")
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--verify", action="store_true", help="Offline integrity check")
    action.add_argument("--fetch", action="store_true", help="Download missing suites at the pinned versions")
    action.add_argument("--list", action="store_true", help="List optional dependencies without downloading")
    parser.add_argument("--source", choices=REPOSITORIES)
    args = parser.parse_args()
    try:
        locked = load_lock()
        for name in ([args.source] if args.source else REPOSITORIES):
            pin = locked[name]
            if args.list:
                print(name, pin["reference"], pin["revision"], "downloaded" if (ROOT/"upstream"/name).is_dir() else "optional/not downloaded")
                continue
            if args.fetch and not (ROOT/"upstream"/name).exists():
                fetch(name, pin["revision"], pin["reference"], pin)
            else:
                r = verify(name)
                print(name, r["revision"], r["file_count"], "files: OK")
    except (OSError, ValueError, KeyError, tarfile.TarError) as exc:
        parser.exit(2, f"Source dependency error: {exc}\n")


if __name__ == "__main__":
    main()
