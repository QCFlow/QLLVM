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
#include "gate_matrix.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wunused-function"
#include <Eigen/Dense>
#pragma GCC diagnostic pop
#include <cassert>
#include <iostream>
#include <unordered_map>
#include "get_matrix.hpp"
const double ANGLE_ZERO_EPSILON = 1e-12;

namespace {
using namespace qllvm::utils;
using namespace std::complex_literals;

double mod_2pi(double angle, double atol) {
    double wrapped = std::fmod(angle + M_PI, 2 * M_PI) - M_PI;
    if (std::abs(wrapped - M_PI) < atol) {
        return -M_PI;
    }
    return wrapped;
}

// Compute ZYZ Euler angles from unitary matrix
std::vector<double> params_zyz_inner(const Eigen::Matrix2cd& mat) {
    using namespace std::complex_literals;
    auto det = mat.determinant();
    auto det_arg = std::arg(det);
    auto phase = det_arg / 2.0;
    auto theta = 2.0 * std::atan2(std::abs(mat(1,0)), std::abs(mat(0,0)));
    auto ang1 = std::arg(mat(1,1));
    auto ang2 = std::arg(mat(1,0));
    auto phi = ang1 + ang2 - det_arg;
    auto lam = ang1 - ang2;
    return {theta, phi, lam, phase};
}

// Compute XYX Euler angles from unitary matrix
std::vector<double> params_xyx_inner(const Eigen::Matrix2cd& mat) {
    using namespace std::complex_literals;

    Eigen::Matrix2cd mat_zyz = Eigen::MatrixXcd::Zero(2, 2);
    mat_zyz << 0.5*(mat(0,0) + mat(0,1) + mat(1,0) + mat(1,1)), 0.5*(mat(0,0) - mat(0,1) + mat(1,0) - mat(1,1)),
               0.5*(mat(0,0) + mat(0,1) - mat(1,0) - mat(1,1)), 0.5*(mat(0,0) - mat(0,1) - mat(1,0) + mat(1,1));
    std::vector<double> params = params_zyz_inner(mat_zyz);
    auto theta = params[0];
    auto phi = params[1];
    auto lam = params[2];
    auto phase = params[3];
    auto nwe_phi = mod_2pi(phi + M_PI, 0.0);
    auto nwe_lam = mod_2pi(lam + M_PI, 0.0);
    return {theta, nwe_phi, nwe_lam, phase+(nwe_lam + nwe_phi - phi -lam)/2.0};
}

// Compute ZXZ Euler angles from unitary matrix
std::vector<double> params_zxz_inner(const Eigen::Matrix2cd& mat) {
    using namespace std::complex_literals;

    std::vector<double> params = params_zyz_inner(mat);
    return {params[0], params[1] + M_PI_2, params[2] - M_PI_2, params[3]};
}

// Compute XZX Euler angles from unitary matrix
std::vector<double> params_xzx_inner(const Eigen::Matrix2cd& mat) {
    using namespace std::complex_literals;

    auto det = mat.determinant();
    auto phase = std::arg(det) / 2.0;
    auto sqrt_det = std::sqrt(det);
    // std::cout << "det:  " << det << "phase: " << phase << "sqrt_det:    " << sqrt_det << std::endl;
    Eigen::Matrix2cd mat_zyz = Eigen::MatrixXcd::Zero(2, 2);
    mat_zyz << std::complex((mat(0,0) / sqrt_det).real(), (mat(1,0) / sqrt_det).imag()),std::complex((mat(1,0) / sqrt_det).real(), (mat(0,0) / sqrt_det).imag()),
               std::complex(-(mat(1,0) / sqrt_det).real(), (mat(0,0) / sqrt_det).imag()), std::complex((mat(0,0) / sqrt_det).real(), -(mat(1,0) / sqrt_det).imag());           

    std::vector<double> params = params_zxz_inner(mat_zyz);
    auto theta = params[0];
    auto phi = params[1];
    auto lam = params[2];
    auto phase_zxz = params[3];

    return {theta, phi, lam, phase + phase_zxz};
}

// Compute U3 parameters from unitary matrix
std::vector<double> params_u3_inner(const Eigen::Matrix2cd& mat) {
    std::vector<double> params = params_zyz_inner(mat);
    double theta = params[0], phi = params[1], lam = params[2], phase = params[3];
    
    return {theta, phi, lam, phase - 0.5 * (phi + lam)};
}

// Compute U1X parameters from unitary matrix
std::vector<double> params_u1x_inner(const Eigen::Matrix2cd& mat) {
    std::vector<double> params = params_zyz_inner(mat);
    double theta = params[0], phi = params[1], lam = params[2], phase = params[3];
    
    return {theta, phi, lam, phase - 0.5 * (theta + phi + lam)};
}

std::vector<double> angles_from_unitary(const Eigen::Matrix2cd& unitary, EulerBasis target_basis){
    switch(target_basis) {
        // case EulerBasis::U321:
        // case EulerBasis::U3:
        // case EulerBasis::U:
        //     return params_u3_inner(unitary);

        // case EulerBasis::PSX:
        case EulerBasis::ZSX:
        case EulerBasis::ZSXX:
            return params_u1x_inner(unitary);
        // case EulerBasis::U1X:
        //     return params_u1x_inner(unitary);

        case EulerBasis::ZYZ:
            return params_zyz_inner(unitary);

        case EulerBasis::ZXZ:
            return params_zxz_inner(unitary);

        case EulerBasis::XYX:
            return params_xyx_inner(unitary);
        
        case EulerBasis::XZX:
            return params_xzx_inner(unitary);
        
        default:
            throw std::invalid_argument("Unknown basis");
    }
}

OneQubitGateSequence circuit_kak(double theta, double phi, double lam, double phase, StandardGate k_gate,StandardGate a_gate,bool simplify = true, double atol = 1e-10) {
    OneQubitGateSequence circuit;
    circuit.global_phase = phase - (phi + lam) / 2.0;
    if(!simplify){
        atol = -1.0;
    }

    if(std::abs(theta) < atol){
        lam += phi;
        lam = mod_2pi(lam, atol);
        if(std::abs(lam) > atol){
            circuit.gates.push_back({k_gate, {lam}});
            circuit.global_phase += lam / 2.0;
        }
        return circuit;
    }

    if(std::abs(theta - M_PI) < atol){
        circuit.global_phase += phi;
        lam -= phi;
        phi = 0.0;
    }

    if((std::abs(mod_2pi(lam + M_PI, atol)) < atol) || (std::abs(mod_2pi(phi + M_PI, atol)) < atol)){
        lam += M_PI;
        theta = -theta;
        phi += M_PI;
    }

    lam = mod_2pi(lam,atol);

    if(std::abs(lam) > atol){
        circuit.global_phase += lam / 2.0;
        circuit.gates.push_back({k_gate, {lam}});
    }
    circuit.gates.push_back({a_gate, {theta}});

    phi = mod_2pi(phi,atol);
    if(std::abs(phi) > atol){
        circuit.global_phase += phi / 2.0;
        circuit.gates.push_back({k_gate, {phi}});
    }
    return circuit;
}

// Generate U3 circuit using ZYZ decomposition
OneQubitGateSequence circuit_u3(double theta, double phi, double lam, double phase) {
    
    OneQubitGateSequence circuit;
    circuit.global_phase = phase;
    circuit.gates.push_back({StandardGate::U3, {theta, phi, lam}});
    
    return circuit;
}

// Generate U321 circuit using combination of U2 and U1 gates
OneQubitGateSequence circuit_u321(double theta, double phi, double lam, double phase) {
    
    OneQubitGateSequence circuit;
    circuit.global_phase = phase;
    
    if (std::abs(theta) < ANGLE_ZERO_EPSILON) {
        circuit.gates.push_back({StandardGate::U1, {phi + lam}});
    }else if (std::abs(theta - M_PI_2) < ANGLE_ZERO_EPSILON) {
        circuit.gates.push_back({StandardGate::U2, {phi , lam}});
    }else {
        circuit.gates.push_back({StandardGate::U3, {theta, phi, lam}});
    }
    return circuit;
}

// Generate U circuit using U gate
OneQubitGateSequence circuit_u(double theta, double phi, double lam, double phase) {
    OneQubitGateSequence circuit;
    circuit.global_phase = phase;
    
    circuit.gates.push_back({StandardGate::U, {theta, phi, lam}});
    return circuit;
}

void fnz_phase(OneQubitGateSequence& circuit, double phi,double atol){
    phi = mod_2pi(phi, atol);
    if(std::abs(phi) > atol){
        circuit.gates.push_back({StandardGate::Phase, {phi}});
    }
    return;
}

void fnz_rz(OneQubitGateSequence &circuit, double phi,double atol){
    auto phi_ = mod_2pi(phi, atol);
    if(std::abs(phi_) > atol){
        // std::cout << "phi_: " << phi_ << std::endl;
        circuit.gates.push_back({StandardGate::RZ, {phi_}});
        circuit.global_phase += phi_ / 2.0;  
    }
    return;
}

void fnz_sx(OneQubitGateSequence& circuit,double phi = 0.0,double atol = 0.0){
    circuit.gates.push_back({StandardGate::SX, {}});
    return;
}

void fnz_x(OneQubitGateSequence& circuit,double phi = 0.0,double atol = 0.0){
    circuit.gates.push_back({StandardGate::X, {}});
    return;
}

OneQubitGateSequence circuit_psx_gen(double theta, double phi, double lam, double phase, bool simplify,
                                    double atol, void (*fnz)(OneQubitGateSequence&, double, double),
                                    void (*fnx)(OneQubitGateSequence&,double, double), std::function<void(OneQubitGateSequence&,double, double)> optional_fn = nullptr) {
    OneQubitGateSequence circuit;
    circuit.global_phase = phase;

    if (!simplify) {
        atol = -1.0;
    }

    if (std::abs(theta) < atol) {
        fnz(circuit, lam + phi, atol);
        return circuit;
    }

    if (std::abs(theta - M_PI_2) < atol) {
        fnz(circuit, lam - M_PI_2, atol);
        fnx(circuit,0.0,0.0);
        fnz(circuit, phi + M_PI_2, atol);
        return circuit;
    }

    if (std::abs(theta - M_PI) < atol) {
        circuit.global_phase += lam;
        phi -= lam;
        lam = 0.0;
    }

    if ((std::abs(mod_2pi(lam + M_PI, atol)) < atol) || (std::abs(mod_2pi(phi, atol)) < atol)) {
        lam += M_PI;
        theta = -theta;
        phi += M_PI;
        circuit.global_phase -= theta;
    }

    theta += M_PI;
    phi += M_PI;
    circuit.global_phase -= M_PI_2;
    fnz(circuit, lam, atol);

    if(optional_fn && (std::abs(mod_2pi(theta, atol)) < atol)){
        optional_fn(circuit,0.0,0.0);
    } else {
        fnx(circuit,0.0,0.0);
        fnz(circuit, theta, atol);
        fnx(circuit,0.0,0.0);
    }

    fnz(circuit, phi, atol);
    return circuit;
}

OneQubitGateSequence circuit_psx(double theta, double phi, double lam, double phase, bool simplify) {

    auto atol = ANGLE_ZERO_EPSILON;
    if(!simplify){
        atol = -1.0;
    }

    return circuit_psx_gen(theta,phi,lam,phase,simplify,atol,fnz_phase,fnz_sx);
}

// Generate ZSXX circuit
OneQubitGateSequence circuit_zsxx(double theta, double phi, double lam, double phase, bool simplify) {
    auto atol = ANGLE_ZERO_EPSILON;
    if (!simplify) {
        atol = -1.0;
    }
    // Correct function pointers for fnz (with two doubles) and fnx (with one argument)
    return circuit_psx_gen(theta, phi, lam, phase, simplify, atol, fnz_rz, fnz_sx, fnz_x);
}

// Generate ZSX circuit
OneQubitGateSequence circuit_zsx(double theta, double phi, double lam, double phase,bool simplify) {
    auto atol = ANGLE_ZERO_EPSILON;
    if(!simplify){
        atol = -1.0;
    }
    
    return circuit_psx_gen(theta,phi,lam,phase,simplify,atol,fnz_rz,fnz_sx);
}

// Function to decompose unitary matrix into gates
OneQubitGateSequence decompose_unitary_to_basis(const Eigen::Matrix2cd& unitary,EulerBasis target_basis, 
                                                bool simplify = true, double atol = ANGLE_ZERO_EPSILON) {
    std::vector<double> angles = angles_from_unitary(unitary, target_basis);
    double theta = angles[0];
    double phi = angles[1];
    double lam = angles[2];
    double phase = angles[3];
    
    // std::cout << "theta: " << theta << "phi: " << phi << "lam: " << lam << "phase:  " << phase << std::endl;
    switch (target_basis) {
        // case EulerBasis::U3:
        //     return circuit_u3(theta, phi, lam, phase);
        // case EulerBasis::U321:
        //     return circuit_u321(theta, phi, lam, phase);
        // case EulerBasis::U:
        //     return circuit_u(theta, phi, lam, phase);
        // case EulerBasis::PSX:
        //     return circuit_psx(theta, phi, lam, phase,simplify);
        case EulerBasis::ZYZ:
            return circuit_kak(theta, phi, lam, phase, StandardGate::RZ, StandardGate::RY, simplify, atol);
        case EulerBasis::ZXZ:
            return circuit_kak(theta, phi, lam, phase, StandardGate::RZ, StandardGate::RX,simplify, atol);
        case EulerBasis::XYX:
            return circuit_kak(theta, phi, lam, phase, StandardGate::RX, StandardGate::RY,simplify, atol);
        case EulerBasis::XZX:
            return circuit_kak(theta, phi, lam, phase, StandardGate::RX, StandardGate::RZ,simplify, atol);
        case EulerBasis::ZSXX:
            return circuit_zsxx(theta, phi, lam, phase,simplify);
        case EulerBasis::ZSX:
            return circuit_zsx(theta, phi, lam, phase,simplify);
        default:
            throw std::invalid_argument("Unknown basis");
    }
}

// Convert string to EulerBasis
EulerBasis eulerBasisFromStr(const std::string& name) {
    if (name == "U3") return EulerBasis::U3;
    if (name == "U321") return EulerBasis::U321;
    if (name == "U") return EulerBasis::U;
    if (name == "PSX") return EulerBasis::PSX;
    if (name == "U1X") return EulerBasis::U1X;
    if (name == "RR") return EulerBasis::RR;
    if (name == "ZYZ") return EulerBasis::ZYZ;
    if (name == "ZXZ") return EulerBasis::ZXZ;
    if (name == "XYX") return EulerBasis::XYX;
    if (name == "XZX") return EulerBasis::XZX;
    if (name == "ZSXX") return EulerBasis::ZSXX;
    if (name == "ZSX") return EulerBasis::ZSX;
    
    throw std::invalid_argument("Unknown basis: " + name);
}

std::string getGateName_single(StandardGate gate) {
    switch (gate) {
        case StandardGate::RZ: return "rz";
        case StandardGate::RY: return "ry";
        case StandardGate::RX: return "rx";
        case StandardGate::SX: return "sx";
        case StandardGate::X: return "x";
        case StandardGate::U3: return "u3";
        case StandardGate::U2: return "u2";
        case StandardGate::U1: return "u1";
        case StandardGate::U: return "u";
        case StandardGate::Phase: return "p";
        case StandardGate::R: return "r";
        default: return "unknown";
    }
}

std::vector<qop_t> least_cost_basis(const Eigen::Matrix2cd& unitary,const std::vector<EulerBasis>& target_basis_set) {
    
    std::vector<qop_t> best_sequence;
    for (int i = 0;i < target_basis_set.size();i++) {
        auto basis = target_basis_set[i];
        OneQubitGateSequence decompose_result = decompose_unitary_to_basis(unitary, basis);
        auto simplified_seq = OneQubitGateSequence_to_qop_t(decompose_result);
        if(i == 0){
            best_sequence = simplified_seq;
            // std::cout << "best_sequence.size(): " << best_sequence.size() << std::endl;
        }else{
            // std::cout << "simplified_seq.size(): " << simplified_seq.size() << std::endl;
            if(simplified_seq.size() < best_sequence.size()){
                best_sequence = simplified_seq;
            }
        }
    }
    
    return best_sequence;
}

} // namespace

