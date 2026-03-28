#pragma once

#include <Eigen/Eigenvalues>
#include <unsupported/Eigen/KroneckerProduct>
#include <unsupported/Eigen/MatrixFunctions>
#include <optional>
#include <iostream>
#include <unordered_set> 

namespace qllvm {
namespace kak {

// KAK decomposition via *Magic* Bell basis transformation
// Reference:
// https://arxiv.org/pdf/quant-ph/0211002.pdf

class TMP_OP {
public:
    std::string name;
    std::vector<double> params;
    std::vector<int> indexs;

    TMP_OP(std::string n, std::vector<double> ps, std::vector<int> ins) {
        this->name = n;
        for (auto i : ins) {
            this->indexs.emplace_back(i);
        }
        for (auto p : ps) {
            this->params.emplace_back(p);
        }
    }

    void info() {
        std::cout << "name: " << this->name;
        std::cout << ", params:";
        for (auto i : this->params) {
            std::cout << " " << i;
        }

        std::cout << ", indexs:";
        for (auto i : this->indexs) {
            std::cout << " " << i;
        }
        std::cout << std::endl; // add a newline
    }
};

class KAK{
public:
   std::vector<std::string> requiredKeys();
   std::vector<qllvm::kak::TMP_OP> expand(Eigen::Matrix4cd unitary, std::vector<size_t> bits,std::unordered_set<std::string>  basic_gate_set);
private:
  // Single qubit gate matrix
  using GateMatrix = Eigen::Matrix<std::complex<double>, 2, 2>;
  using InputMatrix = Eigen::Matrix<std::complex<double>, 4, 4>;
  struct KakDecomposition
  {
    // Kak description of an arbitrary two-qubit operation.
    // U = g x (Gate A1 Gate A0) x exp(i(xXX + yYY + zZZ))x(Gate b1 Gate b0)
    // i.e. 
    // (1) A global phase factor
    // (2) Two single-qubit operations (before): Gate b0, b1 
    // (3) The Exp() circuit specified by 3 coefficients (x, y, z)
    // (4) Two single-qubit operations (after): Gate a0, a1 
    std::complex<double> g;
    GateMatrix b0;
    GateMatrix b1;
    GateMatrix a0;
    GateMatrix a1;
    double x;
    double y;
    double z;
    // Generates gate sequence:
    std::vector<qllvm::kak::TMP_OP>  toGates(size_t in_bit1, size_t in_bit2,std::unordered_set<std::string>  basic_gate_set) const;
    Eigen::MatrixXcd toMat() const;
  };
    
  std::optional<KakDecomposition> kakDecomposition(const InputMatrix& in_matrix) const;
  // Returns a canonicalized interaction plus before and after corrections.
  KakDecomposition canonicalizeInteraction(double x, double y, double z) const;
};

} // namespace kak
} // namespace qllvm