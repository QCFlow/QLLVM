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
 * QIR Runner hybrid runtime — histogram from last qir_runner_execute() call.
 ******************************************************************************/
#ifndef QLLVM_QIR_RUNNER_RUNTIME_H
#define QLLVM_QIR_RUNNER_RUNTIME_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void qir_runner_set_shots(int shots);
void qir_runner_set_exe_dir(const char *dir);
void qir_runner_execute(const char *kernel_name, int shots);

/** Number of {bitstring -> count} entries after the last execute (sorted by bitstring). */
int qir_runner_last_histogram_size(void);

/**
 * Read one histogram entry by index.
 * @param index 0 .. qir_runner_last_histogram_size()-1
 * @param bitstring_out null-terminated bit string (e.g. "00"); needs room for n_bits+'\0'
 * @param bitstring_cap capacity of bitstring_out
 * @param count_out occurrence count for this bitstring
 * @return 0 on success, -1 on invalid index or buffer too small
 */
int qir_runner_last_histogram_get(int index, char *bitstring_out, size_t bitstring_cap,
                                    int *count_out);

#ifdef __cplusplus
}
#endif

#endif
