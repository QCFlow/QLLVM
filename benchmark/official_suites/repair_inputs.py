#!/usr/bin/env python3
"""Materialize reviewed QASMBench corrections without editing downloaded files."""
import argparse
import hashlib
import json
from pathlib import Path
from worker import normalize_qasm2_input

FILES = ['small/hhl_n10/hhl_n10.qasm',
         'small/hhl_n10/hhl_n10_transpiled.qasm',
         'large/vqe_uccsd_n28/vqe_uccsd_n28_transpiled.qasm']

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',type=Path,required=True)
    parser.add_argument('--source-manifest',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    metadata=json.loads(args.source_manifest.read_text())
    records=[]
    for relative in FILES:
        source=args.source_root/relative
        raw=source.read_bytes();original_hash=hashlib.sha256(raw).hexdigest()
        if original_hash != metadata['files'][relative]['sha256']:
            raise ValueError('Downloaded input differs from official archive manifest: '+relative)
        fixed,changes=normalize_qasm2_input(raw.decode('utf-8'))
        if not changes:raise ValueError('Expected reviewed correction: '+relative)
        destination=args.output/relative
        if destination.resolve()==source.resolve():raise ValueError('Preserve original downloaded input')
        destination.parent.mkdir(parents=True,exist_ok=True)
        destination.write_text(fixed)
        records.append(dict(file=relative,source=str(source.resolve()),source_sha256=original_hash,
                            output=str(destination.resolve()),output_sha256=hashlib.sha256(destination.read_bytes()).hexdigest(),changes=changes))
    manifest={'repository':metadata['repository'],'download_revision':metadata['revision'],
              'fetched_at':metadata['fetched_at'],'files':records}
    (args.output/'repairs.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(manifest,ensure_ascii=False,indent=2))

if __name__=='__main__':main()
