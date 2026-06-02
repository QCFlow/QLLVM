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
#include "qllvm-mpi-runtime.h"

#include "qir-runner-runtime.h"

#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr int kMaxBitstringLen = 64;

void pack_histogram(std::vector<char> &buf) {
  const int n = qir_runner_last_histogram_size();
  buf.clear();
  auto append_int = [&buf](int v) {
    buf.insert(buf.end(), reinterpret_cast<const char *>(&v),
               reinterpret_cast<const char *>(&v) + sizeof(int));
  };

  append_int(n);
  for (int i = 0; i < n; ++i) {
    char bits[kMaxBitstringLen];
    int count = 0;
    if (qir_runner_last_histogram_get(i, bits, sizeof(bits), &count) != 0) {
      append_int(0);
      append_int(0);
      continue;
    }
    const int len = static_cast<int>(std::strlen(bits));
    append_int(len);
    append_int(count);
    buf.insert(buf.end(), bits, bits + len + 1);
  }
}

void unpack_histogram(const std::vector<char> &buf) {
  if (buf.size() < sizeof(int)) {
    qir_runner_load_histogram(0, nullptr, nullptr);
    return;
  }

  size_t off = 0;
  auto read_int = [&buf, &off](int &v) -> bool {
    if (off + sizeof(int) > buf.size()) return false;
    std::memcpy(&v, buf.data() + off, sizeof(int));
    off += sizeof(int);
    return true;
  };

  int n = 0;
  if (!read_int(n) || n < 0) {
    qir_runner_load_histogram(0, nullptr, nullptr);
    return;
  }

  std::vector<std::string> bitstrings;
  std::vector<int> counts;
  bitstrings.reserve(static_cast<size_t>(n));
  counts.reserve(static_cast<size_t>(n));

  for (int i = 0; i < n; ++i) {
    int len = 0;
    int count = 0;
    if (!read_int(len) || !read_int(count) || len < 0 || len >= kMaxBitstringLen) {
      qir_runner_load_histogram(0, nullptr, nullptr);
      return;
    }
    if (off + static_cast<size_t>(len) + 1 > buf.size()) {
      qir_runner_load_histogram(0, nullptr, nullptr);
      return;
    }
    bitstrings.emplace_back(buf.data() + off, static_cast<size_t>(len));
    off += static_cast<size_t>(len) + 1;
    counts.push_back(count);
  }

  std::vector<const char *> ptrs;
  ptrs.reserve(bitstrings.size());
  for (const auto &s : bitstrings) ptrs.push_back(s.c_str());
  qir_runner_load_histogram(static_cast<int>(bitstrings.size()),
                            ptrs.empty() ? nullptr : ptrs.data(),
                            counts.empty() ? nullptr : counts.data());
}

}  // namespace

extern "C" {

void qllvm_mpi_bcast_histogram(MPI_Comm comm, int root) {
  int rank = 0;
  MPI_Comm_rank(comm, &rank);

  std::vector<char> packed;
  int packed_size = 0;

  if (rank == root) {
    pack_histogram(packed);
    packed_size = static_cast<int>(packed.size());
  }

  MPI_Bcast(&packed_size, 1, MPI_INT, root, comm);

  if (rank != root) packed.resize(static_cast<size_t>(packed_size));

  if (packed_size > 0) {
    MPI_Bcast(packed.data(), packed_size, MPI_BYTE, root, comm);
  }

  if (rank != root) unpack_histogram(packed);
}

void qllvm_mpi_execute_kernel(const char *kernel_name, int shots) {
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) qir_runner_execute(kernel_name, shots);

  qllvm_mpi_bcast_histogram(MPI_COMM_WORLD, 0);
}

void qllvm_mpi_execute_kernel_local(const char *kernel_name, int shots) {
  qir_runner_execute(kernel_name, shots);
}

}  // extern "C"
