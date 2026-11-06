"""Stream the existing S3 LLVM tar into isolated staging; verify every byte."""
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import tarfile

lock_path, destination = sys.argv[1:]
pin = json.loads(pathlib.Path(lock_path).read_text())["llvm"]
root = pathlib.Path(destination).resolve()
root.mkdir(parents=True, exist_ok=False)
process = subprocess.Popen(
    ["aws", "s3", "cp", pin["s3Uri"], "-", "--region", pin["region"], "--only-show-errors"],
    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
)
digest = hashlib.sha256()

class HashedStream:
    total = 0

    def read(self, size=-1):
        data = process.stdout.read(size)
        digest.update(data)
        self.total += len(data)
        return data

stream = HashedStream()
try:
    with tarfile.open(fileobj=stream, mode="r|") as archive:
        for member in archive:
            parts = pathlib.PurePosixPath(member.name).parts
            if ".." in parts or member.issym() or member.islnk():
                raise ValueError("Unexpected LLVM archive path/link")
            if parts[:2] != ("vcpkg_installed", "x64-windows") or len(parts) <= 2:
                continue
            member.name = "/".join(parts[2:])
            if root not in (root / member.name).resolve().parents:
                raise ValueError("LLVM path escapes staging directory")
            archive.extract(member, str(root))
    while stream.read(1024 * 1024):
        pass
    error = process.stderr.read().decode("utf-8", errors="replace")
    if process.wait() != 0:
        raise RuntimeError("S3 read failed: " + error)
    if stream.total != pin["bytes"] or digest.hexdigest() != pin["sha256"]:
        raise ValueError("S3 LLVM archive size/SHA256 mismatch")
    config = (root / "share/llvm/LLVMConfig.cmake").read_text()
    if not re.search(r"set\(LLVM_PACKAGE_VERSION\s+\"?" + re.escape(pin["version"]) + r"[\"\s)]", config):
        raise ValueError("Prebuilt LLVM version mismatch")
    (root / "s3-provenance.json").write_text(json.dumps(pin, indent=2))
    print("Verified and extracted pinned S3 LLVM archive: " + pin["sha256"])
finally:
    if process.poll() is None:
        process.terminate()
        process.wait(timeout=30)
