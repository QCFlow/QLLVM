/*
 * This code is part of QLLVM.
 *
 * (C) Copyright QCFlow 2026.
 *
 * This code is licensed under the Apache License, Version 2.0. You may
 * obtain a copy of this license in the LICENSE file in the root directory
 * of this source tree or at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Any modifications or derivative works of this code must retain this
 * copyright notice, and modified files need to carry a notice indicating
 * that they have been altered from the originals.
 */
/*******************************************************************************
 * MPI + qir-runner hybrid runtime — rank-0 execute, collective histogram sync.
 ******************************************************************************/
#ifndef QLLVM_MPI_RUNTIME_H
#define QLLVM_MPI_RUNTIME_H

#include <mpi.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Execute quantum kernel on root rank, then broadcast histogram to all ranks. */
void qllvm_mpi_execute_kernel(const char *kernel_name, int shots);

/** Each rank runs qir-runner locally (no histogram broadcast). For -rank-kernel-map. */
void qllvm_mpi_execute_kernel_local(const char *kernel_name, int shots);

void qllvm_mpi_bcast_histogram(MPI_Comm comm, int root);

#ifdef __cplusplus
}
#endif

#endif
