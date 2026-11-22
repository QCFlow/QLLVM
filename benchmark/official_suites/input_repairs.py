"""Content-addressed corrections for identified official benchmark artifacts.

QASMBench commit 357b942396d5c2b7cbc1c229c585a6ef5ccaebac appended
measurements of undeclared q/c registers to HHL10. Restore the exact prior
official unmeasured circuit; never infer a multi-register measurement mapping.
"""
import hashlib


HHL_RESTORATIONS = {
    "07f6ec77d01a41db21212b70fb024e06e26c6c563914e8cd8abc434a190aaddc": {
        "file": "small/hhl_n10/hhl_n10.qasm",
        "revision": "4a4a3f3091a8a4d1bf5726971df4fdd7a17111fa",
        "blob": "917a7d918a3cfe41f4878f2bb87b6524cddb6739",
    },
    "02dacd7e0789c9650ff1628d2dc6525df701c0d919e684aa3f4837d50d24457b": {
        "file": "small/hhl_n10/hhl_n10_transpiled.qasm",
        "revision": "a7869b0879fd519e8b45b2a5592c49d1e1f3d4fd",
        "blob": "0794a5d74f87ad3de41377587a7af9bb764e1282",
    },
}


def restore_archived_hhl(source):
    """Return a documented correction only for either exact archived file."""
    raw = source.encode("utf-8")
    record = HHL_RESTORATIONS.get(hashlib.sha256(raw).hexdigest())
    if record is None:
        return source, []
    suffix = "".join("measure q[{}] -> c[{}];\n".format(i, i) for i in range(6))
    if not source.endswith(suffix):
        raise ValueError("Archived HHL measurement suffix does not match")
    restored = source[:-len(suffix)]
    content = restored.encode("utf-8")
    blob = hashlib.sha1(b"blob " + str(len(content)).encode() + b"\0" + content).hexdigest()
    if blob != record["blob"]:
        raise ValueError("HHL restoration does not match the prior official Git blob")
    return restored, [
        "restored official unmeasured {} at {}; removed six invalid appended "
        "measurements; verified Git blob {}".format(record["file"], record["revision"], blob)
    ]
