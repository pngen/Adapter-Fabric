// Adapter Fabric — governed migration.
#include "adapter_fabric/migration.hpp"
#include <iostream>
using namespace adapter_fabric;
int main(){
  MigrationStateMachine m;
  m.advance(MigrationPhase::source_validated);
  m.advance(MigrationPhase::destination_reserved);
  m.advance(MigrationPhase::transferring);
  m.advance(MigrationPhase::verifying);
  m.advance(MigrationPhase::destination_ready);
  std::cout<<"migration phase before authority transfer: "<<to_string(m.phase())<<"\n";
  bool ok=m.advance(MigrationPhase::authority_promoted);
  std::cout<<"authority promoted: "<<ok<<" phase="<<to_string(m.phase())<<"\n";
  m.advance(MigrationPhase::source_retired);
  m.advance(MigrationPhase::source_released);
  std::cout<<"source released: "<<to_string(m.phase())<<"\n";
  return 0;
}
