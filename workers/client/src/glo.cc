#include "workers/client/src/glo.h"

namespace glo {

std::unordered_map<GLuint, std::vector<GLuint>> ActiveTexture::Resource::stack;
std::unordered_map<GLuint, std::vector<GLuint>> ActiveFramebuffer::Resource::stack;
std::vector<GLuint> ActiveProgram::Resource::stack;

}  // ::glo