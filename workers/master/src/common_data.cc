#include "workers/master/src/common_data.h"
#include <improbable/worker.h>

namespace gloam {
namespace master {

MasterData::MasterData() : data_{false, {}} {}

const schema::MasterData& MasterData::data() {
  return data_;
}

void MasterData::init(managed::ManagedConnection& c) {
  c.dispatcher.OnAddComponent<schema::Master>(
      [&](const worker::AddComponentOp<schema::Master>& op) { data_ = op.Data; });

  c.dispatcher.OnComponentUpdate<schema::Master>(
      [&](const worker::ComponentUpdateOp<schema::Master>& op) { op.Update.ApplyTo(data_); });
}

void MasterData::update() {}

}  // ::master
}  // ::gloam
