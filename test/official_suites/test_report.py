"""Report reconstruction preserves paired metrics and excludes partial/stale data."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

REPORT=Path(__file__).resolve().parents[2]/'benchmark/official_suites/report'
spec=importlib.util.spec_from_file_location('qllvm_report_publish',REPORT/'publish.py')
publisher=importlib.util.module_from_spec(spec);spec.loader.exec_module(publisher)

class ReportTests(unittest.TestCase):
    def fixture(self):
        metadata=dict(campaign_id='fixture',source_revision='abcdef123456',compiler_sha256='binary',
            complete=True,qllvm_full_retest=True,reuse_peer_results=True,
            started_at='2026-11-22T00:00:00+08:00',frameworks=['qllvm','qiskit','tket','pyqpanda','pyqpanda3','pennylane'])
        catalog=[dict(id='case',suite='mqtbench',qubits=2)]
        def row(framework,n,gates,two,depth,seconds):
            return dict(id='case',suite='mqtbench',framework=framework,repetition=n,expected_repetitions=3,
                status='performance_measured',compiler_sha256='binary',campaign_id='fixture',
                actual_input_sha256='input',common_basis=True,pipeline_seconds=seconds,
                output_metrics=dict(gates=gates,two_qubit_gates=two,depth=depth))
        payload=dict(metadata=metadata,results=[row('qllvm',n,4,0,2,.5) for n in [1,2,3]],
                     peer_reference=[row('qiskit',n,8,2,4,2.) for n in [1,2,3]])
        return payload,catalog,dict(state='completed')

    def test_partial_and_old_compiler_rows_cannot_become_complete(self):
        payload,catalog,live=self.fixture();payload['results'].pop()
        merged,status,issues=publisher.assemble(payload,catalog,live)
        self.assertEqual(status['passed'],0);self.assertEqual(status['pending'],1)
        self.assertTrue(issues)
        payload['results'][0]['compiler_sha256']='stale'
        with self.assertRaises(AssertionError):publisher.assemble(payload,catalog,live)

    def test_rebuild_is_self_contained_and_keeps_zero_outcomes(self):
        payload,catalog,live=self.fixture()
        merged,status,issues=publisher.assemble(payload,catalog,live)
        with tempfile.TemporaryDirectory() as folder:
            d=Path(folder)
            for name,obj in [('results.merged.json',merged),('catalog.json',dict(cases=catalog)),('merged_status.json',status),('remaining_issues.json',dict(cases=issues))]:
                (d/name).write_text(json.dumps(obj))
            subprocess.run([sys.executable,str(REPORT/'analyze.py'),'--data',str(d)],check=True,capture_output=True)
            a=json.loads((d/'analysis.json').read_text())['overall']['qiskit']
            self.assertEqual(a['gates']['ratio'],.5)
            self.assertEqual(a['time']['ratio'],.25)
            self.assertEqual(a['two_qubit_gates']['n'],0)
            self.assertEqual(a['two_qubit_outcomes']['zero_better'],1)
            page=d/'index.html'
            subprocess.run([sys.executable,str(REPORT/'build_web.py'),'--data',str(d),'--output',str(page)],check=True,capture_output=True)
            text=page.read_text()
            self.assertIn('4.00',text)
            self.assertIn('50.0',text)
            self.assertIn('const allCases=',text)
            self.assertIn('downloadCases',text)
            self.assertIn('id="download-framework-cases"',text)
            self.assertNotIn('href="../data/',text)
            self.assertNotIn('.pdf"',text)
            self.assertNotIn('cell.appendChild(el("small", "", "配对 ',text)

    def test_input_hash_mismatch_not_paired(self):
        payload,catalog,live=self.fixture()
        payload['peer_reference'][0]['actual_input_sha256']='different'
        merged,status,issues=publisher.assemble(payload,catalog,live)
        with tempfile.TemporaryDirectory() as folder:
            d=Path(folder)
            for name,obj in [('results.merged.json',merged),('catalog.json',dict(cases=catalog)),('merged_status.json',status)]:
                (d/name).write_text(json.dumps(obj))
            subprocess.run([sys.executable,str(REPORT/'analyze.py'),'--data',str(d)],check=True,capture_output=True)
            a=json.loads((d/'analysis.json').read_text())['overall']['qiskit']
            self.assertEqual(a['time']['n'],0)
