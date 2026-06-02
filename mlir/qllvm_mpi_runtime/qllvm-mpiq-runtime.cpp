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
#include "qllvm-mpiq-runtime.h"

#include <mpi.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

const char *mock_out_dir() {
  const char *d = std::getenv("QLLVM_MPIQ_MOCK_DIR");
  return (d && *d) ? d : ".";
}

bool read_file_lines(const char *path, int *line_count_out) {
  FILE *fp = std::fopen(path, "r");
  if (!fp) return false;
  int lines = 0;
  char buf[4096];
  while (std::fgets(buf, sizeof(buf), fp) != nullptr) ++lines;
  std::fclose(fp);
  if (line_count_out) *line_count_out = lines;
  return true;
}

}  // namespace

extern "C" {

int qllvm_mpiq_execute(const char *kernel_name, const char *compiled_qasm_path,
                       const char *config_json_path) {
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) {
    int qasm_lines = 0;
    if (!compiled_qasm_path || !*compiled_qasm_path ||
        !read_file_lines(compiled_qasm_path, &qasm_lines)) {
      std::fprintf(stderr, "[mpiq-mock] failed to read compiled QASM: %s\n",
                   compiled_qasm_path ? compiled_qasm_path : "(null)");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    char out_path[1024];
    std::snprintf(out_path, sizeof(out_path), "%s/qllvm_mpir_mock_%s.json", mock_out_dir(),
                  kernel_name ? kernel_name : "kernel");

    FILE *out = std::fopen(out_path, "w");
    if (!out) {
      std::fprintf(stderr, "[mpiq-mock] failed to write %s\n", out_path);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    std::fprintf(out,
                 "{\n  \"mode\": \"mock\",\n  \"kernel\": \"%s\",\n  \"qasm\": \"%s\",\n"
                 "  \"config\": \"%s\",\n  \"qasm_lines\": %d,\n  \"status\": \"dispatched\"\n}\n",
                 kernel_name ? kernel_name : "", compiled_qasm_path ? compiled_qasm_path : "",
                 config_json_path ? config_json_path : "", qasm_lines);
    std::fclose(out);

    std::printf("[mpiq-mock] rank 0 dispatched kernel '%s' (%d qasm lines) -> %s\n",
                kernel_name ? kernel_name : "", qasm_lines, out_path);
    if (config_json_path && *config_json_path) {
      std::printf("[mpiq-mock] config: %s\n", config_json_path);
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if (rank != 0) {
    std::printf("[mpiq-mock] rank %d synchronized after mock dispatch of '%s'\n", rank,
                kernel_name ? kernel_name : "");
  }

  return 0;
}

}  // extern "C"
