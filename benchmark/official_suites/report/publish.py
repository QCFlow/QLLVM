"""Publish one frozen campaign as a self-contained performance-report webpage."""
from pathlib import Path
import argparse,collections,datetime,hashlib,json,subprocess,sys,tempfile

def assemble(payload,catalog,live):
    meta=payload['metadata'];campaign=meta['campaign_id'];compiler=meta['compiler_sha256']
    full=meta.get('qllvm_full_retest',False)
    groups=collections.defaultdict(list)
    for r in payload['results']:
        assert r['campaign_id']==campaign
        assert r['framework'] in meta['frameworks']
        if r['framework']=='qllvm' and r['status']!='out_of_scope':assert r.get('compiler_sha256')==compiler or r['status']=='unsupported'
        groups[r['id'],r['framework']].append(r)
    if meta.get('reuse_peer_results'):
        for old in payload.get('peer_reference',[]):
            assert old['framework']!='qllvm'
            r=dict(old,comparison_source='existing_peer_measurements')
            if r['status']=='out_of_scope':r.update(original_status=r['status'],status='no_reference')
            groups[r['id'],r['framework']].append(r)
    rows=[];summary=[];issues=[]
    current=live.get('current',{})
    for c in catalog:
        cid=c['id'];eligible=full or (c.get('generation_status')=='ready' and c.get('qubits') is not None and c['qubits']<=128 and c.get('bytes',0)<=8388608)
        for framework in meta['frameworks']:
            rs=groups[cid,framework]
            assert len({r['repetition'] for r in rs})==len(rs)
            by_rep={r['repetition']:r for r in rs}
            for rep in [1,2,3]:
                if rep in by_rep:rows.append(by_rep[rep]);continue
                state='pending' if eligible else 'out_of_scope'
                if meta.get('reuse_peer_results') and framework!='qllvm':state='no_reference'
                if eligible and current.get('id')==cid and current.get('framework')==framework:state='running'
                if framework=='qllvm' and any(x.get('id')==cid for x in live.get('current_cases',[])):state='running'
                rows.append(dict(id=cid,suite=c['suite'],framework=framework,repetition=rep,expected_repetitions=3,
                    status=state,campaign_id=campaign,compiler_sha256=compiler if framework=='qllvm' else None))
            if framework=='qllvm' and eligible:
                settled=len(rs)==3
                good=settled and all(r['status']=='performance_measured' for r in rs)
                if not good:
                    issue_state='pending' if not settled else '/'.join(sorted({r['status'] for r in rs}))
                    issues.append(dict(id=cid,suite=c['suite'],status=issue_state,reason='新版待复测' if not settled else next((r.get('error') for r in rs if r.get('error')),'新版未取得完整指标'),stage='新版队列' if not settled else '新版实测',next_action='等待新版实测' if not settled else '依据新版日志分析'))
    for suite in ['mqtbench','qasmbench','benchpress']:
        cs=[c for c in catalog if c['suite']==suite];eligible=cs if full else [c for c in cs if c.get('generation_status')=='ready' and c.get('qubits') is not None and c['qubits']<=128 and c.get('bytes',0)<=8388608]
        settled=[c for c in eligible if len(groups[c['id'],'qllvm'])==3]
        passed=sum(all(r['status']=='performance_measured' for r in groups[c['id'],'qllvm']) for c in settled)
        summary.append(dict(suite=suite,total=len(cs),eligible=len(eligible),processed=len(settled),tested=len(settled),passed=passed,pending=len(eligible)-len(settled),pass_pct=100*passed/len(settled) if settled else 0.0))
    tested=sum(s['tested'] for s in summary);passed=sum(s['passed'] for s in summary)
    now=datetime.datetime.now().astimezone().isoformat()
    status=dict(updated_at=now,suites=summary,tested=tested,passed=passed,pending=sum(s['pending'] for s in summary),eligible=sum(s['eligible'] for s in summary),pass_pct=100*passed/tested if tested else 0.0)
    result=dict(metadata=meta,results=rows,presentation=dict(latest_only=True,updated_at=now,campaign_id=campaign,compiler_sha256=compiler),counts=dict(collections.Counter(r['status'] for r in rows)))
    return result,status,issues


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--campaign',type=Path,required=True)
    parser.add_argument('--output',type=Path,default=Path(__file__).with_name('index.html'))
    args=parser.parse_args();campaign=args.campaign.resolve();folder=Path(__file__).resolve().parent
    payload=json.loads((campaign/'results.json').read_text())
    catalog=json.loads((campaign/'catalog.json').read_text())['cases'];live=json.loads((campaign/'status.json').read_text())
    compiler=campaign/'bin/qllvm-compile'
    if compiler.is_file():
        assert hashlib.sha256(compiler.read_bytes()).hexdigest()==payload['metadata']['compiler_sha256']
    if payload['metadata'].get('reuse_peer_results'):
        payload['peer_reference']=json.loads((campaign/'peer-reference.json').read_text())['results']
    merged,status,issues=assemble(payload,catalog,live)
    with tempfile.TemporaryDirectory(prefix='qllvm-report-') as temporary:
        data=Path(temporary)
        for name,obj in [('results.merged.json',merged),('catalog.json',dict(cases=catalog)),('merged_status.json',status),('remaining_issues.json',dict(cases=issues))]:
            (data/name).write_text(json.dumps(obj,ensure_ascii=False))
        subprocess.run([sys.executable,str(folder/'analyze.py'),'--data',str(data)],check=True)
        subprocess.run([sys.executable,str(folder/'build_web.py'),'--data',str(data),'--output',str(args.output.resolve())],check=True)
    print(json.dumps(dict(source_revision=payload['metadata']['source_revision'],tested=status['tested'],completed=status['passed'],pending=status['pending'],output=str(args.output)),ensure_ascii=False))

if __name__=='__main__':main()
