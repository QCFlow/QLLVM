"""Bounded parallel full-catalog QLLVM rerun, reusing counterpart measurements."""
from pathlib import Path
import argparse,collections,concurrent.futures,datetime,fcntl,hashlib,json,multiprocessing,os,shutil,subprocess,sys,tarfile,time,traceback

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b''):h.update(chunk)
    return h.hexdigest()

def atomic(path,payload):
    tmp=path.with_suffix('.tmp');tmp.write_text(json.dumps(payload,ensure_ascii=False,indent=2));tmp.replace(path)

def resources(size):
    # A token reserves one CPU and 8GiB address space. Oversized inputs are
    # attempted, never excluded on the basis of the old 128q/8MiB limits.
    return (4,32,1800) if size>8*1024**2 else (2,16,1800) if size>1024**2 else (1,8,600 if size>128*1024 else 180)

def run_case(job):
    out=Path(job['campaign']);case=job['case'];cid=case['id'];memory=job['memory'];timeout=job['timeout']
    cpu=job.get('cpu')
    if cpu is not None:os.sched_setaffinity(0,{cpu})
    sys.path.insert(0,str(out/'suite'))
    from run import bounded
    python=job['manifest']['python']
    compiler=out/'bin/qllvm-compile';manifest=job['manifest'];directory=out/'inputs'/cid;directory.mkdir(parents=True,exist_ok=True)
    def call(request,location,budget):
        original=location;attempt=0
        while location.exists():attempt+=1;location=original.with_name(original.name+f'-attempt-{attempt}')
        location.mkdir(parents=True);request=dict(request,directory=str(location));req=location/'request.json';atomic(req,request)
        r=bounded([python,'-B',str(out/'suite/worker.py'),'--request',str(req)],location,budget,memory,manifest.get('library_preload'))
        r.update(artifacts=str(location),timeout=budget,memory_gb=memory,cpu_affinity=[cpu] if cpu is not None else None)
        return r
    entry=job.get('prepared')
    if entry and entry.get('path'):assert sha(Path(entry['path']))==entry['sha256']
    else:
        candidate=job.get('candidate')
        if candidate:
            source=Path(candidate['path']);h=sha(source)
            if candidate.get('sha256'):assert h==candidate['sha256']
            target=directory/'compiled.qasm'
            if not target.exists():shutil.copy2(source,target)
            assert sha(target)==h
            entry=dict(path=str(target),sha256=h,source=str(source))
        else:
            source=(Path(manifest['input_root'])/case['file']).resolve()
            assert Path(manifest['input_root']).resolve() in source.parents
            assert sha(source)==case['sha256']
            if case.get('archive_member'):
                with tarfile.open(source) as archive:
                    member=archive.getmember(case['archive_member']);assert member.isfile()
                    target=directory/'original.qasm'
                    if not target.exists():
                        with archive.extractfile(member) as src,target.open('wb') as dst:shutil.copyfileobj(src,dst)
                    source=target
            prep=call(dict(framework='prepare',input=str(source),compiler=str(compiler),opt='O1',verify_qubits=manifest.get('verify_qubits',0),tolerance=1e-7),directory/'prepare',1800)
            if prep['status']=='prepared':
                target=Path(prep['artifacts'])/'compiled.qasm';entry=dict(path=str(target),sha256=sha(target),prepare=prep)
            else:
                # Still try the current compiler on the full official input.
                # A different input hash simply cannot be paired with a peer.
                entry=dict(path=str(source),sha256=sha(source),prepare=prep,original_input_fallback=True)
    rows=list(job.get('existing',[]));done={r['repetition'] for r in rows}
    for rep in [1,2,3]:
        if rep in done:continue
        req=dict(framework='qllvm',input=entry['path'],compiler=str(compiler),opt='O1',verify_qubits=manifest.get('verify_qubits',0),tolerance=1e-7)
        if manifest.get('verification_semantics','auto')!='auto':req['verification_semantics']=manifest['verification_semantics']
        r=call(req,out/'runs'/cid/'qllvm'/str(rep),timeout)
        metrics=r.get('output_metrics')
        if r['status'] in ['verified','compile_only','compiled_unverified'] and r.get('pipeline_seconds') is not None and metrics and all(metrics.get(k) is not None for k in ['gates','two_qubit_gates','depth']):
            r['original_status']=r['status'];r['status']='performance_measured'
        r.update(id=cid,suite=case['suite'],framework='qllvm',repetition=rep,expected_repetitions=3,
            campaign_id=manifest['campaign_id'],compiler_sha256=manifest['compiler_sha256'],
            actual_input_sha256=entry['sha256'],measurement_origin='current_qllvm_full_retest',execution_profile='parallel_cases_single_pinned_cpu',
            original_input_fallback=entry.get('original_input_fallback',False))
        rows.append(r)
        atomic(out/'runs'/cid/'qllvm-case-progress.json',dict(results=rows,prepared=entry))
    return dict(id=cid,rows=rows,prepared=entry)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--campaign',type=Path,required=True);args=parser.parse_args();out=args.campaign.resolve()
    lock=(out/'runner.lock').open('a');fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    meta=json.loads((out/'manifest.json').read_text());assert sha(out/'bin/qllvm-compile')==meta['compiler_sha256']
    assert sha(Path(__file__))==meta['parallel_runner_sha256']
    for name,value in meta['code_sha256'].items():assert sha(out/'suite'/name)==value
    catalog=json.loads((out/'catalog.json').read_text())['cases'];candidates=json.loads((out/'input_candidates.json').read_text())
    prepared=json.loads((out/'prepared.json').read_text()) if (out/'prepared.json').exists() else {}
    raw=json.loads((out/'results.json').read_text());groups=collections.defaultdict(list)
    for r in raw['results']:
        if r['framework']=='qllvm' and r['status']!='out_of_scope':groups[r['id']].append(r)
    # Recover each independently checkpointed repetition after interruption.
    for case in catalog:
        checkpoint=out/'runs'/case['id']/'qllvm-case-progress.json'
        if checkpoint.exists():
            data=json.loads(checkpoint.read_text());by_rep={r['repetition']:r for r in groups[case['id']]}
            for r in data['results']:
                assert r['compiler_sha256']==meta['compiler_sha256'];by_rep[r['repetition']]=r
            groups[case['id']]=list(by_rep.values());prepared[case['id']]=data['prepared']
    # Distinct physical cores, avoiding SMT siblings when topology is exposed.
    available=sorted(os.sched_getaffinity(0));cpus=[];seen=set()
    for cpu in available:
        topology=Path(f'/sys/devices/system/cpu/cpu{cpu}/topology')
        key=tuple((topology/name).read_text().strip() for name in ['physical_package_id','core_id']) if topology.exists() else cpu
        if key not in seen:cpus.append(cpu);seen.add(key)
        if len(cpus)==meta.get('parallel_cases',8):break
    max_workers=min(meta.get('parallel_cases',8),len(cpus));total_tokens=meta.get('memory_budget_gb',64)//8;active={};used=0;state='running';last_publish=0;last_save=0
    def save(force=False):
        nonlocal last_publish,last_save
        rows=[r for rs in groups.values() for r in rs];now=datetime.datetime.now().astimezone().isoformat()
        meta['complete']=state=='completed';atomic(out/'results.json',dict(metadata=meta,results=rows));atomic(out/'prepared.json',prepared)
        settled={cid:rs for cid,rs in groups.items() if len(rs)==3};passed=sum(all(r['status']=='performance_measured' for r in rs) for rs in settled.values())
        current=[dict(id=job['case']['id'],cpu=job['cpu'],memory_gb=job['memory'],timeout=job['timeout']) for job in active.values()]
        status=dict(state=state,updated_at=now,current_cases=current,recorded_results=len(rows),planned_records=len(catalog)*3,
            qllvm_tested=len(settled),qllvm_completed=passed,qllvm_pending=len(catalog)-len(settled),active_workers=len(active),reserved_memory_gb=used*8,
            max_workers=max_workers,memory_budget_gb=total_tokens*8,compiler_sha256=meta['compiler_sha256'],frameworks_to_measure=['qllvm'],reuse_peer_results=True)
        atomic(out/'status.json',status)
        suites=[]
        for suite in ['mqtbench','qasmbench','benchpress']:
            cs=[c for c in catalog if c['suite']==suite];done=[c for c in cs if c['id'] in settled];good=sum(all(r['status']=='performance_measured' for r in settled[c['id']]) for c in done)
            suites.append(f'| {suite} | {len(cs)} | {len(done)} | {good} | {len(cs)-len(done)} |')
        (out/'进度记录.md').write_text('# 当前QLLVM全目录复测\n\n'+f"版本{meta['source_revision'][:7]}；更新时间：{now}；状态：{state}\n\n"+
            f'QLLVM已测 {len(settled)}/{len(catalog)}，完成{passed}，待测{len(catalog)-len(settled)}。\n\n'+
            f'并行用例{len(active)}/{total_tokens}，预留地址空间合计{used*8}/{total_tokens*8} GiB。每个worker固定单个物理核心。\n\n'+
            f'其他五框架复用已有结果，只按相同输入SHA配对。QLLVM不使用旧版性能结果。目录全部{len(catalog)}项。\n\n'+
            '| 测试集 | 总数 | 已测 | 完成 | 待测 |\n|---|---:|---:|---:|---:|\n'+'\n'.join(suites)+'\n\n'+
            '当前用例：\n\n'+'\n'.join(f"- {x['id']}：CPU{x['cpu']}，{x['memory_gb']}GiB，{x['timeout']}秒" for x in current)+'\n\n日志：runner.log；机器状态：status.json；逐次结果：results.json。\n')
        last_save=time.monotonic()
        if meta.get('publish_on_completion_only') and state != 'completed':
            return
        publisher=Path(meta['repository_root'])/'benchmark/official_suites/report/publish.py'
        if not publisher.is_file():return
        if force or time.monotonic()-last_publish>=45:
            with (out/'publish.log').open('a') as log:
                done=subprocess.run([sys.executable,str(publisher),'--campaign',str(out),'--output',meta['report_output']],stdout=log,stderr=subprocess.STDOUT)
            if done.returncode:raise RuntimeError('Publication failed; see publish.log')
            last_publish=time.monotonic()
    priority={cid:i for i,cid in enumerate(meta.get('priority_ids',[]))}
    def estimated_size(c):
        entry=prepared.get(c['id']) or candidates.get(c['id'])
        if entry and entry.get('path'):return max(c.get('bytes',0) or 0,Path(entry['path']).stat().st_size)
        return c.get('bytes',0) or 0
    queue=sorted([c for c in catalog if len(groups[c['id']])!=3],key=lambda c:(0 if c['id'] in priority else 1,priority.get(c['id'],0),estimated_size(c),c['suite'],c['id']))
    try:
        save(True)
        with concurrent.futures.ProcessPoolExecutor(max_workers=max_workers,mp_context=multiprocessing.get_context('spawn')) as executor:
            while queue or active:
                occupied={job['cpu'] for job in active.values()}
                i=0
                while i<len(queue) and len(active)<max_workers:
                    case=queue[i];tokens,memory,timeout=resources(estimated_size(case))
                    if used+tokens>total_tokens:i+=1;continue
                    cpu=next(cpu for cpu in cpus if cpu not in occupied);occupied.add(cpu)
                    job=dict(campaign=str(out),case=case,manifest=meta,candidate=candidates.get(case['id']),prepared=prepared.get(case['id']),
                        existing=groups[case['id']],tokens=tokens,memory=memory,timeout=timeout,cpu=cpu)
                    future=executor.submit(run_case,job);active[future]=job;used+=tokens;queue.pop(i)
                completed,_=concurrent.futures.wait(active,timeout=5,return_when=concurrent.futures.FIRST_COMPLETED)
                for future in completed:
                    job=active.pop(future);used-=job['tokens'];cid=job['case']['id']
                    try:
                        result=future.result();groups[cid]=result['rows'];prepared[cid]=result['prepared']
                    except Exception as error:
                        # Infrastructure/input errors are visible per case and
                        # do not silently stop the entire full-catalog queue.
                        by_rep={r['repetition']:r for r in groups[cid]}
                        for rep in [1,2,3]:
                            if rep not in by_rep:by_rep[rep]=dict(id=cid,suite=job['case']['suite'],framework='qllvm',repetition=rep,expected_repetitions=3,
                                status='failed',error='Case worker error: '+repr(error),compiler_sha256=meta['compiler_sha256'],campaign_id=meta['campaign_id'])
                        groups[cid]=list(by_rep.values());atomic(out/'runs'/('failure-'+hashlib.sha256(cid.encode()).hexdigest()[:16]+'.json'),dict(id=cid,error=repr(error),traceback=traceback.format_exc()))
                    print(cid,[r['status'] for r in groups[cid]],flush=True)
                if completed or time.monotonic()-last_save>15:save()
        # Once the full directory has been attempted, audit the largest timing
        # gaps in isolation. Keep these as diagnostic evidence so the campaign
        # does not silently mix serial and parallel repetition groups.
        reference=collections.defaultdict(list)
        for r in json.loads((out/'peer-reference.json').read_text())['results']:
            if r['framework'] in ['qiskit','tket'] and r['status']=='performance_measured':reference[r['id'],r['framework']].append(r)
        import statistics
        ranked=[]
        by_id={c['id']:c for c in catalog}
        for cid,rs in groups.items():
            if len(rs)!=3 or not all(r['status']=='performance_measured' for r in rs):continue
            if estimated_size(by_id[cid])>1024**2:continue
            ratios=[]
            for peer in ['qiskit','tket']:
                xs=reference[cid,peer]
                if len(xs)!=3 or len({r.get('actual_input_sha256') for r in rs+xs})!=1:continue
                a=statistics.median(r['pipeline_seconds'] for r in rs);b=statistics.median(r['pipeline_seconds'] for r in xs)
                if b>0:ratios.append(a/b)
            if ratios:ranked.append((max(ratios),cid))
        state='serial_audit';save(True);audits=[]
        for ratio,cid in sorted(ranked,reverse=True)[:meta.get('serial_audit_cases',12)]:
            case=by_id[cid];tokens,memory,timeout=resources(estimated_size(case))
            result=run_case(dict(campaign=str(out),case=case,manifest=meta,candidate=candidates.get(cid),prepared=prepared.get(cid),
                existing=[],tokens=tokens,memory=memory,timeout=timeout,cpu=cpus[0]))
            for row in result['rows']:row['execution_profile']='serial_audit_single_pinned_cpu'
            audits.append(dict(id=cid,parallel_max_peer_ratio=ratio,results=result['rows']))
            # Restore the main queue checkpoint; audit runs never replace its
            # three original parallel measurements without explicit publication.
            atomic(out/'runs'/cid/'qllvm-case-progress.json',dict(results=groups[cid],prepared=prepared[cid]))
            atomic(out/'serial-audit.json',dict(results=audits))
            save()
        state='completed';save(True)
    except Exception:
        state='failed';save(True);raise

if __name__=='__main__':main()
