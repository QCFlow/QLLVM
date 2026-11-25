"""Exercise wrapper discovery without importing or running the legacy driver."""
import ast
import os
from pathlib import Path
import shutil
import subprocess
import unittest
from unittest.mock import patch

DRIVER = Path(__file__).resolve().parents[2]/'tools/driver/qllvm.in'
tree = ast.parse(DRIVER.read_text())
function = next(n for n in ast.walk(tree) if isinstance(n,ast.FunctionDef) and n.name=='_resolve_mpi_compiler')
namespace = dict(os=os, subprocess=subprocess)
exec(compile(ast.fix_missing_locations(ast.Module(body=[function],type_ignores=[])),str(DRIVER),'exec'),namespace)
resolve = namespace['_resolve_mpi_compiler']


class MpiDriverFlagsTests(unittest.TestCase):
    def test_openmpi_and_mpich_discovery_preserve_compile_link_separation(self):
        outputs = [
            {'--showme:compile':'-I/mpi/include -pthread','--showme:link':'-L/mpi/lib -lmpi'},
            {'-compile_info':'g++ -I/mpi/include -pthread', '-link_info':'g++ -Wl,-rpath,/mpi/lib -L/mpi/lib -lmpich'},
            {'-show':'g++ -I/mpi/include -L/mpi/lib -lmpich'}]
        for output in outputs:
            with self.subTest(output=output):
                def query(command, **kwargs):
                    if command[1] not in output:raise subprocess.CalledProcessError(1,command)
                    return output[command[1]]
                with patch.object(subprocess,'check_output',side_effect=query):
                    wrapper, compile_flags, link_flags = resolve('/fake/mpicxx')
                self.assertEqual(wrapper, '/fake/mpicxx')
                self.assertIn('-I/mpi/include',compile_flags)
                self.assertFalse(any(f.startswith(('-l','-L','-Wl')) for f in compile_flags))
                self.assertTrue(any(f in link_flags for f in ['-lmpi','-lmpich']))

    @unittest.skipUnless(shutil.which('mpicxx'), 'MPI wrapper unavailable')
    def test_real_wrapper_flags_compile_and_link_a_program(self):
        import tempfile
        wrapper, compile_flags, link_flags = resolve(shutil.which('mpicxx'))
        with tempfile.TemporaryDirectory() as folder:
            source, obj, output = [Path(folder)/n for n in ['mpi.cpp','mpi.o','mpi-test']]
            source.write_text('#include <mpi.h>\nint main(int argc,char **argv){MPI_Init(&argc,&argv);return MPI_Finalize();}\n')
            subprocess.run(['g++',*compile_flags,'-c',str(source),'-o',str(obj)],check=True,capture_output=True)
            subprocess.run(['g++',str(obj),*link_flags,'-o',str(output)],check=True,capture_output=True)
            self.assertTrue(output.is_file())


if __name__=='__main__':
    unittest.main()
