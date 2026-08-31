// Adapter Fabric — persistence/recovery without authority resurrection.
#include "adapter_fabric/fabric.hpp"
#include "adapter_fabric/persistence.hpp"
#include <iostream>
using namespace adapter_fabric;
static AdapterDescriptor mk(BaseModelId bm, ModelRevisionId mr){ AdapterDescriptor d; d.id=AdapterId::generate(); d.revision=AdapterRevisionId::generate(); d.artifact=AdapterArtifactId::generate(); d.artifact_digest="aaa"; d.kind=AdapterKind::lora; d.name="lora"; d.base_model=bm; d.base_model_revision=mr; TargetModule t; t.name="q_proj"; t.in_features=4096; t.out_features=4096; t.shape={4096,4096}; d.targets.push_back(t); d.rank=8; d.dtype=DType::f16; d.memory_bytes=8192; d.format="lora/v1"; d.format_version=1; d.validation=ValidationState::valid; return d; }
int main(){
  Fabric f;
  auto bm=BaseModelId::generate(); auto mr=ModelRevisionId::generate();
  f.register_adapter(mk(bm, mr));
  auto snap=f.snapshot();
  auto bytes=serialize_snapshot(snap);
  // recover into a brand-new fabric; no activation authority is resurrected.
  Fabric f2;
  f2.recover(deserialize_snapshot(bytes));
  std::cout<<"recovered adapters="<<f2.adapters().size()<<" (canonical metadata only, no process-local authority)\n";
  return 0;
}
