// Adapter Fabric — benchmark driver.
#include "adapter_fabric/proof.hpp"
#include <iostream>
int main(){
  try { return adapter_fabric::run_benchmarks(); }
  catch (const std::exception& e) { std::cerr << "benchmark exception: " << e.what() << "\n"; return 1; }
  catch (...) { std::cerr << "benchmark unknown exception\n"; return 1; }
}