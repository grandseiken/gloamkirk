#ifndef GLOAM_WORKERS_CLIENT_SRC_GLO_H
#define GLOAM_WORKERS_CLIENT_SRC_GLO_H
#include <GL/glew.h>
#include <glm/vec2.hpp>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace glo {

inline void Init() {
  glewExperimental = GL_TRUE;
  auto glew_ok = glewInit();
  if (glew_ok != GLEW_OK) {
    std::cerr << "[warning] couldn't initialize GLEW: " << glewGetErrorString(glew_ok) << std::endl;
  }

#define GLEW_CHECK(value)                                            \
  if (!value) {                                                      \
    std::cerr << "[warning] " #value " not supported." << std::endl; \
  }
  GLEW_CHECK(GLEW_VERSION_3_3);
  GLEW_CHECK(GLEW_ARB_shading_language_100);
  GLEW_CHECK(GLEW_ARB_shader_objects);
  GLEW_CHECK(GLEW_ARB_vertex_shader);
  GLEW_CHECK(GLEW_ARB_tessellation_shader);
  GLEW_CHECK(GLEW_ARB_geometry_shader4);
  GLEW_CHECK(GLEW_ARB_fragment_shader);
  GLEW_CHECK(GLEW_ARB_texture_rectangle);
  GLEW_CHECK(GLEW_ARB_sampler_objects);
  GLEW_CHECK(GLEW_ARB_framebuffer_object);
  GLEW_CHECK(GLEW_EXT_framebuffer_blit);
#undef GLEW_CHECK
}

struct Shader {
public:
  Shader(const Shader&) = delete;
  Shader(Shader&& other) = default;
  Shader& operator=(const Shader&) = delete;
  Shader& operator=(Shader&& other) = default;

  Shader(const std::string& name, std::uint32_t shader_type, const std::string& source)
  : resource_{new Resource{shader_type}} {
    auto source_versioned = "#version 430 core\n" + source;
    auto data = source_versioned.data();
    glShaderSource(resource_->shader, 1, &data, nullptr);
    glCompileShader(resource_->shader);

    GLint status;
    glGetShaderiv(resource_->shader, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE) {
      return;
    }

    GLint log_length;
    glGetShaderiv(resource_->shader, GL_INFO_LOG_LENGTH, &log_length);

    std::unique_ptr<GLchar> log{new GLchar[log_length + 1]};
    glGetShaderInfoLog(resource_->shader, log_length, nullptr, log.get());

    std::cerr << "compile error in shader '" << name << "':\n" << log.get() << "\n";
  }

private:
  struct Resource {
    Resource(std::uint32_t shader_type) : shader{glCreateShader(shader_type)} {}

    ~Resource() {
      glDeleteShader(shader);
    }

    GLuint shader;
  };

  std::unique_ptr<Resource> resource_ = 0;
  friend struct Program;
};

struct ActiveTexture {
public:
  ActiveTexture(const ActiveTexture&) = delete;
  ActiveTexture(ActiveTexture&& other) = default;
  ActiveTexture& operator=(const ActiveTexture&) = delete;
  ActiveTexture& operator=(ActiveTexture&& other) = default;

private:
  ActiveTexture(GLuint texture, GLenum target) : resource_{new Resource{texture, target}} {}

  struct Resource {
    Resource(GLuint texture, GLenum target) : target{target} {
      stack[target].push_back(texture);
      glBindTexture(target, texture);
    }

    ~Resource() {
      stack[target].pop_back();
      glBindTexture(target, stack[target].empty() ? 0 : stack[target].back());
    }

    GLenum target;
    static std::unordered_map<GLenum, std::vector<GLuint>> stack;
  };

  std::unique_ptr<Resource> resource_;
  friend struct Texture;
};

struct Texture {
public:
  Texture(const Texture&) = delete;
  Texture(Texture&& other) = default;
  Texture& operator=(const Texture&) = delete;
  Texture& operator=(Texture&& other) = default;

