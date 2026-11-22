"""Exhaustive small-circuit instrument comparison, indexed by final classical bits.

Each measurement/reset branches Kraus operators. Comparing per-outcome Choi
matrices checks every quantum input, including coherent output. Classical input
is initialized to zero. Width/branch limits fail closed; no sampling is used.
"""
import math


def instrument(circuit, max_qubits=4, max_branches=256):
    import numpy as np
    from qiskit import QuantumCircuit
    from qiskit.quantum_info import Operator
    from dynamic_qasm import single_bit_condition
    n = circuit.num_qubits
    if n > max_qubits or circuit.num_clbits > 16:
        raise NotImplementedError('Above bounded instrument checker width')
    dim = 1 << n
    states = [(0, np.eye(dim, dtype=complex))]
    projectors = [np.diag([1,0]),np.diag([0,1])]
    resets = [np.array([[1,0],[0,0]],complex),np.array([[0,1],[0,0]],complex)]
    def embed(matrix, qubit):
        # Operator.compose supports general linear operators (not just unitary).
        return Operator(np.eye(dim)).compose(Operator(matrix),qargs=[qubit]).data
    def execute(scope, qmap, cmap, branches):
        for inst in scope.data:
            op=inst.operation
            qs=[qmap[scope.find_bit(q).index] for q in inst.qubits]
            cs=[cmap[scope.find_bit(c).index] for c in inst.clbits]
            if op.name=='barrier':continue
            if op.name=='if_else':
                index,value=single_bit_condition(op.condition,scope);bit=cmap[index]
                next_states=[]
                for classical,kraus in branches:
                    selected=0 if ((classical>>bit)&1)==value else 1
                    if selected<len(op.blocks):next_states.extend(execute(op.blocks[selected],qs,cs,[(classical,kraus)]))
                    else:next_states.append((classical,kraus))
                branches=next_states
            elif op.name in ('measure','reset'):
                if len(qs)!=1:raise NotImplementedError('Instrument operation arity')
                next_states=[]
                matrices=projectors if op.name=='measure' else resets
                embedded=[embed(m,qs[0]) for m in matrices]
                for classical,kraus in branches:
                    for outcome,matrix in enumerate(embedded):
                        result=matrix@kraus
                        if not np.any(result):continue
                        bits=classical
                        if op.name=='measure':
                            if len(cs)!=1:raise NotImplementedError('Measurement destination arity')
                            bits=(classical & ~(1<<cs[0])) | (outcome<<cs[0])
                        next_states.append((bits,result))
                branches=next_states
            else:
                if cs or op.name in ('for_loop','while_loop','switch_case','store'):
                    raise NotImplementedError('Unsupported instrument operation: '+op.name)
                gate=QuantumCircuit(n);gate.append(op,qs)
                matrix=Operator(gate).data
                if not np.all(np.isfinite(matrix)):raise ValueError('Nonfinite operator')
                branches=[(bits,matrix@kraus) for bits,kraus in branches]
            if len(branches)>max_branches:
                raise NotImplementedError('Above instrument branch limit')
        return branches
    states=execute(circuit,list(range(n)),list(range(circuit.num_clbits)),states)
    choi={};effect=np.zeros((dim,dim),complex)
    for classical,kraus in states:
        vector=kraus.reshape(-1,order='F')
        choi.setdefault(classical,np.zeros((dim*dim,dim*dim),complex))
        choi[classical]+=np.outer(vector,vector.conj())
        effect+=kraus.conj().T@kraus
    if np.max(abs(effect-np.eye(dim)))>1e-7:raise ValueError('Instrument is not trace preserving')
    return choi


def verify_instrument(original,compiled,max_qubits=4,tolerance=1e-7):
    import numpy as np
    if original.num_qubits!=compiled.num_qubits or original.num_clbits!=compiled.num_clbits:
        return {'status':'failed','reason':'Quantum/classical width changed'}
    try:
        a,b=instrument(original,min(max_qubits,4)),instrument(compiled,min(max_qubits,4))
        error=max((float(np.max(np.abs(a.get(k,0)-b.get(k,0)))) for k in set(a)|set(b)),default=0.)
        return {'status':'passed' if math.isfinite(error) and error<=tolerance else 'failed',
                'method':'full_quantum_instrument_per_classical_outcome_Choi',
                'max_abs_error':error,'tolerance':tolerance,
                'scope':'All quantum input states; classical input initialized to zero; final classical bits and quantum output preserved'}
    except NotImplementedError as exc:return {'status':'unsupported','reason':str(exc)}
    except Exception as exc:return {'status':'failed','reason':str(exc)}
