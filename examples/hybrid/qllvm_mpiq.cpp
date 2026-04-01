#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <mpi.h>
#include "../MPIQ/MPIQ.h"
#include <sys/wait.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void split_dir_base(const char *path, char *dir, size_t dir_sz,
                           char *base_out, size_t base_sz) {
  char copy[PATH_MAX];
  strncpy(copy, path, sizeof(copy) - 1);
  copy[sizeof(copy) - 1] = '\0';

  char *slash = strrchr(copy, '/');
  if (slash) {
    *slash = '\0';
    strncpy(dir, copy, dir_sz - 1);
    dir[dir_sz - 1] = '\0';
    strncpy(base_out, slash + 1, base_sz - 1);
  } else {
    strncpy(dir, ".", dir_sz - 1);
    dir[dir_sz - 1] = '\0';
    strncpy(base_out, copy, base_sz - 1);
  }
  base_out[base_sz - 1] = '\0';

  /* stem = 去掉最后一个扩展名 */
  char *dot = strrchr(base_out, '.');
  if (dot)
    *dot = '\0';
}

static int compile_qasm_with_qllvm(const char *src_qasm, char *out_path, size_t out_sz) {
  char dir[PATH_MAX], stem[256];
  split_dir_base(src_qasm, dir, sizeof(dir), stem, sizeof(stem));

  if (snprintf(out_path, out_sz, "%s/%s_compiled.qasm", dir, stem) >= (int)out_sz) {
    fprintf(stderr, "compiled path too long\n");
    return -1;
  }

  char cmd[8192];
  int n = snprintf(
      cmd, sizeof(cmd),
      "qllvm-compile \"%s\" -O1 -qrt nisq -qpu qasm-backend "
      "-emit-backend=qasm-backend "
      "-output-path=\"%s\" "
      "-internal-func-name \"%s\"",
      src_qasm, out_path, stem);
  if (n < 0 || (size_t)n >= sizeof(cmd)) {
    fprintf(stderr, "qllvm-compile command line too long\n");
    return -1;
  }

  fprintf(stderr, "[qllvm] %s\n", cmd);
  int rc = system(cmd);
  if (rc != 0) {
    fprintf(stderr, "qllvm-compile failed, exit status %d\n", rc);
    return -1;
  }
  return 0;
}

int main(int argc, char **argv)
{
    MPIQ_Comm comm = MPIQ_Init(&argc, &argv, NULL, NULL, NULL);

    const char *src_qasm = "../qasm/bell.qasm";
    char compiled_qasm[PATH_MAX];

    if (compile_qasm_with_qllvm(src_qasm, compiled_qasm, sizeof(compiled_qasm)) != 0) {
        return EXIT_FAILURE;
    }

    char ***qubit;
    char ***result;
    int qubit_send_count = 0, qubit_recv_count = 0;
    int *arry_send_counts = NULL;
    int *arry_recv_counts = NULL;
    QubitConfig ***ip_card_configs = NULL;
    int **card_count;

    QubitConfig ***icc = parse_qubit_configs_v2("../conf/config.json", ip_card_configs, &card_count);

    qubit = qasm_to_pulse_waveforms(&qubit_send_count, &arry_send_counts, compiled_qasm);

    MPIQ_Send(qubit_send_count, arry_send_counts, qubit, icc[0][1][0].ip, icc[0][1][0].card_id, comm);

    for (int i = 0; i < qubit_send_count; i++) {
        for (int j = 0; j < arry_send_counts[i]; j++) {
            free(qubit[i][j]);
            qubit[i][j] = NULL;
        }
        free(qubit[i]);
        qubit[i] = NULL;
    }
    free(qubit);
    free(arry_send_counts);

    result = MPIQ_Recv("127.0.0.1", 1, &qubit_recv_count, &arry_recv_counts, comm);

    for (int i = 0; i < qubit_recv_count; i++) {
        for (int j = 0; j < arry_recv_counts[i]; j++) {
            free(result[i][j]);
            result[i][j] = NULL;
        }
        free(result[i]);
        result[i] = NULL;
    }
    free(result);
    result = NULL;

    return 0;
}