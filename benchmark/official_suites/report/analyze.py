"""Compute webpage statistics from complete same-input repetition groups."""
from pathlib import Path
import argparse,collections,csv,json,math,statistics as st,sys
sys.path.insert(0,str(Path(__file__).resolve().parent.parent))
from metric_statistics import two_qubit_outcomes
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--data',type=Path,required=True);args=p.parse_args()
DATA=args.data
payload=json.loads((DATA/'results.merged.json').read_text());rows=payload['results'];meta=payload['metadata']
merged_status=json.loads((DATA/'merged_status.json').read_text());catalog=json.loads((DATA/'catalog.json').read_text())['cases']
stamp=merged_status['updated_at']
names={'qllvm':'QLLVM','qiskit':'Qiskit','tket':'TKET','pyqpanda':'QPanda','pyqpanda3':'QPanda3','pennylane':'PennyLane'}
suites={'mqtbench':'MQT Bench','qasmbench':'QASMBench','benchpress':'Benchpress'}
opponents=['qiskit','tket','pyqpanda','pyqpanda3','pennylane']
groups=collections.defaultdict(lambda:collections.defaultdict(list))
for row in rows:groups[row['id']][row['framework']].append(row)
assert len({(r['id'],r['framework'],r['repetition']) for r in rows})==len(rows)
catmap={c['id']:c for c in catalog};order={c['id']:i for i,c in enumerate(catalog)}
def valid(a):
    return bool(a) and len(a)==a[0].get('expected_repetitions',3) and all(r['status']=='performance_measured' and r.get('pipeline_seconds') is not None for r in a)
def med(a,m):
    return st.median(r['pipeline_seconds'] if m=='time' else r['output_metrics'][m] for r in a)
def gm(v): return math.exp(st.mean(math.log(x) for x in v)) if v else None
def fmt(x,dec=1): return '—' if x is None else f'{x:.{dec}f}'
def reduction(r): return None if r is None else 100*(1-r)
def compareword(r,kind='少'):
    if r is None:return '不可比'
    x=100*(1-r)
    return ('减少' if x>=0 else '增加')+f'{abs(x):.1f}%'
def stats_for(f,suite=None):
    vals=collections.defaultdict(list)
    for p in pairs[f]:
        if suite and p['suite']!=suite:continue
        for m in ['time','gates','two_qubit_gates','depth']:
            if p[m+'_ratio'] is not None:vals[m].append(p[m+'_ratio'])
    result = {m:{'ratio':gm(vals[m]),'n':len(vals[m])} for m in ['time','gates','two_qubit_gates','depth']}
    result['two_qubit_outcomes'] = two_qubit_outcomes([
        dict(p, peer_two_qubit_gates=p[f+'_two_qubit_gates']) for p in pairs[f]], suite)
    return result

pairs={}
for f in opponents:
    out=[]
    for cid in sorted(groups,key=lambda c:(list(suites).index(catmap[c]['suite']),order[c])):
        a,b=groups[cid]['qllvm'],groups[cid][f]
        if not valid(a) or not valid(b):continue
        hs={r.get('actual_input_sha256') for r in a+b}
        if len(hs)!=1 or not next(iter(hs)):continue
        p={'index':len(out)+1,'id':cid,'suite':catmap[cid]['suite'],'qubits':catmap[cid].get('qubits'),
           'actual_input_sha256':next(iter(hs)), 'common_basis':all(r.get('common_basis') for r in a+b)}
        for m in ['time','gates','two_qubit_gates','depth']:
            x,y=med(a,m),med(b,m)
            p['qllvm_'+m]=x;p[f+'_'+m]=y
            p[m+'_ratio']=x/y if x>0 and y>0 and (m=='time' or p['common_basis']) else None
            p[m+'_reduction_pct']=reduction(p[m+'_ratio'])
        out.append(p)
    pairs[f]=out
    with (DATA/f'paired_{f}.csv').open('w',newline='',encoding='utf-8-sig') as h:
        fields=list(out[0]) if out else ['index','id','suite','qubits','actual_input_sha256','common_basis']+[
            name for m in ['time','gates','two_qubit_gates','depth'] for name in ['qllvm_'+m,f+'_'+m,m+'_ratio',m+'_reduction_pct']]
        w=csv.DictWriter(h,fieldnames=fields);w.writeheader();w.writerows(out)
overall={f:stats_for(f) for f in opponents}
suite_stats={s:{f:stats_for(f,s) for f in opponents} for s in suites}
progress=[]
for s in suites:
    cs=[c for c in catalog if c['suite']==s]; rr=[r for r in rows if r['suite']==s]; q=[r for r in rr if r['framework']=='qllvm']
    good=[g['qllvm'] for cid,g in groups.items() if catmap[cid]['suite']==s and valid(g['qllvm'])]
    counts={status:len({r['id'] for r in q if r['status']==status}) for status in {r['status'] for r in q}}
    complete=next(x['processed'] for x in merged_status['suites'] if x['suite']==s)
    progress.append({'suite':s,'total':len(cs),'complete':complete,'records':len(rr),'good':len(good),
                     'counts':counts,'median_seconds':st.median(med(g,'time') for g in good) if good else None})
analysis={'captured_at':stamp,'records':len(rows),'status':merged_status,'progress':progress,'overall':overall,'by_suite':suite_stats}
(DATA/'analysis.json').write_text(json.dumps(analysis,ensure_ascii=False,indent=2))
