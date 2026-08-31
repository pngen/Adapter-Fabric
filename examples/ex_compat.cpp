// Adapter Fabric — compatibility boundary (structured evidence).
#include "adapter_fabric/fabric.hpp"
#include "adapter_fabric/explain.hpp"
#include <iostream>
using namespace adapter_fabric;
static AdapterDescriptor mk(BaseModelId bm, ModelRevisionId mr){ AdapterDescriptor d; d.id=AdapterId::generate(); d.revision=AdapterRevisionId::generate(); d.artifact=AdapterArtifactId::generate(); d.artifact_digest="aaa"; d.kind=AdapterKind::lora; d.name="lora"; d.base_model=bm; d.base_model_revision=mr; TargetModule t; t.name="q_proj"; t.in_features=4096; t.out_features=4096; t.shape={4096,4096}; d.targets.push_back(t); d.rank=8; d.dtype=DType::f16; d.memory_bytes=8192; d.format="lora/v1"; d.format_version=1; d.validation=ValidationState::valid; RuntimeCapability c; c.name="sm_120"; c.value="supported"; d.capabilities.push_back(c); return d; }
int main(){
  Fabric f;
  auto bm=BaseModelId::generate(); auto mr=ModelRevisionId::generate();
  auto d=f.register_adapter(mk(bm, mr));
  CompatibilityTarget t; t.base_model=bm; t.base_model_revision=mr; t.policy_generation=PolicyGeneration{1}; t.runtime_capabilities["fp16"]="supported"; t.runtime_capabilities["sm_120"]="supported";
  auto rep=f.validate(d.id, t);
  auto ex=explain_compatibility(rep);
  std::cout<<"text: "<<ex.text<<"\njson: "<<ex.json<<"\n";
  return 0;
}
