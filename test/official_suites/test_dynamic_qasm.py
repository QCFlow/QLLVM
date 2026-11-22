"""Single-bit feedback serialization and exhaustive instrument checks."""
from pathlib import Path
import sys,unittest
import tempfile
import numpy as np
from qiskit import QuantumCircuit,qasm2,transpile
from qiskit.quantum_info import Choi
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'benchmark/official_suites'))
from dynamic_qasm import has_feedback,serialize_dynamic_qasm2
from instrument_verification import instrument,verify_instrument
import worker

class DynamicBridgeTests(unittest.TestCase):
    def roundtrip(self,circuit):
        lowered=transpile(circuit,basis_gates=['u3','cx'],optimization_level=0,seed_transpiler=12345)
        output=qasm2.loads(serialize_dynamic_qasm2(lowered),custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertEqual(verify_instrument(circuit,output)['status'],'passed')
        return output

    def test_feedback_else_reset_and_repeated_measurement(self):
        c=QuantumCircuit(2,2);c.h(0);c.measure(0,1)
        with c.if_test((c.clbits[1],True)) as otherwise:c.x(1)
        with otherwise:c.z(1)
        c.reset(0);c.measure(1,0);c.measure(0,1)
        self.roundtrip(c)

    def test_measurement_mapping_and_bit_polarity_are_observable(self):
        c=QuantumCircuit(2,2);c.h(0);c.measure(0,1)
        with c.if_test((c.clbits[1],True)):c.x(1)
        output=self.roundtrip(c)
        text=qasm2.dumps(output)
        for changed in [text.replace('if (c1 == 1)','if (c1 == 0)'),text.replace('-> c1[0]','-> c0[0]')]:
            self.assertNotEqual(changed,text)
            self.assertEqual(verify_instrument(c,qasm2.loads(changed))['status'],'failed')

    def test_all_inputs_not_only_zero_state(self):
        a=QuantumCircuit(1);b=QuantumCircuit(1);b.z(0)
        self.assertEqual(verify_instrument(a,b)['status'],'failed')

    def test_reset_channel_matches_independent_qiskit_choi(self):
        c=QuantumCircuit(1);c.h(0);c.reset(0)
        self.assertTrue(np.allclose(instrument(c)[0],Choi(c).data,atol=1e-12,rtol=0))

    def test_classical_writes_in_if_body_are_rejected(self):
        c=QuantumCircuit(1,1)
        with c.if_test((c.clbits[0],True)):c.measure(0,0)
        with self.assertRaisesRegex(NotImplementedError,'write classical'):
            serialize_dynamic_qasm2(c)

    def test_multi_bit_register_condition_is_rejected(self):
        c=QuantumCircuit(1,2)
        with c.if_test((c.cregs[0],1)):c.x(0)
        with self.assertRaisesRegex(NotImplementedError,'Multi-bit'):
            serialize_dynamic_qasm2(c)

    def test_width_limit_does_not_claim_verification(self):
        c=QuantumCircuit(5)
        self.assertEqual(verify_instrument(c,c)['status'],'unsupported')

    def test_prepare_qasm3_feedback_preserves_metrics_and_outcomes(self):
        text='''OPENQASM 3.0;
include "stdgates.inc";
bit[2] c;
qubit[2] q;
h q[0];
c[1] = measure q[0];
if (c[1]) { x q[1]; }
c[0] = measure q[1];
'''
        with tempfile.TemporaryDirectory() as d:
            src,out=Path(d)/'in.qasm3',Path(d)/'out.qasm'
            src.write_text(text)
            metrics=worker.prepare(src,out)
            self.assertTrue(out.with_suffix('.serialization.json').exists())
            actual=worker.load_qasm(out)
            self.assertEqual(verify_instrument(worker.load_qasm(src),actual)['status'],'passed')
            self.assertEqual(metrics['operations'].get('if_else',0),0)
            self.assertEqual(metrics['operations']['u3'],2)

    def test_existing_qasm2_whole_register_condition_is_not_rewritten(self):
        text='OPENQASM 2.0; include "qelib1.inc"; qreg q[1]; creg c[64]; if(c==9223372036854775808) x q[0];'
        with tempfile.TemporaryDirectory() as d:
            src,out=Path(d)/'in.qasm',Path(d)/'out.qasm'
            src.write_text(text)
            worker.prepare(src,out)
            self.assertIn('9223372036854775808',out.read_text())
            self.assertFalse(out.with_suffix('.serialization.json').exists())

    def test_qiskit_optimized_feedback_export_after_shared_bridge(self):
        # Regression for the IQPE shared-preparation failure: Qiskit supports
        # feedback once the single-bit predicate is a whole QASM2 register.
        c=QuantumCircuit(2,1);c.h(0);c.x(1);c.cp(0.7,1,0);c.h(0);c.measure(0,0)
        with c.if_test((c.clbits[0],True)):c.x(0)
        shared=self.roundtrip(c)
        optimized=transpile(shared,basis_gates=worker.BASIS,optimization_level=3,seed_transpiler=12345)
        output=qasm2.loads(qasm2.dumps(optimized),custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertEqual(verify_instrument(c,output)['status'],'passed')
        self.assertTrue(has_feedback(output))

if __name__=='__main__':unittest.main()
