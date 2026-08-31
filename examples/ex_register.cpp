// Adapter Fabric — registration + validation.
#include "adapter_fabric/fabric.hpp"
#include "adapter_fabric/adapter.hpp"
#include <iostream>
using namespace adapter_fabric;
static AdapterDescriptor mk(const std::string& n, BaseModelId bm, ModelRevisionId mr){ AdapterDescriptor d; d.id=AdapterId::generate(); d.revision=AdapterRevisionId::generate(); d.artifact=AdapterArtifactId::generate(); d.artifact_digest="aaa"; d.kind=AdapterKind::lora; d.name=n; d.base_model=bm; d.base_model_revision=mr; TargetModule t; t.name="q_proj"; t.in_features=4096; t.out_features=4096; t.shape={4096,4096}; d.targets.push_back(t); d.rank=8; d.dtype=DType::f16; d.memory_bytes=8192; d.format="lora/v1"; d.format_version=1; d.validation=ValidationState::valid; return d; }
int main(){
  Fabric f;
  auto bm=BaseModelId::generate(); auto mr=ModelRevisionId::generate();
  auto a=f.register_adapter(mk("lora-a", bm, mr));
  std::cout<<"registered "<<a.name<<" id="<<a.id.str()<<" gen="<<a.generation.value()<<"\n";
  CompatibilityTarget t; t.base_model=bm; t.base_model_revision=mr; t.policy_generation=PolicyGeneration{1}; t.runtime_capabilities["fp16"]="supported";
  auto rep=f.validate(a.id, t);
  std::cout<<(rep.compatible?"compatible":"incompatible")<<": "<<rep.summary<<"\n"<<to_json(rep)<<"\n";
  // wrong revision -> incompatible, explainable
  CompatibilityTarget bad; bad.base_model=bm; bad.base_model_revision=ModelRevisionId::generate(); bad.policy_generation=PolicyGeneration{1}; bad.runtime_capabilities["fp16"]="supported";
  auto rep2=f.validate(a.id, bad);
  std::cout<<(rep2.compatible?"compatible":"incompatible")<<": "<<rep2.summary<<"\n";
  return 0;
}
