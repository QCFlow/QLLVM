from pathlib import Path
import argparse,base64,collections,csv,html,json,statistics,re,sys

ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parent))
from result_diagnostics import failure_reason
p=argparse.ArgumentParser(description='Render the self-contained QLLVM performance report');p.add_argument('--data',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
DATA=args.data
analysis=json.loads((DATA/'analysis.json').read_text())
status=json.loads((DATA/'merged_status.json').read_text())
merged=json.loads((DATA/'results.merged.json').read_text())
catalog={c['id']:c for c in json.loads((DATA/'catalog.json').read_text())['cases']}
suites={'mqtbench':'MQT Bench','qasmbench':'QASMBench','benchpress':'Benchpress'}
names={'qiskit':'Qiskit','tket':'TKET','pyqpanda3':'QPanda3','pennylane':'PennyLane','pyqpanda':'QPanda'}
ids={'pyqpanda3':'qpanda3','pyqpanda':'qpanda'}
def reduction(x):return None if x is None else f'{100*(1-x):.1f}'
def timing(x):return None if x is None else f'{x:.2f}'
def speedup(x):return None if x is None or x<=0 else f'{1/x:.2f}'
latest=merged.get('presentation',{}).get('latest_only',False)
revision=merged['metadata'].get('source_revision','04baf82')[:7]
peers=[];series={}
for f,name in names.items():
    st=analysis['overall'][f];peer={'id':ids.get(f,f),'name':name,
        'gates':reduction(st['gates']['ratio']),'nGates':st['gates']['n'],
        'two':reduction(st['two_qubit_gates']['ratio']),'nTwo':st['two_qubit_gates']['n'],
        'depth':reduction(st['depth']['ratio']),'nDepth':st['depth']['n'],
        'time':speedup(st['time']['ratio']),'nTime':st['time']['n'],
        'twoOutcomes':st.get('two_qubit_outcomes'),'suites':None}
    if f!='pyqpanda':
        peer['suites']=[]
        for s,label in suites.items():
            v=analysis['by_suite'][s][f]
            peer['suites'].append([label]+[reduction(v[m]['ratio']) for m in ['gates','two_qubit_gates','depth']]+[speedup(v['time']['ratio'])])
    peer['bySuite']={}
    for suite in suites:
        v=analysis['by_suite'][suite][f]
        peer['bySuite'][suite]={
            'gates':reduction(v['gates']['ratio']),'nGates':v['gates']['n'],
            'two':reduction(v['two_qubit_gates']['ratio']),'nTwo':v['two_qubit_gates']['n'],
            'depth':reduction(v['depth']['ratio']),'nDepth':v['depth']['n'],
            'time':speedup(v['time']['ratio']),'nTime':v['time']['n'],
            'twoOutcomes':v.get('two_qubit_outcomes')}
    peers.append(peer)
    with (DATA/f'paired_{f}.csv').open(encoding='utf-8-sig') as file:rows=list(csv.DictReader(file))
    item={'suites':[],'labels':[r['id'] for r in rows]}
    for s,label in suites.items():
        nums=[i+1 for i,r in enumerate(rows) if r['suite']==s]
        if nums:item['suites'].append([label,min(nums),max(nums)])
    for name,key in [('gates','gates'),('two','two_qubit_gates'),('depth','depth')]:
        item[name]=[round(float(r[key+'_reduction_pct']),2) if r[key+'_reduction_pct'] else None for r in rows]
    series[ids.get(f,f)]=item

group=collections.defaultdict(list)
for r in merged['results']:
    group[(r['id'],r['framework'])].append(r)
framework_names={'qllvm':'QLLVM',**names}
framework_order=list(framework_names)
issues_path=DATA/'remaining_issues.json'
issues={r['id']:r for r in json.loads(issues_path.read_text())['cases']} if issues_path.exists() else {}
cases=[]
for (cid,framework),rows in group.items():
    if len(rows)!=rows[0].get('expected_repetitions',3):continue
    if all(r['status']=='out_of_scope' for r in rows):continue
    good=all(r['status']=='performance_measured' for r in rows)
    statuses={r['status'] for r in rows}
    settled=not bool(statuses & {'pending','running'})
    label='完成' if good else next((label for key,label in [
        ('running','复测中'),('pending','待复测'),
        ('timeout','超时'),('semantic_failed','语义校验失败'),
        ('unsupported','暂不支持'),('unavailable','环境不可用'),('failed','编译失败'),
        ('metrics_failed','指标采集失败')] if key in statuses),'未完成')
    if 'no_reference' in statuses:label='暂无对比数据'
    case={'id':cid,'suite':catalog[cid]['suite'],'qubits':catalog[cid].get('qubits'),
          'framework':framework,'compiler':framework_names[framework],
          'passed':good,'status':label,'settled':settled,
          'compiler_sha256':rows[0].get('compiler_sha256') if framework=='qllvm' else None,
          'campaign_id':rows[0].get('campaign_id',merged['metadata'].get('campaign_id'))}
    for m in ['gates','two_qubit_gates','depth','pipeline_seconds']:
        case[m]=statistics.median(r['pipeline_seconds'] if m=='pipeline_seconds' else r['output_metrics'][m] for r in rows) if good else None
    issue=issues.get(cid,{}) if not good and framework=='qllvm' else {}
    case['reason']=issue.get('reason','') or (failure_reason(rows) if not good else '')
    case['stage']=issue.get('stage','')
    cases.append(case)
cases.sort(key=lambda r:(list(suites).index(r['suite']),r['qubits'] or 0,r['id'],framework_order.index(r['framework'])))
qllvm_cases=[{k:v for k,v in r.items() if k not in ('framework','compiler')} for r in cases if r['framework']=='qllvm']
assert sum(r['settled'] for r in qllvm_cases)==status['tested'] and sum(r['passed'] for r in qllvm_cases)==status['passed']
with (DATA/'merged_cases.csv').open('w',newline='',encoding='utf-8-sig') as f:
    w=csv.DictWriter(f,fieldnames=list(qllvm_cases[0]));w.writeheader();w.writerows(qllvm_cases)
with (DATA/'framework_cases.csv').open('w',newline='',encoding='utf-8-sig') as f:
    w=csv.DictWriter(f,fieldnames=list(cases[0]));w.writeheader();w.writerows(cases)

s=(ROOT/'template.html').read_text()
a=s.index('const peers = ');b=s.index('\nfunction el(',a)
s=s[:a]+'const peers = '+json.dumps(peers,ensure_ascii=False,separators=(',',':'))+';\nconst overviewStatus = '+json.dumps(status,ensure_ascii=False,separators=(',',':'))+';\n'+s[b:]
a=s.index('const caseSeries = ');b=s.index('\nconst caseNames',a)
s=s[:a]+'const caseSeries = '+json.dumps(series,ensure_ascii=False,separators=(',',':'))+';'+s[b:]
stamp=status['updated_at'][:16].replace('T',' ')
s=re.sub(r'<p class="when">.*?</p>',f'<p class="when">测试时间：2026-11-17 14:18 — {stamp}（北京时间）</p>',s,count=1)
if latest:
    start=merged['metadata']['started_at'][:16].replace('T',' ')
    s=re.sub(r'<p class="when">.*?</p>',f'<p class="when">测试时间：{start} — {stamp}（北京时间）<br>QLLVM {revision} · 新版全目录复测 · 自动更新</p>',s,count=1)
cards=f'''<details class="status-summary" id="completion-status">
<summary>QLLVM 性能测试完成情况</summary>
<div class="status-heading"><p>已测 <b>{status['tested']}</b> 项 · 完成 <b>{status['passed']}</b> 项 · 完成率 <b>{status['pass_pct']:.1f}%</b></p></div>
<div class="status-grid">'''
if latest:
    cards=cards.replace("</p></div>",f" · 待复测 <b>{status['pending']}</b> 项</p></div>",1)
for x in status['suites']:
    detail=f"新版已测 {x['tested']} / 待测 {x.get('pending',0)} / 范围内 {x.get('eligible',x['total'])}" if latest else f"目录进度 {x['processed']} / {x['total']}"
    cards+=f'''<article class="status-card" data-suite="{x['suite']}"><h3>{suites[x['suite']]}</h3><strong>{x['pass_pct']:.1f}%</strong><p>完成 {x['passed']} / 已测 {x['tested']}</p><small>{detail}</small></article>'''
cards+='</div></details>'
s=s.replace('\n</div>\n<script>', '\n'+cards+'\n</div>\n<script>', 1)
s=s.replace('测试后端为全连接 QASM 模拟器。','编译目标采用全连接配置，输出为 QASM。')
s=s.replace('的性能提升','的编译结果对比')
s=re.sub(r'(<dt>QLLVM</dt><dd>).*?(</dd>)',lambda m:m[1]+revision+' · SHA-256 '+merged['metadata']['compiler_sha256'][:12]+' · Release O1'+m[2],s)
meta=merged['metadata']
progress='，'.join(f"{suites[x['suite']]} {x['tested']}/{x['total']}" for x in status['suites'])
s=re.sub(r'测试集进度：[^<]*','测试集进度：'+progress+'。',s)
s=re.sub(r'<p>环境：.*?</p>','<p>每例三次独立测量；并行用例各固定单个物理核心、原生库及SDK线程为1。超时和内存不足保留实际状态。其他框架复用已有结果，按输入SHA一致配对。</p>',s)
s=s.replace('比较取相同输入的三次中位数。','比较取相同输入的三次实测中位数。')
a=s.index('  <section class="close">');b=s.index('  <section class="detail"',a)
s=s[:a]+('  <section class="close"><h2>测试结果概述</h2><p>图表按双方取得完整指标的同输入用例计算；门数和深度另要求共同输出门集。'+('本版本全目录测试已结束。' if meta.get('complete') else '尚未完成的用例显示当前进度。')+'</p></section>\n')+s[b:]
case_html='''<section class="case-browser" id="case-browser" hidden><h2>用例结果</h2>
<div class="case-filters"><input id="case-query" type="search" placeholder="搜索用例，如 qft_N038、hwb10" aria-label="搜索用例">
<select id="case-suite" aria-label="选择测试集"><option value="">全部测试集</option><option value="mqtbench">MQT Bench</option><option value="qasmbench">QASMBench</option><option value="benchpress">Benchpress</option></select>
<select id="case-framework" aria-label="选择编译器"><option value="">全部编译器</option>'''+''.join(f'<option value="{f}">{name}</option>' for f,name in framework_names.items())+'''</select>
<select id="case-status" aria-label="筛选结果"><option value="">全部结果</option><option value="pass">已完成</option><option value="fail">未完成</option></select></div>
<div class="case-table-wrap"><table class="case-table"><thead><tr><th>用例</th><th>编译器</th><th>结果</th><th>总门数</th><th>双比特门</th><th>深度</th><th>编译秒</th></tr></thead><tbody id="case-rows"></tbody></table></div>
<div class="case-pagination"><button id="case-prev" type="button">上一页</button><span id="case-page"></span><button id="case-next" type="button">下一页</button><a href="#" id="download-framework-cases">下载全部编译器结果</a><a href="#" id="download-remaining-issues">下载QLLVM剩余问题</a></div></section>'''
s=s.replace('  <details>', case_html+'\n  <details>', 1)
css='''
.close[hidden],.case-browser[hidden],.status-summary[hidden]{display:none}
.status-summary{margin:26px 0 0}.status-heading{display:flex;gap:16px;align-items:center;justify-content:space-between}.status-heading p{color:var(--muted);font-size:14px}.status-heading b{color:var(--ink)}
.status-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:14px}.status-card{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:18px}.status-card h3{font-size:15px;margin:0 0 10px}.status-card strong{font-size:30px;color:var(--blue-ink)}.status-card p{margin:6px 0;color:var(--muted);font-size:14px}.status-card small{color:var(--faint)}
.case-browser{margin-top:34px}.case-browser h2{font-size:20px}.case-filters{display:flex;gap:10px;flex-wrap:wrap;margin:14px 0}.case-filters input{flex:1;min-width:180px}.case-filters input,.case-filters select,.case-pagination button{font:inherit;font-size:14px;border:1px solid var(--line);border-radius:8px;padding:9px;background:white;color:var(--ink)}.case-table-wrap{overflow:auto;border:1px solid var(--line);border-radius:12px;background:white}.case-table{width:100%;border-collapse:collapse;font-size:13px;min-width:650px}.case-table th,.case-table td{padding:10px;border-bottom:1px solid var(--line);text-align:right}.case-table th:first-child,.case-table td:first-child{text-align:left;max-width:400px;overflow-wrap:anywhere}.case-table th{background:var(--soft)}.case-pass{color:#187447}.case-other{color:#9d4b16}.case-pagination{display:flex;gap:12px;align-items:center;flex-wrap:wrap;margin-top:12px;font-size:13px;color:var(--muted)}.case-pagination a{margin-left:auto}.case-pagination button:disabled{opacity:.4}.case-table small{display:block;color:var(--muted)}
@media(max-width:650px){.status-heading{display:block}.status-grid{grid-template-columns:1fr}.status-card{display:grid;grid-template-columns:1fr auto;gap:3px 10px}.status-card h3,.status-card p{margin:0}.status-card strong{grid-column:2;grid-row:1/3}.case-pagination a{margin-left:0}}
'''
s=s.replace('</style>',css+'\n</style>',1)
js='''
const allCases=__CASES__;
function downloadCases(event,remaining){
  event.preventDefault();
  const keys=['id','suite','compiler','status','gates','two_qubit_gates','depth','pipeline_seconds','reason'];
  const selected=remaining?allCases.filter(r=>r.framework==='qllvm'&&!r.passed):allCases;
  const quote=value=>'"'+String(value??'').replaceAll('"','""')+'"';
  const csv='\\ufeff'+[keys,...selected.map(r=>keys.map(k=>r[k]))].map(row=>row.map(quote).join(',')).join('\\r\\n');
  const url=URL.createObjectURL(new Blob([csv],{type:'text/csv;charset=utf-8'}));
  const a=document.createElement('a');a.href=url;a.download=remaining?'remaining_issues.csv':'framework_cases.csv';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
}
document.getElementById('download-framework-cases').addEventListener('click',e=>downloadCases(e,false));
document.getElementById('download-remaining-issues').addEventListener('click',e=>downloadCases(e,true));
let casePage=0;
const casePageSize=20;
function renderCases(){
  const query=document.getElementById('case-query').value.trim().toLowerCase();
  const suite=document.getElementById('case-suite').value;
  const framework=document.getElementById('case-framework').value;
  const mode=document.getElementById('case-status').value;
  const filtered=allCases.filter(r=>(!query||r.id.toLowerCase().includes(query))&&(!suite||r.suite===suite)&&(!framework||r.framework===framework)&&(!mode||(mode==='pass')===r.passed));
  const pages=Math.max(1,Math.ceil(filtered.length/casePageSize));casePage=Math.min(casePage,pages-1);
  const tbody=document.getElementById('case-rows');tbody.replaceChildren();
  filtered.slice(casePage*casePageSize,(casePage+1)*casePageSize).forEach(r=>{
    const row=document.createElement('tr');const label=document.createElement('td');
    label.textContent=r.id;if(r.reason){const note=document.createElement('small');note.textContent=r.reason;note.title=r.stage;label.append(note)}row.append(label);
    const compiler=document.createElement('td');compiler.textContent=r.compiler;row.append(compiler);
    const status=document.createElement('td');status.textContent=r.status;status.className=r.passed?'case-pass':'case-other';row.append(status);
    ['gates','two_qubit_gates','depth','pipeline_seconds'].forEach(k=>{const td=document.createElement('td');td.textContent=r[k]==null?'—':k==='pipeline_seconds'?r[k].toFixed(3):r[k].toLocaleString();row.append(td)});tbody.append(row);
  });
  if(!filtered.length){const row=document.createElement('tr');const cell=document.createElement('td');cell.colSpan=7;cell.textContent='当前筛选没有用例';row.append(cell);tbody.append(row)}
  document.getElementById('case-page').textContent=`共 ${new Set(filtered.map(r=>r.id)).size} 个用例 · ${filtered.length} 条编译器结果 · ${casePage+1} / ${pages} 页`;
  document.getElementById('case-prev').disabled=casePage===0;document.getElementById('case-next').disabled=casePage>=pages-1;
}
['case-query','case-suite','case-framework','case-status'].forEach(id=>document.getElementById(id).addEventListener(id==='case-query'?'input':'change',()=>{casePage=0;renderCases()}));
document.getElementById('case-prev').addEventListener('click',()=>{casePage--;renderCases()});
document.getElementById('case-next').addEventListener('click',()=>{casePage++;renderCases()});renderCases();
'''.replace('__CASES__',json.dumps(cases,ensure_ascii=False,separators=(',',':')))
s=s.replace('</script>',js+'\n</script>',1)
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.with_suffix('.tmp').write_text(s)
print('Web updated:',status['passed'],'/',status['tested'], 'cases; paired counts',[(p['name'],p['nTime']) for p in peers])

args.output.with_suffix('.tmp').replace(args.output)
