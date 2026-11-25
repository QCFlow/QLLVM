from pathlib import Path
import os
import sys
import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "python_qllvm"))


@pytest.fixture
def compiler():
    path = os.environ.get("QLLVM_COMPILER")
    if not path or not Path(path).is_file():
        pytest.fail("Set QLLVM_COMPILER to the compiler rebuilt from this source tree")
    return path
