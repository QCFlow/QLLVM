#!/usr/bin/env python3
"""Audit bundled archives and recompute published means; Python stdlib only.
Does not run the compiler, QCEC, performance experiments or semantic checkers.
"""
import posixpath
import argparse,collections,csv,hashlib,io,json,statistics,zipfile
from pathlib import Path

def sha(b): return hashlib.sha256(b).hexdigest()
def rows(b): return list(csv.DictReader(io.StringIO(b.decode('utf-8-sig'))))
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
 p.add_argument('--output',type=Path,required=True)
 p.add_argument('--current-status-csv',type=Path)
 a=p.parse_args(); root=a.root; report={};checks=collections.Counter()
 with zipfile.ZipFile(root/'QLLVM_semantic_validation_430.zip') as z:
  for name,digest in json.loads(z.read('SHA256SUMS.json')).items():
   assert sha(z.read(name))==digest, name
   checks['semantic_archive_manifest']+=1
  dispositions=rows(z.read('evidence_430/dispositions_430.csv'))
  lookup={r['case'].removesuffix('_O1'):r for r in dispositions}
  assert len(lookup)==430
  for r in dispositions:
   for name,field in [('original.qasm','source_sha256'),('output.qasm','output_sha256')]:
    assert sha(z.read('evidence_430/'+r['archive_directory']+'/'+name))==r[field],r['case']
    checks['input_output_circuit_hashes']+=1
   name=r['evidence']; name=name if name in z.namelist() else posixpath.normpath('evidence_430/'+name)
   assert sha(z.read(name))==r['evidence_sha256'],r['case']
   checks['per_case_evidence_hashes']+=1
  report['current_qllvm_status']=dict(collections.Counter(r['status'] for r in dispositions))
  report['current_methods']=dict(collections.Counter(r['method'] for r in dispositions))
 quality=rows((root/'scripts/data/paired_quality.csv').read_bytes());assert len(quality)==430
 joined=[]
 for r in quality:
  v=lookup[r['case']];assert r['qllvm_sha256']==v['output_sha256'];checks['plot_output_hash_links']+=1
  joined.append(dict(r,qllvm_historical_status=r['qllvm_status'],qllvm_current_status=v['status'],qllvm_current_method=v['method']))
 if a.current_status_csv:
  a.current_status_csv.parent.mkdir(parents=True,exist_ok=True)
  with a.current_status_csv.open('w',newline='') as f:
   w=csv.DictWriter(f,fieldnames=list(joined[0]));w.writeheader();w.writerows(joined)
 report['quality']={}
 for baseline,expected in [('qiskit',375),('cirq',216)]:
  selected=[r for r in joined if r[baseline+'_status']=='observational_equivalent_numeric' and r['qllvm_current_status']=='observational_equivalent_numeric']
  assert len(selected)==expected
  result={'historical_baseline_statuses':dict(collections.Counter(r[baseline+'_status'] for r in quality))}
  for label,rs in [('all',quality),('mutually_verified',selected)]:
   result[label]={'pairs':len(rs)}
   for metric in ['gate_count','depth']:
    result[label][metric+'_mean_relative_reduction_pct']=statistics.mean(100*(1-float(r['qllvm_'+metric])/float(r[baseline+'_'+metric])) for r in rs)
  report['quality'][baseline]=result
 with zipfile.ZipFile(root/'QLLVM_routing_data.zip') as z:
  records=json.loads(z.read('verified_results.json'));assert len(records)==207
  by_instance=collections.defaultdict(list)
  for r in records:
   by_instance[r['name']].append(r)
   for tool in ['qllvm','qiskit']:
    path=f"outputs/{r['name']}/{r['seed']}/{tool}.qasm"
    assert sha(z.read(path))==r[tool]['sha256'],path;checks['routing_output_hashes']+=1
  assert len(by_instance)==69 and all(len(v)==3 for v in by_instance.values())
  report['routing']={'instances':69,'seed_pairs':207}
  for metric in ['gates','depth','twoq']:
   report['routing'][metric+'_mean_relative_overhead_pct']=statistics.mean(statistics.mean(100*(r['qllvm'][metric]/r['qiskit'][metric]-1) for r in v) for v in by_instance.values())
 with zipfile.ZipFile(root/'QLLVM_framework_quality_data.zip') as z:
  records=json.loads(z.read('results/cudaq_quality_results.json'));valid=[r for r in records if r.get('normalized_both_verified')]
  assert len(records)==55 and len(valid)==54
  report['framework']={'cudaq':{'pairs':len(valid),**{m+'_mean_reduction_pct':statistics.mean(r['normalized_'+m+'_reduction_pct'] for r in valid) for m in ['gates','depth']}}}
  records=json.loads(z.read('results/native_quality_results.json'))
  for mode in ['xacc','qcor_precision']:
   valid=[r for r in records if r['mode']==mode and r.get('normalized_both_verified')];assert len(valid)==52
   report['framework'][mode]={'pairs':len(valid),**{m+'_mean_reduction_pct':statistics.mean(r['normalized_'+m+'_reduction_pct'] for r in valid) for m in ['gates','depth']}}
 report['checks']=dict(checks);report['all_hash_checks_pass']=True
 report['scope']='Archive integrity and numerical aggregation only; not independent semantic or performance verification.'
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
