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
 * Mock MPIQ runtime for hardware-mode MPI hybrid builds (no real QPU required).
 ******************************************************************************/
#ifndef QLLVM_MPIQ_RUNTIME_H
#define QLLVM_MPIQ_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

/** Mock pulse dispatch: rank 0 reads compiled QASM, writes mock artifact, syncs all ranks. */
int qllvm_mpiq_execute(const char *kernel_name, const char *compiled_qasm_path,
                       const char *config_json_path);

#ifdef __cplusplus
}
#endif

#endif