namespace qllvm {
namespace utils {

    

OneQubitGateSequence decomposeunitarytobasis(const Eigen::Matrix2cd& unitary,EulerBasis basis){
  return decompose_unitary_to_basis(unitary,basis);
}
std::vector<double> paramsxyxinner(const Eigen::Matrix2cd& mat){
  return params_xyx_inner(mat);
}

std::vector<double> paramszyzinner(const Eigen::Matrix2cd& mat){
  return params_zyz_inner(mat);
}

std::vector<qop_t> leastcost_basis(const Eigen::Matrix2cd& unitary,const std::vector<EulerBasis>& target_basis_set){
    return least_cost_basis(unitary,target_basis_set);
}

EulerBasis euler_Basis_FromStr(const std::string& name){
  return eulerBasisFromStr(name);
}

std::vector<qop_t> OneQubitGateSequence_to_qop_t(OneQubitGateSequence decompose_result){
  std::vector<qop_t> composite;
  for (auto g : decompose_result.gates){
    composite.emplace_back(std::make_pair(getGateName_single(g.type), g.params));
  }
  return composite;
}

std::vector<qop_t> new_euler_decompose(const std::vector<qop_t> &op_list,std::unordered_set<std::string> basic_gate_set){
    Eigen::Matrix2cd totalU = Eigen::MatrixXcd::Identity(2, 2);
    for (const auto &op : op_list) {
        totalU = qllvm::matrix::getSingleGateMat(op) * totalU;
    }
    
    std::vector<qllvm::utils::EulerBasis> target_basis_set;
    std::vector<std::string> basis_names = {};
    if(basic_gate_set.size() > 0){
        basis_names.clear();
        if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("ry") != basic_gate_set.end()){
            basis_names.emplace_back("ZYZ");
        }
        if(basic_gate_set.find("rx") != basic_gate_set.end() && basic_gate_set.find("ry") != basic_gate_set.end()){
            basis_names.emplace_back("XYX");
        }
        if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("rx") != basic_gate_set.end()){
            basis_names.emplace_back("ZXZ");
            basis_names.emplace_back("XZX");
        }
        if(basic_gate_set.find("sx") != basic_gate_set.end() && basic_gate_set.find("x") != basic_gate_set.end()){
            basis_names.emplace_back("ZSXX");
        }
        if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("sx") != basic_gate_set.end()){
            basis_names.emplace_back("ZSX");
        }

    }
    for(int i = 0;i < basis_names.size();i++){
        auto basis = qllvm::utils::euler_Basis_FromStr(basis_names[i]);
        target_basis_set.emplace_back(basis);
    }
    return least_cost_basis(totalU,target_basis_set);
}

} // namespace utils
} // namespace qllvm