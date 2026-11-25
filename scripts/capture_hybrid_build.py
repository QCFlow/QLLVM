#!/usr/bin/env python3
"""Record source inputs, native binary and test environment without reading secrets."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tarfile


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, cwd=None):
    return subprocess.check_output(command, cwd=cwd, text=True).strip()


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--compiler',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    files=run(['git','ls-files','--cached','--others','--exclude-standard'],root).splitlines()
    # Include the whole repository's tracked/unignored inputs, excluding only
    # review outputs if someone intentionally puts them inside this tree.
    files=sorted(set(p for p in files if (root/p).is_file() and not (root/p).resolve().is_relative_to(out)))
    entries=[{'path':p,'sha256':sha(root/p),'bytes':(root/p).stat().st_size} for p in files]
    manifest={'baseline_commit':run(['git','rev-parse','HEAD'],root),
              'branch':run(['git','branch','--show-current'],root),
              'working_tree_status':run(['git','status','--porcelain=v1'],root),
              'source_inputs':entries,'compiler':{'path':str(args.compiler.resolve()),'sha256':sha(args.compiler)},
              'python':sys.version,'python_executable':sys.executable,
              'environment':run([sys.executable,'-m','pip','freeze']).splitlines(),
              'note':'Source snapshot + build logs + binary hash. External LLVM/system dependencies are recorded separately; no claim of hermetic build.'}
    builddir=args.compiler.resolve().parent.parent
    cache=builddir/'CMakeCache.txt'
    if cache.is_file():
        keys=('CMAKE_BUILD_TYPE','CMAKE_CXX_COMPILER','CMAKE_C_COMPILER','LLVM_DIR','MLIR_DIR','LLVM_ROOT','CMAKE_HOME_DIRECTORY','QLLVM_ENABLE_MPI')
        manifest['cmake_configuration']=[line for line in cache.read_text().splitlines() if any(line.startswith(k+':') for k in keys)]
        manifest['cmake_cache_sha256']=sha(cache)
    dependencies=run(['ldd',str(args.compiler.resolve())])
    paths=[Path(line.split('=>',1)[1].split()[0]) for line in dependencies.splitlines() if '=>' in line and line.split('=>',1)[1].strip().startswith('/')]
    manifest['dynamic_libraries']=[{'path':str(p.resolve()),'sha256':sha(p)} for p in paths if p.is_file()]
    (out/'build-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    (out/'tracked-changes.patch').write_text(run(['git','diff','--binary','HEAD'],root)+'\n')
    with tarfile.open(out/'source-inputs.tar.gz','w:gz') as archive:
        for p in files:archive.add(root/p,arcname=p,recursive=False)
    (out/'dynamic-libraries.txt').write_text(dependencies+'\n')
    print(json.dumps({'inputs':len(entries),'compiler_sha256':manifest['compiler']['sha256'],'output':str(out)}))


if __name__=='__main__':main()
