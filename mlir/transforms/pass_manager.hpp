/*******************************************************************************
 * Copyright (c) 2018-, UT-Battelle, LLC.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the MIT License 
 * which accompanies this distribution. 
 *
 * Contributors:
 *   Alexander J. McCaskey - initial API and implementation
 *   Thien Nguyen - implementation
 *******************************************************************************/
#pragma once
#include "mlir/Dialect/Affine/Passes.h"
#include "optimizations/IdentityPairRemovalPass.hpp"
#include "optimizations/RemoveUnusedQIRCallsPass.hpp"
#include "optimizations/SimplifyQubitExtractPass.hpp"
#include "optimizations/SingleQubitGateMergingPass.hpp"
#include "optimizations/ModifierBlockInliner.hpp"
#include "quantum_to_llvm.hpp"
#include "lowering/ModifierRegionLowering.hpp"
#include "optimizations/circuitState.hpp"
#include "optimizations/circuitState2.hpp"
#include "optimizations/remove_diagonal_gates_before_measure.hpp"
#include "optimizations/CommutativeCancellationPass.hpp"
#include "optimizations/ConsolidateBlocks.hpp"
#include "optimizations/RemoveIdentityEquivalent.hpp"
#include "optimizations/trans_basicgate.hpp"
#include "optimizations/DecomposemultiPass.hpp"
#include "optimizations/Merge_u3_gate.hpp"
#include "optimizations/trans_u3_ro_rphi.hpp"
#include "optimizations/gen_qasm.hpp"
#include "optimizations/Check_gate.hpp"

#include <map>
#include <unordered_map>
#include <iostream>
#include<tr1/unordered_map>
#include <unordered_set> 

#define STR(input) #input

#define CONSOLIDATEBLOCKS 1

#define COMMUTATIVECANCELLATIONPASS 2


#define SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS 3
#define CNOTIDENTITYPAIRREMOVALPASS 4
#define DUPLICATERESETREMOVALPASS 5
#define SINGLE_QUBIT_GATE_MERGING_PASS 6
#define REMOVEIDENTITYEQUIVALENT 0

#define REMOVE_DIAGONAL_GATES_BEFORE_MEASURE 7
#define REMOVE_UNUSED_QIR_CALLS_PASS 8

#define CIRUIT_STATE1 9
#define CIRUIT_STATE2 10
#define TRANSBASICGATE 11


