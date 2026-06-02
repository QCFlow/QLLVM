#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

echo "Building mpi_bell ..."
qllvm main_mpi.cpp bell.qasm -o mpi_bell -qpu qir-runner -mpi -O1

NP="${1:-4}"
SHOTS="${2:-512}"
echo "Running: mpirun -np $NP ./mpi_bell -shots $SHOTS"
mpirun -np "$NP" ./mpi_bell -shots "$SHOTS"
