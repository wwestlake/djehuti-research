"""Extract a hash-verified Windows LLVM archive, removing its single root."""
import pathlib
import sys
import tarfile

archive, destination = sys.argv[1:]
root = pathlib.Path(destination).resolve()
root.mkdir(parents=True, exist_ok=True)
with tarfile.open(archive, "r|xz") as package:
    top = None
    for member in package:
        parts = pathlib.PurePosixPath(member.name).parts
        if not parts:
            continue
        if top is None:
            top = parts[0]
        if parts[0] != top or ".." in parts or member.issym() or member.islnk():
            raise ValueError("Unexpected LLVM archive path/link")
        if len(parts) == 1:
            continue
        member.name = "/".join(parts[1:])
        target = (root / member.name).resolve()
        if root not in target.parents:
            raise ValueError("LLVM archive path escapes staging directory")
        package.extract(member, str(root))
print("Extracted official LLVM development archive")
