// Adapter Fabric — downstream find_package consumer.
#include "adapter_fabric/fabric.hpp"
#include "adapter_fabric/adapter.hpp"
#include <iostream>

int main() {
  adapter_fabric::Fabric f;
  auto bm = adapter_fabric::BaseModelId::generate();
  auto mr = adapter_fabric::ModelRevisionId::generate();
  adapter_fabric::AdapterDescriptor d;
  d.id = adapter_fabric::AdapterId::generate(); d.revision = adapter_fabric::AdapterRevisionId::generate();
  d.artifact = adapter_fabric::AdapterArtifactId::generate(); d.artifact_digest = "cccc";
  d.kind = adapter_fabric::AdapterKind::lora; d.name = "consumer-lora";
  d.base_model = bm; d.base_model_revision = mr;
  adapter_fabric::TargetModule t; t.name = "q_proj"; t.in_features = 8; t.out_features = 8; t.shape = {8,8};
  d.targets.push_back(t); d.rank = 4; d.dtype = adapter_fabric::DType::f16; d.memory_bytes = 256;
  d.format = "lora/v1"; d.format_version = 1; d.validation = adapter_fabric::ValidationState::valid;
  auto reg = f.register_adapter(std::move(d));
  std::cout << "consumer registered " << reg.name << " id=" << reg.id.str() << " gen=" << reg.generation.value() << "\n";
  std::cout << "Adapter Fabric consumer OK\n";
  return 0;
}
