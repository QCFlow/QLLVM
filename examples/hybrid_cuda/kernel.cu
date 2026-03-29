#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>

__global__ void myKernel(int* data, int n) {
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < n)
    data[i] *= 2;
  if (i == 0)
    printf("[device] in kernel (thread 0)\n");
}

static void cuda_check_(cudaError_t e, const char* what) {
  if (e != cudaSuccess) {
    fprintf(stderr, "[CUDA] %s: %s\n", what, cudaGetErrorString(e));
    exit(EXIT_FAILURE);
  }
}

#define CUDA_CHECK(call) cuda_check_((call), #call)

void launchCudaKernel(int n) {
  int* d_data = nullptr;
  CUDA_CHECK(cudaMalloc(&d_data, static_cast<size_t>(n) * sizeof(int)));

  printf("[host] before kernel launch\n");
  fflush(stdout);

  dim3 block(256);
  dim3 grid((static_cast<unsigned>(n) + block.x - 1) / block.x);
  myKernel<<<grid, block>>>(d_data, n);

  CUDA_CHECK(cudaGetLastError());
  /* 设备 printf 在同步后才刷到主机 stdout */
  CUDA_CHECK(cudaDeviceSynchronize());

  printf("[host] after kernel sync\n");
  fflush(stdout);

  CUDA_CHECK(cudaFree(d_data));
}