  Texture() : resource_{new Resource} {
    glSamplerParameteri(resource_->sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(resource_->sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(resource_->sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(resource_->sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }

  void set_linear() {
    glSamplerParameteri(resource_->sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(resource_->sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }

  ActiveTexture bind() const {
    return {resource_->texture, resource_->target};
  }

  void create_1d(GLsizei width, GLuint components, GLenum type, const void* data) {
    resource_->target = GL_TEXTURE_1D;
    auto active = bind();
    auto internal_format =
        components == 4 ? GL_RGBA8 : components == 3 ? GL_RGB8 : components == 1 ? GL_R32F : 0;
    auto format =
        components == 4 ? GL_RGBA : components == 3 ? GL_RGB : components == 1 ? GL_RED : 0;
    glTexImage1D(resource_->target, 0, internal_format, width, 0, format, type, data);
  }

  void create_2d(const glm::ivec2& dimensions, GLuint components, GLenum type, const void* data) {
    resource_->target = GL_TEXTURE_2D;
    auto active = bind();
    auto internal_format =
        components == 4 ? GL_RGBA8 : components == 3 ? GL_RGB8 : components == 1 ? GL_R32F : 0;
    auto format =
        components == 4 ? GL_RGBA : components == 3 ? GL_RGB : components == 1 ? GL_RED : 0;
    glTexImage2D(resource_->target, 0, internal_format, dimensions.x, dimensions.y, 0, format, type,
                 data);
  }

  void create_high_precision_rgb(const glm::ivec2& dimensions) {
    resource_->target = GL_TEXTURE_2D;
    auto active = bind();
    glTexImage2D(resource_->target, 0, GL_RGB16F, dimensions.x, dimensions.y, 0, GL_RGB, GL_FLOAT,
                 nullptr);
  }

  void create_rgba(const glm::ivec2& dimensions) {
    resource_->target = GL_TEXTURE_2D;
    auto active = bind();
    glTexImage2D(resource_->target, 0, GL_RGBA8, dimensions.x, dimensions.y, 0, GL_RGBA,
                 GL_UNSIGNED_INT_8_8_8_8_REV, nullptr);
  }

  void create_depth_stencil(const glm::ivec2& dimensions) {
    resource_->target = GL_TEXTURE_2D;
    auto active = bind();
    glTexImage2D(resource_->target, 0, GL_DEPTH24_STENCIL8, dimensions.x, dimensions.y, 0,
                 GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
  }

  void attach_to_framebuffer(GLenum attachment) const {
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, resource_->target, resource_->texture, 0);
  }

private:
  struct Resource {
    Resource() : target{0}, texture{0}, sampler{0} {
      glGenTextures(1, &texture);
      glGenSamplers(1, &sampler);
    }

    ~Resource() {
      glDeleteSamplers(1, &sampler);
      glDeleteTextures(1, &texture);
    }

    GLenum target;
    GLuint texture;
    GLuint sampler;
  };

  std::unique_ptr<Resource> resource_;
  friend struct ActiveProgram;
};

struct ActiveProgram {
public:
  ActiveProgram(const Texture&) = delete;
  ActiveProgram(ActiveProgram&& other) = default;
  ActiveProgram& operator=(const ActiveProgram&) = delete;
  ActiveProgram& operator=(ActiveProgram&& other) = default;

  GLuint uniform(const std::string& name) const {
    return glGetUniformLocation(resource_->program, name.c_str());
  }

  void uniform_texture(const std::string& name, const Texture& texture) const {
    glUniform1i(uniform(name), resource_->texture_index);
    glActiveTexture(GL_TEXTURE0 + resource_->texture_index);
    glBindTexture(texture.resource_->target, texture.resource_->texture);
    glBindSampler(resource_->texture_index, texture.resource_->sampler);
    ++resource_->texture_index;
  }

private:
  ActiveProgram(GLuint program) : resource_{new Resource{program}} {}

  struct Resource {
    Resource(GLuint program) : program{program} {
      stack.push_back(program);
      glUseProgram(program);
    }

    ~Resource() {
      stack.pop_back();
      glUseProgram(stack.empty() ? 0 : stack.back());
    }

    GLuint program;
    mutable GLenum texture_index = 0;
    static std::vector<GLuint> stack;
  };

  std::unique_ptr<Resource> resource_;
  friend struct Program;
};

struct Program {
public:
  Program(const Texture&) = delete;
  Program(Program&& other) = default;
  Program& operator=(const Program&) = delete;
  Program& operator=(Program&& other) = default;

  Program(const std::string& name, Shader&& a, Shader&& b) : resource_{new Resource} {
    std::vector<Shader> shaders;
    shaders.emplace_back(std::move(a));
    shaders.emplace_back(std::move(b));
    compile(name, shaders);
  }

  Program(const std::string& name, const std::vector<Shader>& shaders) : resource_{new Resource} {
    compile(name, shaders);
  }

  ActiveProgram use() const {
    return {resource_->program};
  }

private:
  void compile(const std::string& name, const std::vector<Shader>& shaders) {
    for (const auto& shader : shaders) {
      if (shader.resource_->shader == 0) {
        return;
      }
    }

    for (const auto& shader : shaders) {
      glAttachShader(resource_->program, shader.resource_->shader);
    }
    glLinkProgram(resource_->program);

    GLint status;
    glGetProgramiv(resource_->program, GL_LINK_STATUS, &status);
    for (const auto& shader : shaders) {
      glDetachShader(resource_->program, shader.resource_->shader);
    }
    if (status == GL_TRUE) {
      return;
    }

    GLint log_length;
    glGetProgramiv(resource_->program, GL_INFO_LOG_LENGTH, &log_length);

    std::unique_ptr<GLchar> log{new GLchar[log_length + 1]};
    glGetProgramInfoLog(resource_->program, log_length, nullptr, log.get());

    std::cerr << "link error in program '" << name << "':\n" << log.get() << "\n";
    return;
  }

  struct Resource {
    Resource() : program{glCreateProgram()} {}

    ~Resource() {
      glDeleteProgram(program);
    }

    GLuint program;
  };

  std::unique_ptr<Resource> resource_;
};

struct ActiveFramebuffer {
public:
  ActiveFramebuffer(const ActiveFramebuffer&) = delete;
  ActiveFramebuffer(ActiveFramebuffer&& other) = default;
  ActiveFramebuffer& operator=(const ActiveFramebuffer&) = delete;
  ActiveFramebuffer& operator=(ActiveFramebuffer&& other) = default;

private:
  ActiveFramebuffer(GLuint fbo, GLenum target) : resource_{new Resource{fbo, target}} {}

  struct Resource {
    Resource(GLuint fbo, GLenum target) : fbo{fbo}, target{target} {
      glBindFramebuffer(target, fbo);
      stack[target].push_back(fbo);
    }

    ~Resource() {
      stack[target].pop_back();
      // Hack, probably works for actual use-cases.
      if (target == GL_FRAMEBUFFER && stack[target].empty()) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, stack[GL_READ_FRAMEBUFFER].empty()
                              ? 0
                              : stack[GL_READ_FRAMEBUFFER].back());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, stack[GL_DRAW_FRAMEBUFFER].empty()
                              ? 0
                              : stack[GL_DRAW_FRAMEBUFFER].back());
      } else if (stack[target].empty()) {
        glBindFramebuffer(target, stack[GL_FRAMEBUFFER].empty() ? 0 : stack[GL_FRAMEBUFFER].back());
      } else {
        glBindFramebuffer(target, stack[target].back());
      }
    }

    GLuint fbo;
    GLenum target;
    static std::unordered_map<GLenum, std::vector<GLuint>> stack;
  };

  std::unique_ptr<Resource> resource_;
  friend struct Framebuffer;
};

struct Framebuffer {
public:
  Framebuffer(const Framebuffer&) = delete;
  Framebuffer(Framebuffer&& other) = default;
  Framebuffer& operator=(const Framebuffer&) = delete;
  Framebuffer& operator=(Framebuffer&& other) = default;

  Framebuffer(const glm::ivec2& dimensions) : resource_{new Resource{dimensions}} {}

  void add_colour_buffer(bool high_precision) {
    resource_->colour_textures.emplace_back();
    if (high_precision) {
      resource_->colour_textures.back().create_high_precision_rgb(resource_->dimensions);
    } else {
      resource_->colour_textures.back().create_rgba(resource_->dimensions);
    }
    add_colour_buffer(resource_->colour_textures.back());
  }

  void add_colour_buffer(const Texture& texture) {
    ActiveFramebuffer bind{resource_->framebuffer, GL_FRAMEBUFFER};
    texture.attach_to_framebuffer(GL_COLOR_ATTACHMENT0 + resource_->colour_attachment++);
  }

  void set_draw_buffers(const std::vector<std::size_t>& indices) {
    std::vector<GLenum> draw_buffers;
    for (auto i : indices) {
      draw_buffers.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
    }
    glNamedFramebufferDrawBuffers(resource_->framebuffer, static_cast<GLsizei>(draw_buffers.size()),
                                  draw_buffers.data());
  }

  void add_depth_stencil_buffer() {
    ActiveFramebuffer bind{resource_->framebuffer, GL_FRAMEBUFFER};
    resource_->depth_stencil_texture.reset(new Texture);
    resource_->depth_stencil_texture->create_depth_stencil(resource_->dimensions);
    add_depth_stencil_buffer(*resource_->depth_stencil_texture);
  }

  void add_depth_stencil_buffer(const Texture& texture) {
    ActiveFramebuffer bind{resource_->framebuffer, GL_FRAMEBUFFER};
    texture.attach_to_framebuffer(GL_DEPTH_STENCIL_ATTACHMENT);
  }

  void check_complete() const {
    if (glCheckNamedFramebufferStatus(resource_->framebuffer, GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
      std::cerr << "[warning] Framebuffer is not complete." << std::endl;
    }
  }

  const glm::ivec2& dimensions() const {
    return resource_->dimensions;
  }

  ActiveFramebuffer draw() const {
    return {resource_->framebuffer, GL_DRAW_FRAMEBUFFER};
  }

  ActiveFramebuffer read() const {
    return {resource_->framebuffer, GL_READ_FRAMEBUFFER};
  }

  const std::vector<Texture>& colour_textures() const {
    return resource_->colour_textures;
  }

  const std::unique_ptr<Texture>& depth_stencil_texture() const {
    return resource_->depth_stencil_texture;
  }

private:
  struct Resource {
    Resource(const glm::ivec2& dimensions) : dimensions{dimensions} {
      glGenFramebuffers(1, &framebuffer);
    }

    ~Resource() {
      glDeleteFramebuffers(1, &framebuffer);
    }

    GLuint framebuffer = 0;
    glm::ivec2 dimensions;
    std::uint32_t colour_attachment = 0;
    std::vector<Texture> colour_textures;
    std::unique_ptr<Texture> depth_stencil_texture;
  };

  std::unique_ptr<Resource> resource_;
};

struct VertexData {
public:
  VertexData(const VertexData&) = delete;
  VertexData(VertexData&& other) = default;
  VertexData& operator=(const VertexData&) = delete;
  VertexData& operator=(VertexData&& other) = default;

  VertexData(const std::vector<GLfloat>& data, const std::vector<GLuint>& indices, GLenum hint)
  : resource_{new Resource} {
    resource_->size = static_cast<GLuint>(indices.size());

    glBindBuffer(GL_ARRAY_BUFFER, resource_->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * data.size(), data.data(), hint);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resource_->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), hint);

    glBindVertexArray(resource_->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resource_->ibo);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  void enable_attribute(GLuint location, GLuint count, GLuint stride, GLuint offset) const {
    glBindVertexArray(resource_->vao);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, resource_->vbo);
    glVertexAttribPointer(location, count, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat),
                          reinterpret_cast<void*>(sizeof(float) * offset));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  void draw() const {
    glBindVertexArray(resource_->vao);
    glDrawElements(GL_TRIANGLES, resource_->size, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }

private:
  struct Resource {
    Resource() {
      glGenBuffers(1, &vbo);
      glGenBuffers(1, &ibo);
      glGenVertexArrays(1, &vao);
    }

    ~Resource() {
      glDeleteVertexArrays(1, &vao);
      glDeleteBuffers(1, &vbo);
      glDeleteBuffers(1, &ibo);
    }

    GLuint size = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    GLuint vao = 0;
  };

  std::unique_ptr<Resource> resource_;
};

}  // ::glo

#endif
