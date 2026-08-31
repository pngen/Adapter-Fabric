// Adapter Fabric — invalidation after a model revision change.
#include "adapter_fabric/fabric.hpp"
#include <iostream>
using namespace adapter_fabric;
static AdapterDescriptor mk(BaseModelId bm, ModelRevisionId mr){ AdapterDescriptor d; d.id=AdapterId::generate(); d.revision=AdapterRevisionId::generate(); d.artifact=AdapterArtifactId::generate(); d.artifact_digest="aaa"; d.kind=AdapterKind::lora; d.name="lora"; d.base_model=bm; d.base_model_revision=mr; TargetModule t; t.name="q_proj"; t.in_features=4096; t.out_features=4096; t.shape={4096,4096}; d.targets.push_back(t); d.rank=8; d.dtype=DType::f16; d.memory_bytes=8192; d.format="lora/v1"; d.format_version=1; d.validation=ValidationState::valid; return d; }
int main(){
  Fabric f;
  auto bm=BaseModelId::generate(); auto mr=ModelRevisionId::generate();
  auto d=f.register_adapter(mk(bm, mr));
  auto rec=f.invalidate(d.id, InvalidationTrigger::base_model_revision_change, "model revision changed underneath");
  std::cout<<"invalidated: "<<rec.reason<<" prior_gen="<<rec.prior_generation.value()<<"\n";
  return 0;
}
