"""Portable initializer freezes explicit resources and rejects stale inputs."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import unittest

SUITE=Path(__file__).resolve().parents[2]/'benchmark/official_suites'
sys.path.insert(0,str(SUITE))
import latest_parallel

class ParallelCampaignTests(unittest.TestCase):
    def test_resource_brackets(self):
        self.assertEqual(latest_parallel.resources(10),(1,8,180))
        self.assertEqual(latest_parallel.resources(200*1024),(1,8,600))
        self.assertEqual(latest_parallel.resources(2*1024**2),(2,16,1800))
        self.assertEqual(latest_parallel.resources(9*1024**2),(4,32,1800))

    def test_initializer_freezes_user_paths_and_rejects_wrong_input_hash(self):
        with tempfile.TemporaryDirectory() as folder:
            d=Path(folder);compiler=d/'compiler';compiler.write_bytes(b'fixture compiler')
            source=d/'source.qasm';source.write_text('OPENQASM 2.0; qreg q[1];')
            reference=d/'reference';reference.mkdir()
            catalog=dict(cases=[dict(id='one',suite='mqtbench',file='input.qasm',sha256='source')])
            (reference/'catalog.json').write_text(json.dumps(catalog))
            prepared=dict(one=dict(path=str(source),sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
            (reference/'prepared.json').write_text(json.dumps(prepared))
            (reference/'peer-reference.json').write_text(json.dumps(dict(results=[])))
            output=d/'campaign';page=d/'page.html';python_link=d/'sdk-python';python_link.symlink_to(sys.executable)
            cmd=[sys.executable,str(SUITE/'new_parallel_campaign.py'),'--reference',str(reference),
                 '--compiler',str(compiler),'--source-revision','fixture','--output',str(output),
                 '--workers','1','--memory-budget-gb','32','--report',str(page),'--python',str(python_link)]
            subprocess.run(cmd,check=True,capture_output=True,text=True)
            meta=json.loads((output/'manifest.json').read_text())
            self.assertEqual(meta['python'],str(python_link))
            self.assertEqual(meta['report_output'],str(page))
            self.assertEqual(meta['parallel_cases'],1)
            self.assertEqual(meta['memory_budget_gb'],32)
            self.assertEqual(meta['compiler_sha256'],hashlib.sha256(compiler.read_bytes()).hexdigest())
            for name,digest in meta['code_sha256'].items():
                self.assertEqual(hashlib.sha256((output/'suite'/name).read_bytes()).hexdigest(),digest)
            prepared['one']['sha256']='stale'
            (reference/'prepared.json').write_text(json.dumps(prepared))
            cmd[cmd.index('--output')+1]=str(d/'bad')
            result=subprocess.run(cmd,capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0)
