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
 *
 * Modified by QCFlow (2026) for QLLVM project.
 */

/*******************************************************************************
 * QIR Runtime Stub for QASM-only hybrid compilation
 * Minimal no-op implementations to satisfy the linker for hybrid LLVM workflows.
 * For full QIR runtime behavior, link a complete QIR runtime library instead of this stub.
 *******************************************************************************/
#include <cstdint>
#include <cstdlib>
#include <iostream>

struct Qubit { uint64_t id; };
struct Result { bool val; };
struct Array { void* data; size_t size; };
using TuplePtr = int8_t*;
using Callable = void*;
using QirString = char*;
enum Pauli { PauliI, PauliX, PauliY, PauliZ };

static Result resultZero{false};
static Result resultOne{true};
Result* ResultZero = &resultZero;
Result* ResultOne = &resultOne;

extern "C" {

void __quantum__rt__set_config_parameter(int8_t*, int8_t*) {}
void __quantum__rt__initialize(int, int8_t**) {}
void __quantum__rt__finalize() {}
void __quantum__rt__set_external_qreg(void*) {}

void __quantum__qis__swap(Qubit*, Qubit*) {}
void __quantum__qis__cz(Qubit*, Qubit*) {}
void __quantum__qis__cnot(Qubit*, Qubit*) {}
void __quantum__qis__cp(double, Qubit*, Qubit*) {}
void __quantum__qis__cphase(double, Qubit*, Qubit*) {}
void __quantum__qis__h(Qubit*) {}
void __quantum__qis__s(Qubit*) {}
void __quantum__qis__sdg(Qubit*) {}
void __quantum__qis__t(Qubit*) {}
void __quantum__qis__tdg(Qubit*) {}
void __quantum__qis__reset(Qubit*) {}
void __quantum__qis__x(Qubit*) {}
void __quantum__qis__y(Qubit*) {}
void __quantum__qis__z(Qubit*) {}
void __quantum__qis__sx(Qubit*) {}
void __quantum__qis__rx(double, Qubit*) {}
void __quantum__qis__ry(double, Qubit*) {}
void __quantum__qis__rz(double, Qubit*) {}
void __quantum__qis__p(double, Qubit*) {}
void __quantum__qis__u3(double, double, double, Qubit*) {}
void __quantum__qis__igate(Qubit*) {}

Result* __quantum__qis__mz(Qubit*) { return ResultZero; }
bool __quantum__rt__result_equal(Result*, Result*) { return false; }
void __quantum__rt__result_update_reference_count(Result*, int32_t) {}
Result* __quantum__rt__result_get_one() { return ResultOne; }
Result* __quantum__rt__result_get_zero() { return ResultZero; }

Array* __quantum__rt__qubit_allocate_array(uint64_t n) {
  Array* a = (Array*)malloc(sizeof(Array));
  a->size = n * sizeof(Qubit);
  a->data = calloc(n, sizeof(Qubit));
  return a;
}
void __quantum__rt__qubit_release_array(Array* q) {
  if (q && q->data) free(q->data);
  if (q) free(q);
}
void __quantum__rt__qubit_release(Qubit*) {}
Qubit* __quantum__rt__qubit_allocate() {
  static Qubit q{0};
  return &q;
}

void __quantum__rt__start_ctrl_u_region() {}
void __quantum__rt__end_ctrl_u_region(Qubit*) {}
void __quantum__rt__end_multi_ctrl_u_region(void*) {}
void __quantum__rt__start_adj_u_region() {}
void __quantum__rt__end_adj_u_region() {}
void __quantum__rt__start_pow_u_region() {}
void __quantum__rt__end_pow_u_region(int64_t) {}
void __quantum__rt__mark_compute() {}
void __quantum__rt__unmark_compute() {}

Array* __quantum__rt__array_create_1d(int32_t, int64_t c) {
  Array* a = (Array*)malloc(sizeof(Array));
  a->size = (size_t)c;
  a->data = calloc((size_t)c, 1);
  return a;
}
int64_t __quantum__rt__array_get_size_1d(Array* a) { return a ? (int64_t)a->size : 0; }
int8_t* __quantum__rt__array_get_element_ptr_1d(Array* q, uint64_t idx) {
  return q && q->data ? (int8_t*)q->data + idx : nullptr;
}
Array* __quantum__rt__array_copy(Array*, bool) { return nullptr; }
Array* __quantum__rt__array_concatenate(Array*, Array*) { return nullptr; }
Array* __quantum__rt__array_slice(Array*, int32_t, int64_t, int64_t, int64_t) { return nullptr; }
Array* __quantum__rt__array_slice_1d(Array*, int64_t, int64_t, int64_t) { return nullptr; }
void __quantum__rt__array_update_alias_count(void*, int32_t) {}
void __quantum__rt__array_update_reference_count(void*, int32_t) {}
int32_t __quantum__rt__array_get_dim(void*) { return 1; }
int64_t __quantum__rt__array_get_size(void*, int32_t) { return 0; }
Array* __quantum__rt__array_create_nonvariadic(int, int, int64_t) { return nullptr; }
Array* __quantum__rt__array_create(int, int, int64_t*) { return nullptr; }
int8_t* __quantum__rt__array_get_element_ptr_nonvariadic(void*, int64_t*) { return nullptr; }
int8_t* __quantum__rt__array_get_element_ptr(void*, ...) { return nullptr; }
void* __quantum__rt__array_project(void*, int, int64_t) { return nullptr; }

void __quantum__rt__string_update_reference_count(QirString*, int32_t) {}
QirString __quantum__rt__string_create(char*) { return nullptr; }
QirString __quantum__rt__string_concatenate(QirString, QirString) { return nullptr; }
bool __quantum__rt__string_equal(QirString, QirString) { return false; }
QirString __quantum__rt__int_to_string(int64_t) { return nullptr; }
QirString __quantum__rt__double_to_string(double) { return nullptr; }
QirString __quantum__rt__bool_to_string(bool) { return nullptr; }
QirString __quantum__rt__result_to_string(Result*) { return nullptr; }
QirString __quantum__rt__pauli_to_string(Pauli) { return nullptr; }
QirString __quantum__rt__qubit_to_string(Qubit*) { return nullptr; }
QirString __quantum__rt__range_to_string(int64_t, int64_t, int64_t) { return nullptr; }
const char* __quantum__rt__string_get_data(QirString) { return ""; }
int32_t __quantum__rt__string_get_length(QirString) { return 0; }

TuplePtr __quantum__rt__tuple_create(int64_t) { return nullptr; }
void __quantum__rt__tuple_update_reference_count(int8_t*, int32_t) {}
void __quantum__rt__tuple_update_alias_count(int8_t*, int32_t) {}

void __quantum__rt__callable_update_reference_count(Callable, int32_t) {}
void __quantum__rt__callable_update_alias_count(Callable, int32_t) {}
void __quantum__rt__callable_invoke(Callable, TuplePtr, TuplePtr) {}
Callable __quantum__rt__callable_copy(Callable, bool) { return nullptr; }
void __quantum__rt__capture_update_reference_count(Callable, int32_t) {}
void __quantum__rt__capture_update_alias_count(Callable, int32_t) {}
void __quantum__rt__callable_memory_management(int32_t, Callable, bool) {}
void __quantum__rt__callable_make_adjoint(Callable) {}
void __quantum__rt__callable_make_controlled(Callable) {}
Callable __quantum__rt__callable_create(void* /*ft*/, int, ...) { return nullptr; }

void __quantum__rt__fail(QirString) {}
void __quantum__rt__message(QirString) {}

void __quantum__qis__applyifelseintrinsic__body(Result*, void*, void*) {}
void __quantum__qis__applyconditionallyintrinsic__body(Array*, Array*, void*, void*) {}

}  // extern "C"