// Construct qllvm MLIR pass manager:
// Make sure we use the same set of passes and configs
// across different use cases of MLIR compilation.
namespace qllvm {

/// Options for pass manager configuration (Options-based API)
struct PassManagerOptions {
  std::string inputFile;  // for RemoveUnusedQIRCallsPass
  std::unordered_set<std::string> basicGateSet;
  std::map<std::string, bool> boolArgs;
  std::vector<int> customPassSequence;  // when non-empty, use instead of default sequence
};

/// Output stats from pass runs (pass_count, pass_effect)
struct PassManagerStats {
  static constexpr int N = 20;
  int opt_count[N] = {0};
  int opt_depth[N] = {0};
  int zero_count[N] = {0};
  int enable[N] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  int passes_count[N] = {0};
};

/// Configure passes for removing unused IR (inliner, loop unroll, SimplifyQubitExtract, RemoveUnusedQIRCalls)
void configureRemoveUnusedPasses(mlir::PassManager &passManager, const std::string &inputFile);

/// Configure optimization passes. Unified Options-based API.
/// @param opts options; if opts.customPassSequence is non-empty, use it; else if basicGateSet non-empty use basicgate path; else default
/// @param outStats optional; if non-null, populated for pass_count/pass_effect
void configureOptimizationPasses(mlir::PassManager &passManager,
                                 const PassManagerOptions &opts,
                                 PassManagerStats *outStats = nullptr);

// --- configureRemoveUnusedPasses implementation ---
inline void configureRemoveUnusedPasses(mlir::PassManager &passManager, const std::string &inputFile) {
  passManager.addPass(mlir::createInlinerPass());
  passManager.addPass(std::make_unique<ModifierBlockInlinerPass>());
  auto loop_unroller = mlir::createLoopUnrollPass(/*unrollFactor*/-1, /*unrollUpToFactor*/ false, /*unrollFull*/true);
  // Nest a pass manager that operates on functions within the one which
  // operates on ModuleOp.
  OpPassManager &nestedFunctionPM = passManager.nest<mlir::FuncOp>();
  nestedFunctionPM.addPass(std::move(loop_unroller));
  passManager.addPass(mlir::createInlinerPass());

  passManager.addPass(std::make_unique<SimplifyQubitExtractPass>());
  passManager.addPass(std::make_unique<RemoveUnusedQIRCallsPass>(inputFile));
}

inline void configureOptimizationPasses(mlir::PassManager &passManager) {
  configureOptimizationPasses(passManager, PassManagerOptions{}, nullptr);
}

inline void configureOptimizationPasses(mlir::PassManager &passManager,
                                        std::unordered_set<std::string> basic_gate) {
  PassManagerOptions opts;
  opts.basicGateSet = std::move(basic_gate);
  configureOptimizationPasses(passManager, opts, nullptr);
}

inline void configureOptimizationPasses(mlir::PassManager &passManager,
                                         const PassManagerOptions &opts,
                                         PassManagerStats *outStats) {
  constexpr int N_REPS = 4;
  int *opt_count = outStats ? outStats->opt_count : nullptr;
  int *opt_depth = outStats ? outStats->opt_depth : nullptr;
  int *zero_count = outStats ? outStats->zero_count : nullptr;
  int *enable = outStats ? outStats->enable : nullptr;
  int *passes_count = outStats ? outStats->passes_count : nullptr;
  int local_opt_count[PassManagerStats::N] = {0};
  int local_opt_depth[PassManagerStats::N] = {0};
  int local_zero_count[PassManagerStats::N] = {0};
  int local_enable[PassManagerStats::N] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  int local_passes_count[PassManagerStats::N] = {0};
  if (!opt_count) opt_count = local_opt_count;
  if (!opt_depth) opt_depth = local_opt_depth;
  if (!zero_count) zero_count = local_zero_count;
  if (!enable) enable = local_enable;
  if (!passes_count) passes_count = local_passes_count;

  const auto &bool_args = opts.boolArgs;
  const std::string &fn = opts.inputFile;

  if (!opts.customPassSequence.empty()) {
    std::unordered_map<std::string, int> pass_count;
    std::vector<std::string> passNames = {"REMOVEIDENTITYEQUIVALENT", "CONSOLIDATEBLOCKS", "COMMUTATIVECANCELLATIONPASS",
        "REMOVE_DIAGONAL_GATES_BEFORE_MEASURE", "SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS", "CNOTIDENTITYPAIRREMOVALPASS",
        "DUPLICATERESETREMOVALPASS", "SINGLE_QUBIT_GATE_MERGING_PASS", "REMOVE_UNUSED_QIR_CALLS_PASS", "CIRUIT_STATE", "TRANSBASICGATE"};
    for (const auto &p : passNames) pass_count.emplace(p, 0);

    for (size_t i = 0; i < opts.customPassSequence.size(); i++) {
      int PASS = opts.customPassSequence[i];
      switch (PASS) {
        case REMOVEIDENTITYEQUIVALENT:
          pass_count["REMOVEIDENTITYEQUIVALENT"]++;
          passManager.addPass(std::make_unique<RemoveIdentityEquivalent>(bool_args, opt_count[REMOVEIDENTITYEQUIVALENT], opt_depth[REMOVEIDENTITYEQUIVALENT], opt_depth[0], zero_count[REMOVEIDENTITYEQUIVALENT], enable[REMOVEIDENTITYEQUIVALENT], passes_count[REMOVEIDENTITYEQUIVALENT]));
          break;
        case REMOVE_DIAGONAL_GATES_BEFORE_MEASURE:
          pass_count["REMOVE_DIAGONAL_GATES_BEFORE_MEASURE"]++;
          passManager.addPass(std::make_unique<remove_diagonal_gates_before_measure>(bool_args, opt_count[REMOVE_DIAGONAL_GATES_BEFORE_MEASURE], opt_depth[REMOVE_DIAGONAL_GATES_BEFORE_MEASURE], opt_depth[0], zero_count[REMOVE_DIAGONAL_GATES_BEFORE_MEASURE], enable[REMOVE_DIAGONAL_GATES_BEFORE_MEASURE], passes_count[REMOVE_DIAGONAL_GATES_BEFORE_MEASURE]));
          break;
        case CONSOLIDATEBLOCKS:
          pass_count["CONSOLIDATEBLOCKS"]++;
          passManager.addPass(std::make_unique<ConsolidateBlocks>(bool_args, opt_count[CONSOLIDATEBLOCKS], opt_depth[CONSOLIDATEBLOCKS], opt_depth[0], zero_count[CONSOLIDATEBLOCKS], enable[CONSOLIDATEBLOCKS], passes_count[CONSOLIDATEBLOCKS]));
          break;
        case COMMUTATIVECANCELLATIONPASS:
          pass_count["COMMUTATIVECANCELLATIONPASS"]++;
          passManager.addPass(std::make_unique<CommutativeCancellationPass>(bool_args, opt_count[COMMUTATIVECANCELLATIONPASS], opt_depth[COMMUTATIVECANCELLATIONPASS], opt_depth[0], zero_count[COMMUTATIVECANCELLATIONPASS], enable[COMMUTATIVECANCELLATIONPASS], passes_count[COMMUTATIVECANCELLATIONPASS]));
          break;
        case SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS:
          pass_count["SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS"]++;
          passManager.addPass(std::make_unique<SingleQubitIdentityPairRemovalPass>(bool_args, opt_count[SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS], opt_depth[SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS], opt_depth[0], zero_count[SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS], enable[SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS], passes_count[SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS]));
          break;
        case CNOTIDENTITYPAIRREMOVALPASS:
          pass_count["CNOTIDENTITYPAIRREMOVALPASS"]++;
          passManager.addPass(std::make_unique<CNOTIdentityPairRemovalPass>(bool_args, opt_count[CNOTIDENTITYPAIRREMOVALPASS], opt_depth[CNOTIDENTITYPAIRREMOVALPASS], opt_depth[0], zero_count[CNOTIDENTITYPAIRREMOVALPASS], enable[CNOTIDENTITYPAIRREMOVALPASS], passes_count[CNOTIDENTITYPAIRREMOVALPASS]));
          break;
        case DUPLICATERESETREMOVALPASS:
          pass_count["DUPLICATERESETREMOVALPASS"]++;
          passManager.addPass(std::make_unique<DuplicateResetRemovalPass>(bool_args, opt_count[DUPLICATERESETREMOVALPASS], opt_depth[DUPLICATERESETREMOVALPASS], opt_depth[0], zero_count[DUPLICATERESETREMOVALPASS], enable[DUPLICATERESETREMOVALPASS], passes_count[DUPLICATERESETREMOVALPASS]));
          break;
        case SINGLE_QUBIT_GATE_MERGING_PASS:
          pass_count["SINGLE_QUBIT_GATE_MERGING_PASS"]++;
          passManager.addPass(std::make_unique<SingleQubitGateMergingPass>(bool_args, opt_count[SINGLE_QUBIT_GATE_MERGING_PASS], opt_depth[SINGLE_QUBIT_GATE_MERGING_PASS], opt_depth[0], zero_count[SINGLE_QUBIT_GATE_MERGING_PASS], enable[SINGLE_QUBIT_GATE_MERGING_PASS], passes_count[SINGLE_QUBIT_GATE_MERGING_PASS]));
          break;
        case REMOVE_UNUSED_QIR_CALLS_PASS:
          pass_count["REMOVE_UNUSED_QIR_CALLS_PASS"]++;
          passManager.addPass(std::make_unique<RemoveUnusedQIRCallsPass>(bool_args, passes_count[REMOVE_UNUSED_QIR_CALLS_PASS], opt_depth[REMOVE_UNUSED_QIR_CALLS_PASS], opt_depth[0], fn));
          break;
        case CIRUIT_STATE1:
          pass_count["CIRUIT_STATE"]++;
          passManager.addPass(std::make_unique<circuitState>(bool_args, passes_count[CIRUIT_STATE1]));
          break;
        case CIRUIT_STATE2:
          pass_count["CIRUIT_STATE"]++;
          passManager.addPass(std::make_unique<circuitState2>(bool_args, passes_count[CIRUIT_STATE2]));
          break;
        default:
          break;
      }
    }
    return;
  }

  if (!opts.basicGateSet.empty()) {
    const auto &basic_gate = opts.basicGateSet;
    // passManager.addPass(std::make_unique<DecomposemultiPass>());
    for (int i = 0; i < N_REPS; ++i) {
      passManager.addPass(std::make_unique<RemoveIdentityEquivalent>());
      passManager.addPass(std::make_unique<SingleQubitIdentityPairRemovalPass>());
      passManager.addPass(std::make_unique<CNOTIdentityPairRemovalPass>());
      passManager.addPass(std::make_unique<DuplicateResetRemovalPass>());
      passManager.addPass(std::make_unique<remove_diagonal_gates_before_measure>());
      passManager.addPass(std::make_unique<CommutativeCancellationPass>());
      passManager.addPass(std::make_unique<RemoveIdentityEquivalent>());
      passManager.addPass(std::make_unique<ConsolidateBlocks>(basic_gate));
      passManager.addPass(std::make_unique<trans_basicgate>(basic_gate));
      passManager.addPass(std::make_unique<SingleQubitGateMergingPass>(basic_gate));
    }
    
  } else {
    // passManager.addPass(std::make_unique<DecomposemultiPass>());
    for (int i = 0; i < N_REPS; ++i) {
      passManager.addPass(std::make_unique<RemoveIdentityEquivalent>());
      passManager.addPass(std::make_unique<SingleQubitIdentityPairRemovalPass>());
      passManager.addPass(std::make_unique<CNOTIdentityPairRemovalPass>());
      passManager.addPass(std::make_unique<DuplicateResetRemovalPass>());
      passManager.addPass(std::make_unique<remove_diagonal_gates_before_measure>());
      passManager.addPass(std::make_unique<CommutativeCancellationPass>());
      passManager.addPass(std::make_unique<RemoveIdentityEquivalent>());
      passManager.addPass(std::make_unique<ConsolidateBlocks>());
      passManager.addPass(std::make_unique<SingleQubitGateMergingPass>());
    }
  }
  passManager.addPass(std::make_unique<RemoveUnusedQIRCallsPass>());
}
} // namespace qllvm
