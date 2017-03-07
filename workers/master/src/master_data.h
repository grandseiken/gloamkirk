#ifndef GLOAM_WORKERS_MASTER_SRC_MASTER_DATA_H
#define GLOAM_WORKERS_MASTER_SRC_MASTER_DATA_H
#include "common/src/managed/managed.h"
#include <schema/master.h>

namespace gloam {
namespace master {

class MasterData : public gloam::managed::WorkerLogic {
public:
  MasterData();
  const schema::MasterData& data();

  void init(managed::ManagedConnection& c) override;
  void update() override;

private:
  schema::MasterData data_;
};

}  // ::master
}  // ::gloam

#endif
