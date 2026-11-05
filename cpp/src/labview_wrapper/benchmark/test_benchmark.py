# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements. See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to you under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy at
# https://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Exercise the compiled C ABI in a separate, matching-bitness executable."""

import argparse
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    args = parser.parse_args()
    exe = args.exe.resolve(strict=True)
    env = os.environ.copy()
    env.pop("TSFILE_BENCH_INPUT", None)
    for key in ("PATH", "LD_LIBRARY_PATH"):
        env[key] = str(exe.parent) + os.pathsep + env.get(key, "")

    def invoke(*values, status=0):
        result = subprocess.run(
            [str(exe), *map(str, values)],
            env=env,
            capture_output=True,
            text=True,
            timeout=120,
        )
        marker = f"{'VERIFY' if values[0] == '--verify' else 'WRITE'}_STATUS={status}"
        if marker not in result.stdout or (result.returncode == 0) != (status == 0):
            raise AssertionError(f"{values}: {result.stdout}\n{result.stderr}")

    with tempfile.TemporaryDirectory(prefix="tsfile-isol-") as tmp:
        work = Path(tmp)
        generated = work / "generated.tsfile"
        invoke(generated, 17, 3)
        invoke("--verify", generated, 17, 3)
        metrics = dict(
            line.split("=", 1)
            for line in Path(str(generated) + ".metrics").read_text().splitlines()
        )
        assert metrics["TOTAL_ROWS"] == "51"
        assert metrics["TOTAL_POINTS"] == "459"
        assert metrics["STATUS"] == "0"
        assert metrics["INPUT_MODE"] == "generated"
        assert int(metrics["OUTPUT_BYTES"]) == generated.stat().st_size
        assert float(metrics["FULL_SECONDS"]) >= float(metrics["WRAPPER_BLOCK_SECONDS"])
        assert float(metrics["FULL_POINTS_PER_SECOND"]) > 0

        # A repeated request must preserve the previous file and metrics.
        digest = hashlib.sha256(generated.read_bytes()).digest()
        old_metrics = Path(str(generated) + ".metrics").read_bytes()
        invoke(generated, 17, 3, status=-2)
        assert hashlib.sha256(generated.read_bytes()).digest() == digest
        assert Path(str(generated) + ".metrics").read_bytes() == old_metrics
        invoke("--verify", generated, 17, 2, status=-8)
        # Exercise the unchanged, three-argument CLFN entry point as well.
        compat = work / "clfn-generated.tsfile"
        invoke("--compat", compat, 17, 3)
        assert generated.read_bytes() == compat.read_bytes()

        fixture = work / "source.bin"
        values = [row * 100.0 + col + 0.125 for row in range(23) for col in range(9)]
        fixture.write_bytes(struct.pack(f"<{len(values)}d", *values))
        target = work / "fixture.tsfile"
        invoke(target, 23, 4, fixture)
        invoke("--verify", target, 23, 4, fixture)
        wrong = work / "wrong.bin"
        wrong.write_bytes(struct.pack(f"<{len(values)}d", *[v + 1 for v in values]))
        invoke("--verify", target, 23, 4, wrong, status=-8)
        env["TSFILE_BENCH_INPUT"] = str(fixture)
        compat_fixture = work / "clfn-fixture.tsfile"
        invoke("--compat", compat_fixture, 23, 4)
        assert target.read_bytes() == compat_fixture.read_bytes()
        env.pop("TSFILE_BENCH_INPUT")

        invoke(work / "missing.tsfile", 23, 4, work / "missing.bin", status=-3)
        invoke(work / "short.tsfile", 24, 4, fixture, status=-3)
        invoke(work / "long.tsfile", 22, 4, fixture, status=-3)
        invoke(work / "bad\n.tsfile", 23, 4, status=-1)
        invoke(work / "absent" / "file.tsfile", 23, 4, status=-4)
        reserved = work / "reserved.tsfile"
        Path(str(reserved) + ".metrics").write_text("keep")
        invoke(reserved, 23, 4, status=-2)
        assert not reserved.exists()
        assert Path(str(reserved) + ".metrics").read_text() == "keep"

        # Same data and schema on repeated calls must remain deterministic.
        second = work / "repeat.tsfile"
        invoke(second, 17, 3)
        assert generated.read_bytes() == second.read_bytes()
        # Check a multi-page block and a boundary beyond the 4,096-row reader.
        large = work / "pages.tsfile"
        invoke(large, 6000, 3)
        invoke("--verify", large, 6000, 3)

    print("PASS: generated/fixture writes, full readback, timing and error cases")


if __name__ == "__main__":
    main()
