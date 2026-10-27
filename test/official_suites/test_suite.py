"""Behavioral checks for complete source snapshots and honest benchmark reports."""
import argparse
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import io
import tarfile
import shutil
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2] / "benchmark/official_suites"
sys.path.insert(0, str(ROOT))
import sources

spec = importlib.util.spec_from_file_location("official_suite_runner", ROOT / "run.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
report_spec = importlib.util.spec_from_file_location("official_suite_report", ROOT / "report.py")
reporter = importlib.util.module_from_spec(report_spec)
report_spec.loader.exec_module(reporter)


class IntegrityTests(unittest.TestCase):
    def fixture_download(self, folder):
        archive = folder / "fixture.tar.gz"
        with tarfile.open(archive, "w:gz") as handle:
            content = b"official generator\n"
            info = tarfile.TarInfo("repo-revision/generator.py")
            info.size = len(content)
            handle.addfile(info, io.BytesIO(content))
        pin = {"repository": "https://github.com/munich-quantum-toolkit/bench",
               "revision": "a" * 40, "reference": "test-tag",
               "archive_sha256": sources.sha(archive), "file_count": 1, "bytes": len(content)}
        lock = {"schema_version": 1, "sources": {name: pin for name in sources.REPOSITORIES}}
        (folder / "sources.lock.json").write_text(json.dumps(lock))
        return archive, pin

    def test_optional_fetch_matches_lock_and_repeat_is_offline(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            archive, pin = self.fixture_download(folder)
            with patch.object(sources, "ROOT", folder), patch.object(sources, "request", side_effect=lambda url, target: shutil.copyfile(archive, target)) as request:
                sources.fetch("mqtbench", pin["revision"], pin["reference"], pin)
                self.assertEqual(request.call_count, 1)
                with patch.object(sys, "argv", ["sources.py", "--fetch", "--source", "mqtbench"]):
                    sources.main()
                self.assertEqual(request.call_count, 1)
                self.assertEqual(sources.verify("mqtbench")["file_count"], 1)

    def test_download_hash_mismatch_does_not_install_source(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            archive, pin = self.fixture_download(folder)
            pin["archive_sha256"] = "0" * 64
            with patch.object(sources, "ROOT", folder), patch.object(sources, "request", side_effect=lambda url, target: shutil.copyfile(archive, target)):
                with self.assertRaisesRegex(ValueError, "does not match"):
                    sources.fetch("mqtbench", pin["revision"], pin["reference"], pin)
            self.assertFalse((folder / "upstream/mqtbench").exists())

    def test_missing_optional_suite_has_download_instructions(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(sources, "ROOT", Path(directory)):
            with self.assertRaisesRegex(ValueError, "--fetch --source mqtbench"):
                sources.verify("mqtbench")

    def test_full_snapshot_rejects_modified_missing_and_extra_files(self):
        with tempfile.TemporaryDirectory() as folder:
            folder = Path(folder)
            source = folder / "upstream/example"
            source.mkdir(parents=True)
            original = source / "generator.py"
            original.write_text("original official generator\n")
            record = {"files": sources.tree_inventory(source)}
            (folder / "example.source.json").write_text(json.dumps(record))
            with patch.object(sources, "ROOT", folder):
                sources.verify("example")
                original.write_text("changed\n")
                with self.assertRaises(ValueError):
                    sources.verify("example")
                original.unlink()
                with self.assertRaises(ValueError):
                    sources.verify("example")
                original.write_text("original official generator\n")
                (source / "extra.py").write_text("unrecorded code")
                with self.assertRaises(ValueError):
                    sources.verify("example")

    def test_catalog_cannot_read_outside_suite(self):
        with self.assertRaises(ValueError):
            runner.safe_path("../../README.md")


class ReportingTests(unittest.TestCase):
    def row(self, case, framework, status="verified", repetition=1):
        return {"id": case, "framework": framework, "status": status,
                "repetition": repetition, "pipeline_seconds": 0.1}

    def test_different_cases_never_form_a_comparison(self):
        rows = [self.row("A", "qllvm"), self.row("B", "qiskit")]
        self.assertEqual(runner.paired_samples(rows, ["qllvm", "qiskit"]), {})

    def test_compile_only_and_classical_equivalence_do_not_qualify(self):
        for status in ["compile_only", "compiled_unverified", "semantic_failed", "unsupported", "failed", "timeout"]:
            rows = [self.row("A", "qllvm", status), self.row("A", "qiskit")]
            self.assertEqual(runner.paired_samples(rows, ["qllvm", "qiskit"]), {})

    def test_every_repetition_must_pass(self):
        rows = [self.row("A", "qllvm"), self.row("A", "qllvm", "semantic_failed", 2),
                self.row("A", "qiskit"), self.row("A", "qiskit", repetition=2)]
        self.assertEqual(runner.paired_samples(rows, ["qllvm", "qiskit"], 2), {})
        rows[1]["status"] = "verified"
        self.assertIn("A", runner.paired_samples(rows, ["qllvm", "qiskit"], 2))

    def test_missing_repetitions_are_not_complete(self):
        rows = [self.row("A", "qllvm"), self.row("A", "qiskit")]
        self.assertEqual(runner.paired_samples(rows, ["qllvm", "qiskit"], 3), {})

    def test_mismatched_input_content_cannot_be_ranked(self):
        rows = [self.row("A", "qllvm"), self.row("A", "qiskit")]
        rows[0]["actual_input_sha256"] = "first-input"
        rows[1]["actual_input_sha256"] = "different-input"
        self.assertEqual(runner.paired_samples(rows, ["qllvm", "qiskit"]), {})

    def test_unknown_requested_case_is_an_error(self):
        args = argparse.Namespace(suite=None, case=["does-not-exist"], profile="all")
        with self.assertRaises(ValueError):
            runner.choose_cases({"cases": [{"id": "known", "suite": "mqtbench"}]}, args)

    def test_no_result_file_is_never_success(self):
        with tempfile.TemporaryDirectory() as folder:
            result = runner.bounded([sys.executable, "-c", "pass"], Path(folder), 10, 0)
            self.assertEqual(result["status"], "failed")

    def test_timeout_is_reported(self):
        with tempfile.TemporaryDirectory() as folder:
            result = runner.bounded([sys.executable, "-c", "import time; time.sleep(10)"], Path(folder), 0.1, 0)
            self.assertEqual(result["status"], "timeout")

    def test_retest_merge_requires_explicit_replacement_and_matching_config(self):
        with tempfile.TemporaryDirectory() as folder:
            folder = Path(folder)
            metadata = {"complete": True, "frameworks": ["qllvm"], "selected_ids": ["A"],
                        "repetitions": 1, "tolerance": 1e-7}
            data = {"metadata": metadata, "results": [self.row("A", "qllvm")]}
            a, b = folder/"a.json", folder/"b.json"
            a.write_text(json.dumps(data)); b.write_text(json.dumps(data))
            with self.assertRaises(ValueError):
                reporter.combine([a, b])
            _, rows = reporter.combine([a, b], ["qllvm"])
            self.assertEqual(len(rows), 1)
            data["metadata"]["tolerance"] = 1e-3
            b.write_text(json.dumps(data))
            with self.assertRaises(ValueError):
                reporter.combine([a, b], ["qllvm"])

    def test_retest_merge_rejects_partial_trials(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"partial.json"
            path.write_text(json.dumps({"metadata": {"complete": True, "frameworks": ["qllvm"],
                "selected_ids": ["A"], "repetitions": 3}, "results": [self.row("A", "qllvm")]}))
            with self.assertRaises(ValueError):
                reporter.combine([path])


if __name__ == "__main__":
    unittest.main()
