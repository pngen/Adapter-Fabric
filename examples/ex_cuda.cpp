// Adapter Fabric — real CUDA adapter execution/reuse proof.
#include "adapter_fabric/proof.hpp"
#include "adapter_fabric/cuda.hpp"
#include <iostream>
int main(){
  if(!adapter_fabric::cuda::available()){ std::cout<<"CUDA unavailable: "<<adapter_fabric::cuda::availability_reason()<<"\n"; return 0; }
  return adapter_fabric::run_cuda_proof();
}
