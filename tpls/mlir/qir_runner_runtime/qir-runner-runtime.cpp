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
 * QIR Runner Runtime - invoke qir-runner as subprocess for hybrid simulation
 * Links with hybrid executables when -qpu qir-runner; kernel calls spawn qir-runner.
 * Parses qir-runner output, stores histogram for C++ and prints Qiskit-style counts.
 *******************************************************************************/
#include "qir-runner-runtime.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#include <process.h>
#define popen _popen
#define pclose _pclose
#else
#include <libgen.h>
#include <unistd.h>
#endif

extern "C" {

void __quantum__rt__set_config_parameter(int8_t*, int8_t*) {}
void __quantum__rt__initialize(int argc, int8_t** argv);
void __quantum__rt__finalize() {}
void __quantum__rt__set_external_qreg(void*) {}

}  // extern "C"

namespace {

static int g_shots = 1024;
static std::string g_exe_dir;
static std::vector<std::pair<std::string, int>> g_last_histogram;

void set_exe_dir_from_argv0(const char* argv0) {
  if (!argv0 || !*argv0) {
    g_exe_dir = "./";
    return;
  }
#ifdef __linux__
  char buf[4096];
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n > 0) {
    buf[n] = '\0';
    char tmp[4096];
    strncpy(tmp, buf, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    char* d = dirname(tmp);
    g_exe_dir = std::string(d) + "/";
    return;
  }
#endif
  std::string s(argv0);
  size_t p = s.find_last_of("/\\");
  g_exe_dir = (p != std::string::npos) ? s.substr(0, p + 1) : "./";
}

/// Parse qir-runner output (START/RESULT/END) into shots, then aggregate counts.
void parse_and_print_counts(const std::string& output) {
  std::vector<std::vector<int>> shots;
  std::vector<int> results;
  std::istringstream iss(output);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.find("START") == 0) {
      results.clear();
    } else if (line.find("RESULT") == 0) {
      size_t pos = line.find('\t');
      if (pos == std::string::npos) pos = line.find(' ');
      if (pos != std::string::npos) {
        int val = std::atoi(line.c_str() + pos + 1);
        results.push_back(val);
      }
    } else if (line.find("END") == 0 && !results.empty()) {
      shots.push_back(results);
    }
  }
  std::map<std::string, int> counts;
  for (const auto& shot : shots) {
    std::string bits;
    for (int b : shot) bits += (char)('0' + b);
    counts[bits]++;
  }
  g_last_histogram.clear();
  g_last_histogram.reserve(counts.size());
  for (const auto& p : counts) g_last_histogram.push_back(p);

  std::cout << "{";
  bool first = true;
  for (const auto& p : counts) {
    if (!first) std::cout << ", ";
    std::cout << "'" << p.first << "': " << p.second;
    first = false;
  }
  std::cout << "}" << std::endl;
}

}  // namespace

extern "C" {

void __quantum__rt__initialize(int argc, int8_t** argv) {
  const char* exe_path = nullptr;
  for (int i = 0; i < argc && argv; i++) {
    if (i == 0 && argv[i]) exe_path = (const char*)argv[i];
    if (argv[i] && strcmp((const char*)argv[i], "-shots") == 0 && i + 1 < argc && argv[i + 1]) {
      g_shots = atoi((const char*)argv[i + 1]);
      if (g_shots <= 0) g_shots = 1024;
    }
  }
  set_exe_dir_from_argv0(exe_path ? exe_path : "");
}

void qir_runner_set_shots(int shots) {
  g_shots = (shots > 0) ? shots : 1024;
}

void qir_runner_set_exe_dir(const char* dir) {
  if (dir && *dir) g_exe_dir = std::string(dir);
}

void qir_runner_execute(const char* kernel_name, int shots) {
  if (!kernel_name || !*kernel_name) return;
  g_last_histogram.clear();
  int s = (shots > 0) ? shots : g_shots;

  std::string bc_path = g_exe_dir + std::string(kernel_name) + ".bc";
  std::string cmd = "qir-runner -f ";
  cmd += bc_path;
  cmd += " -s ";
  cmd += std::to_string(s);

  FILE* fp = popen(cmd.c_str(), "r");
  if (!fp) {
    std::cerr << "[qir-runner] Failed to run: " << cmd << std::endl;
    return;
  }
  std::string output;
  char buf[256];
  while (fgets(buf, sizeof(buf), fp) != nullptr)
    output += buf;
  pclose(fp);

  parse_and_print_counts(output);
}

int qir_runner_last_histogram_size(void) {
  return static_cast<int>(g_last_histogram.size());
}

int qir_runner_last_histogram_get(int index, char* bitstring_out, size_t bitstring_cap,
                                    int* count_out) {
  if (!bitstring_out || bitstring_cap == 0 || !count_out) return -1;
  if (index < 0 || index >= static_cast<int>(g_last_histogram.size())) return -1;
  const std::string& bits = g_last_histogram[static_cast<size_t>(index)].first;
  if (bits.size() + 1 > bitstring_cap) return -1;
  std::memcpy(bitstring_out, bits.c_str(), bits.size() + 1);
  *count_out = g_last_histogram[static_cast<size_t>(index)].second;
  return 0;
}

}  // extern "C"
