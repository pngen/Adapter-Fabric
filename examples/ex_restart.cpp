// Adapter Fabric — worker restart and stale-boot authority rejection.
#include "adapter_fabric/fabric.hpp"
#include <iostream>
using namespace adapter_fabric;
static AdapterDescriptor mk(BaseModelId bm, ModelRevisionId mr){ AdapterDescriptor d; d.id=AdapterId::generate(); d.revision=AdapterRevisionId::generate(); d.artifact=AdapterArtifactId::generate(); d.artifact_digest="aaa"; d.kind=AdapterKind::lora; d.name="lora"; d.base_model=bm; d.base_model_revision=mr; TargetModule t; t.name="q_proj"; t.in_features=4096; t.out_features=4096; t.shape={4096,4096}; d.targets.push_back(t); d.rank=8; d.dtype=DType::f16; d.memory_bytes=8192; d.format="lora/v1"; d.format_version=1; d.validation=ValidationState::valid; return d; }
int main(){
  Fabric f;
  auto bm=BaseModelId::generate(); auto mr=ModelRevisionId::generate();
  auto d=f.register_adapter(mk(bm, mr));
  WorkerRecord w; w.id=WorkerId::generate(); w.boot=WorkerBootId::generate(); w.connected=true; w.device=DeviceId::generate();
  f.register_worker(w);
  auto inst=f.create_instance(d.id, w.id, w.boot, w.device, d.memory_bytes);
  AuthorityFence fe; fe.adapter=d.id; fe.adapter_generation=d.generation; fe.base_model_revision=d.base_model_revision; fe.epoch=f.epoch();
  fe.worker=w.id; fe.boot=w.boot; fe.device=w.device; fe.attempt=AttemptId::generate(); fe.residency_generation=ResidencyGeneration{1};
  fe.composition_generation=CompositionGeneration{1}; fe.artifact_generation=ArtifactGeneration{1};
  f.bind_authority(d.id, fe);
  // worker dies / restarts with a fresh boot
  auto new_boot=WorkerBootId::generate();
  f.on_worker_restart(w.id, new_boot);
  WorkerRecord w2=w; w2.boot=new_boot; f.register_worker(w2);
  auto chk=f.check_fence(fe);   // old boot fence
  std::cout<<"stale boot after restart: "<<(chk.authorized()?"ACCEPTED(bad)":"rejected")<<" ("<<chk.reason<<")\n";
  return chk.authorized()?1:0;
}
