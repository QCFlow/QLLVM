"""Freeze a portable full-catalog QLLVM campaign, optionally reusing peer results."""
import argparse,datetime,hashlib,json,shutil,subprocess,sys
from pathlib import Path

def sha(path):
    digest=hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b''):digest.update(chunk)
    return digest.hexdigest()

def put(path,payload):path.write_text(json.dumps(payload,ensure_ascii=False,indent=2)+'\n')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--catalog',type=Path)
    p.add_argument('--reference',type=Path,help='Completed campaign supplying canonical inputs and peer-reference.json')
    p.add_argument('--peers',type=Path,help='Peer result JSON; QLLVM rows are rejected')
    p.add_argument('--source-revision',help='Actual compiler build revision; inferred from checkout if omitted')
    p.add_argument('--python',default=sys.executable)
    p.add_argument('--preload',help='Optional platform-specific LD_PRELOAD library')
    p.add_argument('--workers',type=int,default=8)
    p.add_argument('--memory-budget-gb',type=int,default=64)
    p.add_argument('--verify-qubits',type=int,default=0)
    p.add_argument('--verification-semantics',choices=['auto','coherent','instrument','terminal-classical'],default='auto')
    p.add_argument('--report',type=Path,help='HTML output; generated automatically during and after the run')
    args=p.parse_args();suite=Path(__file__).resolve().parent;root=suite.parents[1]
    if args.workers<1 or args.memory_budget_gb<32:p.error('Require workers >= 1 and memory budget >= 32 GiB')
    if not args.compiler.is_file():p.error('Compiler does not exist')
    reference=args.reference.resolve() if args.reference else None
    catalog=args.catalog.resolve() if args.catalog else (reference/'catalog.json' if reference else suite/'catalog.json')
    cases=json.loads(catalog.read_text())['cases'];candidates={}
    if reference:
        entries=json.loads((reference/'prepared.json').read_text())
        for cid,e in entries.items():
            path=Path(e['path']);assert sha(path)==e['sha256']
            candidates[cid]=dict(path=str(path),sha256=e['sha256'])
    peers_path=args.peers or (reference/'peer-reference.json' if reference else None)
    peers=json.loads(peers_path.read_text()) if peers_path else dict(results=[])
    assert all(r['framework']!='qllvm' for r in peers['results'])
    revision=args.source_revision or subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False);(out/'bin').mkdir();(out/'suite').mkdir()
    shutil.copy2(args.compiler,out/'bin/qllvm-compile')
    for path in suite.glob('*.py'):shutil.copy2(path,out/'suite'/path.name)
    for name in ['sources.lock.json','versions.json']:shutil.copy2(suite/name,out/'suite'/name)
    shutil.copy2(suite/'latest_parallel.py',out/'parallel_runner.py');shutil.copy2(catalog,out/'catalog.json')
    put(out/'peer-reference.json',peers);put(out/'input_candidates.json',candidates);put(out/'prepared.json',{})
    meta=dict(campaign_id=out.name,repository_root=str(root),input_root=str(suite),source_revision=revision,
        compiler=str(out/'bin/qllvm-compile'),compiler_sha256=sha(out/'bin/qllvm-compile'),
        code_sha256={p.name:sha(p) for p in (out/'suite').glob('*.py')},parallel_runner_sha256=sha(out/'parallel_runner.py'),
        python=str(Path(args.python).absolute()),library_preload=args.preload,started_at=datetime.datetime.now().astimezone().isoformat(),
        complete=False,frameworks=['qllvm','qiskit','tket','pyqpanda','pyqpanda3','pennylane'],
        qllvm_full_retest=True,reuse_peer_results=True,repetitions=3,opt='O1',verify_qubits=args.verify_qubits,verification_semantics=args.verification_semantics,
        parallel_cases=args.workers,memory_budget_gb=args.memory_budget_gb,serial_audit_cases=12,
        report_output=str(args.report.resolve() if args.report else out/'report.html'),
        timing_scope='Read + optimize + export; QLLVM CLI startup included; imports and metrics excluded.')
    put(out/'manifest.json',meta);put(out/'results.json',dict(metadata=meta,results=[]))
    put(out/'status.json',dict(state='prepared',qllvm_tested=0,qllvm_completed=0,qllvm_pending=len(cases)))
    print('Run:',sys.executable,str(out/'parallel_runner.py'),'--campaign',str(out))

if __name__=='__main__':main()
