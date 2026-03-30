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
#pragma once
#include <string>
#include <utility>
#include <vector>
#include <Eigen/Dense>
#include <map>
#include <unordered_set> 

namespace qllvm {
namespace utils {
using pauli_decomp_t = std::pair<std::string, double>;
// Input list of quantum gates: name and list of params (e.g. 1 for rx, ry, rz;
// 3 for u3, etc.)
using qop_t = std::pair<std::string, std::vector<double>>;
// std::vector<pauli_decomp_t> decompose_gate_sequence(const std::vector<qop_t> &op_list);
// Enum for Euler basis types
enum class EulerBasis {
    U3 = 0,
    U321 = 1,
    U = 2,
    PSX = 3,
    U1X = 4,
    RR = 5,
    ZYZ = 6,
    ZXZ = 7,
    XZX = 8,
    XYX = 9,
    ZSXX = 10,
    ZSX = 11
};

// Enum for standard gates
enum class StandardGate {
    X,     // Pauli-X
    SX,    // sqrt(X) gate
    RX,    // Rotation around X
    RY,    // Rotation around Y
    RZ,    // Rotation around Z
    U1,    // Phase gate
    U2,    // U2 gate
    U3,    // General single-qubit gate
    U,     // Same as U3 in Qiskit
    Phase, // Phase gate
    R      // General rotation
};

// Structure to represent a gate with its parameters
struct Gate {
    StandardGate type;
    std::vector<double> params;
};

// One qubit gate sequence class
class OneQubitGateSequence {
public:
    std::vector<Gate> gates;
    double global_phase = 0.0;
};

// Error map for one qubit gates
class OneQubitGateErrorMap {
public:
    OneQubitGateErrorMap(int num_qubits = 0) {
        if (num_qubits > 0) {
            error_map.resize(num_qubits);
        }
    }
    
    void addQubit(const std::map<std::string, double>& qubit_error_map) {
        error_map.push_back(qubit_error_map);
    }
    
    std::vector<std::map<std::string, double>> error_map;
};

// Class to keep track of supported Euler bases
class EulerBasisSet {
public:
    EulerBasisSet() : basis(12, false), initialized(false) {}
    
    void addBasis(EulerBasis basis_type) {
        basis[static_cast<int>(basis_type)] = true;
        initialized = true;
    }
    
    std::vector<EulerBasis> getBases() const {
        std::vector<EulerBasis> result;
        for (size_t i = 0; i < basis.size(); ++i) {
            if (basis[i]) {
                result.push_back(static_cast<EulerBasis>(i));
            }
        }
        return result;
    }
    
    std::vector<bool> basis;
    bool initialized;
};

std::vector<qop_t> OneQubitGateSequence_to_qop_t(OneQubitGateSequence decompose_result);
std::vector<qop_t> new_euler_decompose(const std::vector<qop_t> &op_list,std::unordered_set<std::string> basic_gate_set);
OneQubitGateSequence decomposeunitarytobasis(const Eigen::Matrix2cd& unitary,EulerBasis basis);
EulerBasis euler_Basis_FromStr(const std::string& name);
std::vector<double> paramszyzinner(const Eigen::Matrix2cd& mat);
std::vector<double> paramsxyxinner(const Eigen::Matrix2cd& mat);
std::vector<qop_t> leastcost_basis(const Eigen::Matrix2cd& unitary,const std::vector<EulerBasis>& target_basis_set);

} // namespace utils
} // namespace qllvm