#ifndef GLOAM_WORKERS_CLIENT_SRC_COMPONENTS_H
#define GLOAM_WORKERS_CLIENT_SRC_COMPONENTS_H

namespace worker {
class ComponentRegistry;
}  // ::worker

namespace gloam {
const worker::ComponentRegistry& ClientComponents();
}  // ::gloam

#endif
