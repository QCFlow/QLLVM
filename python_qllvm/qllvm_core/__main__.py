"""Offline build and local execution CLI; never imports a user Python file."""
import argparse
import json
from pathlib import Path
import sys
from .program import Program, QLLVMError
from .runtime import Executable, LocalProvider, MockProvider, TargetCapabilities, build


def main(argv=None):
    parser=argparse.ArgumentParser(description="QLLVM direct-program build and local runtime (v0.1)")
    sub=parser.add_subparsers(dest="command",required=True)
    b=sub.add_parser("build");b.add_argument("program",type=Path);b.add_argument("-o","--output",required=True,type=Path)
    b.add_argument("--compiler");b.add_argument("--opt-level",type=int,choices=[0,1],default=0)
    b.add_argument("--provider",choices=["local","mock"],default="local")
    r=sub.add_parser("run");r.add_argument("bundle",type=Path);r.add_argument("--bindings",default="{}")
    r.add_argument("--shots",type=int,default=1024);r.add_argument("--seed",type=int)
    r.add_argument("--request-id");r.add_argument("--state-dir",required=True,type=Path)
    for name in ("status","result"):
        p=sub.add_parser(name);p.add_argument("job_id");p.add_argument("--state-dir",required=True,type=Path)
    args=parser.parse_args(argv)
    try:
        if args.command=="build":
            executable=build(Program(json.loads(args.program.read_text())),args.output,compiler=args.compiler,
                             opt_level=args.opt_level,target=TargetCapabilities(provider=args.provider))
            result={"bundle":str(executable.path),"executable_id":executable.fingerprint,
                    "parameter_binding":"specialize_before_codegen"}
        elif args.command=="run":
            executable=Executable(args.bundle)
            provider_type=MockProvider if executable.target["provider"]=="mock" else LocalProvider
            with provider_type(args.state_dir) as provider:
                job=provider.submit(executable,json.loads(args.bindings),shots=args.shots,seed=args.seed,
                                    request_id=args.request_id)
                answer=job.result()
                result={"job_id":job.job_id,"status":job.status(),"counts":answer.counts,
                        "provider":answer.provider,"simulated":answer.simulated}
        else:
            with LocalProvider(args.state_dir) as provider:
                job=provider.resume(args.job_id)
                result=provider.query(job.job_id)
                if args.command=="result":
                    answer=job.result(timeout=0)
                    result={"job_id":job.job_id,"counts":answer.counts,"provider":answer.provider,
                            "simulated":answer.simulated}
        print(json.dumps(result,ensure_ascii=False,sort_keys=True));return 0
    except (QLLVMError,OSError,ValueError,TimeoutError) as error:
        print(json.dumps({"error":getattr(error,"code",type(error).__name__),"message":str(error)},
                         ensure_ascii=False),file=sys.stderr)
        return 1


if __name__=="__main__":sys.exit(main())
