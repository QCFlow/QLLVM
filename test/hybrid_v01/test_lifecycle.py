import json
import os
import subprocess
import sys
import threading
import pytest
from qiskit import QuantumCircuit
import qllvm
from qllvm_core.runtime import atomic_json


def test_sdk_optional_import_and_legacy_subprocess_isolation(tmp_path):
    source=str(__import__('pathlib').Path(qllvm.__file__).parent)
    code=f'import sys;sys.path.insert(0,{source!r});import qllvm;assert not any(x in sys.modules for x in ["qiskit","cirq","pyqpanda3"])'
    result=subprocess.run([sys.executable,'-S','-c',code],capture_output=True,text=True)
    assert result.returncode==0,result.stderr


def test_unknown_interrupted_job_is_not_resubmitted(tmp_path):
    with qllvm.LocalProvider(tmp_path) as provider:
        path=provider._path('interrupted')
        atomic_json(path,{'schema_version':'0.1','kind':'qllvm.job','job_id':'interrupted',
                          'owner_pid':2**30,'status':'RUNNING'})
        job=provider.resume('interrupted')
        assert job.status()=='SUBMIT_UNKNOWN'
        with pytest.raises(qllvm.QLLVMError,match='JOB_SUBMIT_UNKNOWN'):job.result(timeout=0)


def test_running_timeout_and_queued_cancellation(tmp_path,compiler):
    qc=QuantumCircuit(1,1);qc.measure(0,0)
    e=qllvm.build(qllvm.import_program(qc),tmp_path/'circuit.qexe',compiler=compiler)
    started=threading.Event();release=threading.Event()

    class SlowProvider(qllvm.LocalProvider):
        def _run_artifact(self,*args):
            started.set()
            assert release.wait(timeout=30)
            return super()._run_artifact(*args)

    with SlowProvider(tmp_path/'jobs',workers=1) as provider:
        first=provider.submit(e,shots=2,request_id='first')
        try:
            assert started.wait(timeout=30)
            with pytest.raises(TimeoutError):first.result(timeout=0)
            assert first.status()=='RUNNING'
            queued=provider.submit(e,shots=3,request_id='queued')
            assert queued.cancel()=='CANCELLED'
            with pytest.raises(qllvm.QLLVMError,match='JOB_CANCELLED'):queued.result(timeout=0)
        finally:
            release.set()
        assert first.result(timeout=30).counts=={'0':2}
