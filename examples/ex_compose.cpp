// Adapter Fabric — composition.
#include "adapter_fabric/fabric.hpp"
#include <iostream>
using namespace adapter_fabric;
static AdapterDescriptor mk(const std::string& n, const std::string& mod, BaseModelId bm, ModelRevisionId mr, uint64_t mem){ AdapterDescriptor d; d.id=AdapterId::generate(); d.revision=AdapterRevisionId::generate(); d.artifact=AdapterArtifactId::generate(); d.artifact_digest="aaa"; d.kind=AdapterKind::lora; d.name=n; d.base_model=bm; d.base_model_revision=mr; TargetModule t; t.name=mod; t.in_features=4096; t.out_features=4096; t.shape={4096,4096}; d.targets.push_back(t); d.rank=8; d.dtype=DType::f16; d.memory_bytes=mem; d.format="lora/v1"; d.format_version=1; d.validation=ValidationState::valid; return d; }
int main(){
  Fabric f;
  auto bm=BaseModelId::generate(); auto mr=ModelRevisionId::generate();
  auto a=f.register_adapter(mk("attn", "q_proj", bm, mr, 4000));
  auto b=f.register_adapter(mk("v", "v_proj", bm, mr, 2000));
  auto c=f.publish_composition({a.id, b.id}, "policy-x");
  std::cout<<"composition gen="<<c.generation.value()<<" digest="<<c.digest<<" totalMem="<<c.total_memory_bytes<<"\n";
  return 0;
}
