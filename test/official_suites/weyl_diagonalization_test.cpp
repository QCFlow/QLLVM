// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
#include "mlir/transforms/optimizations/utils/weyl_diagonalization.hpp"
#include <iostream>
#include <limits>

int main() {
  using namespace Eigen;
  Matrix4d vectors;
  Vector4cd diagonal;
  unsigned attempts=0;
  const Matrix4cd identity=Matrix4cd::Identity();
  if (!qllvm::kak::diagonalizeSymmetricUnitary(identity,vectors,diagonal,1e-15,&attempts) || attempts!=1)
    return 1;
  std::mt19937_64 rng(2031);
  std::normal_distribution<double> normal(0,1);
  unsigned total=0;
  for (unsigned sample=0;sample<200;++sample) {
    Matrix4d real;
    for(int i=0;i<4;++i) for(int j=0;j<4;++j) real(i,j)=normal(rng);
    const Matrix4d q=real.householderQr().householderQ();
    Vector4cd eigenvalues;
    for(int i=0;i<4;++i) eigenvalues(i)=std::polar(1.0,sample%4==0 ? .3 : normal(rng));
    const Matrix4cd m=q*eigenvalues.asDiagonal()*q.transpose();
    if(!qllvm::kak::diagonalizeSymmetricUnitary(m,vectors,diagonal,1e-12,&attempts)) return 2;
    if((vectors*diagonal.asDiagonal()*vectors.transpose()-m).norm()>1e-11) return 3;
    total+=attempts;
    Matrix4d again;Vector4cd d2;
    if(!qllvm::kak::diagonalizeSymmetricUnitary(m,again,d2,1e-12) || (again-vectors).norm()!=0) return 4;
    // At the production tolerance, accept only the same strict residual.
    if(qllvm::kak::diagonalizeSymmetricUnitary(m,vectors,diagonal,1e-15) &&
       (vectors*diagonal.asDiagonal()*vectors.transpose()-m).cwiseAbs().maxCoeff()>1e-15) return 5;
  }
  Matrix4cd invalid=Matrix4cd::Identity();invalid(0,1)=.5;
  if(qllvm::kak::diagonalizeSymmetricUnitary(invalid,vectors,diagonal,1e-15,&attempts) || attempts!=100) return 6;
  invalid(0,1)=std::numeric_limits<double>::quiet_NaN();
  if(qllvm::kak::diagonalizeSymmetricUnitary(invalid,vectors,diagonal,1e-15,&attempts) || attempts!=0) return 7;
  std::cout << "200 symmetric-unitary cases, degeneracy, strict residual, determinism, invalid/NaN rejection passed; attempts=" << total << "\n";
}
