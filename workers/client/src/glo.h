#ifndef GLOAM_CLIENT_SRC_GLO_H
#define GLOAM_CLIENT_SRC_GLO_H
#include <GL/glew.h>
#include <glm/vec2.hpp>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace glo {

inline void Init() {
  glewExperimental = GL_TRUE;
  auto glew_ok = glewInit();
  if (glew_ok != GLEW_OK) {
    std::cerr << "couldn't initialize GLEW: " << glewGetErrorString(glew_ok) << "\n";
  }

#define GLEW_CHECK(value)                               \
  if (!value) {                                         \
    std::cerr << "warning: " #value " not supported\n"; \
  }
  GLEW_CHECK(GLEW_VERSION_3_3);
  GLEW_CHECK(GLEW_ARB_shading_language_100);
  GLEW_CHECK(GLEW_ARB_shader_objects);
  GLEW_CHECK(GLEW_ARB_vertex_shader);
  GLEW_CHECK(GLEW_ARB_fragment_shader);
  GLEW_CHECK(GLEW_ARB_framebuffer_object);
  GLEW_CHECK(GLEW_EXT_framebuffer_multisample);
#undef GLEW_CHECK
}

struct Shader {
public:
  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;
  Shader(Shader&& other) {
    *this = std::move(other);
  }

  Shader& operator=(Shader&& other) {
    if (&other == this) {
      return *this;
    }
    if (shader_) {
      glDeleteShader(shader_);
    }
    shader_ = other.shader_;
    other.shader_ = 0;
    return *this;
  }

  ~Shader() {
    if (shader_) {
      glDeleteShader(shader_);
    }
  }

  Shader(const std::string& name, std::uint32_t shader_type, const std::string& source)
  : shader_{glCreateShader(shader_type)} {
    auto source_versioned = "#version 430\n" + source;
    auto data = source_versioned.data();
    glShaderSource(shader_, 1, &data, nullptr);
    glCompileShader(shader_);

    GLint status;
    glGetShaderiv(shader_, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE) {
      return;
    }

    GLint log_length;
    glGetShaderiv(shader_, GL_INFO_LOG_LENGTH, &log_length);

    std::unique_ptr<GLchar> log{new GLchar[log_length + 1]};
    glGetShaderInfoLog(shader_, log_length, nullptr, log.get());

    std::cerr << "compile error in shader '" << name << "':\n" << log.get() << "\n";
    shader_ = 0;
  }

private:
  GLuint shader_ = 0;
  friend struct Program;
};

struct ActiveTexture {
public:
  ActiveTexture(const ActiveTexture&) = delete;
  ActiveTexture& operator=(const ActiveTexture&) = delete;
  ActiveTexture(ActiveTexture&& other) {
    *this = std::move(other);
  }

  ActiveTexture& operator=(ActiveTexture&& other) {
    if (&other == this) {
      return *this;
    }
    if (target_) {
      glBindTexture(target_, 0);
    }
    target_ = other.target_;
    other.target_ = 0;
    return *this;
  }

  ~ActiveTexture() {
    if (target_) {
      glBindTexture(target_, 0);
    }
  }

private:
  ActiveTexture(GLuint texture, GLuint target) : target_{target} {
    glBindTexture(target, texture);
  }

  GLuint target_;
  friend struct Texture;
};

struct Texture {
public:
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
  Texture(Texture&& other) {
    *this = std::move(other);
  }

  Texture& operator=(Texture&& other) {
    if (&other == this) {
      return *this;
    }
    if (target_) {
      glDeleteSamplers(1, &sampler_);
      glDeleteTextures(1, &texture_);
    }
    texture_ = other.texture_;
    sampler_ = other.sampler_;
    target_ = other.target_;
    other.texture_ = 0;
    other.sampler_ = 0;
    other.target_ = 0;
    return *this;
  }

  ~Texture() {
    if (target_) {
      glDeleteSamplers(1, &sampler_);
      glDeleteTextures(1, &texture_);
    }
  }

  Texture() {
    glGenTextures(1, &texture_);
    glGenSamplers(1, &sampler_);

    glSamplerParameteri(sampler_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler_, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }

  ActiveTexture bind() const {
    return {texture_, target_};
  }

  void create_1d(GLsizei width, GLuint components, const float* data) {
    target_ = GL_TEXTURE_1D;
    auto active = bind();
    auto internal_format = components == 3 ? GL_RGB8 : components == 1 ? GL_R32F : 0;
    auto format = components == 3 ? GL_RGB : components == 1 ? GL_RED : 0;
    glTexImage1D(target_, 0, internal_format, width, 0, format, GL_FLOAT, data);
  }

  void create_2d(const glm::ivec2& dimensions, GLuint components, const float* data) {
    target_ = GL_TEXTURE_2D;
    auto active = bind();
    auto internal_format = components == 3 ? GL_RGB8 : components == 1 ? GL_R32F : 0;
    auto format = components == 3 ? GL_RGB : components == 1 ? GL_RED : 0;
    glTexImage2D(target_, 0, internal_format, dimensions.x, dimensions.y, 0, format, GL_FLOAT,
                 data);
  }

  void create_rgba(const glm::ivec2& dimensions, GLint samples = 1) {
    if (samples > 1) {
      target_ = GL_TEXTURE_2D_MULTISAMPLE;
      auto active = bind();
      glTexImage2DMultisample(target_, samples, GL_RGBA8, dimensions.x, dimensions.y, false);
    } else {
      target_ = GL_TEXTURE_2D;
      auto active = bind();
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, dimensions.x, dimensions.y, 0, GL_RGBA,
                   GL_UNSIGNED_INT_8_8_8_8_REV, nullptr);
    }
  }

  void create_depth_stencil(const glm::ivec2& dimensions, GLint samples = 1) {
    if (samples > 1) {
      target_ = GL_TEXTURE_2D_MULTISAMPLE;
      auto active = bind();
      glTexImage2DMultisample(target_, samples, GL_DEPTH24_STENCIL8, dimensions.x, dimensions.y,
                              false);
    } else {
      target_ = GL_TEXTURE_2D;
      auto active = bind();
      glTexImage2D(target_, 0, GL_DEPTH24_STENCIL8, dimensions.x, dimensions.y, 0, GL_DEPTH_STENCIL,
                   GL_UNSIGNED_INT_24_8, nullptr);
    }
  }

  void attach_to_framebuffer(GLuint attachment) const {
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, target_, texture_, 0);
  }

private:
  GLuint texture_ = 0;
  GLuint sampler_ = 0;
  GLuint target_ = 0;
  friend struct ActiveProgram;
};

struct ActiveProgram {
public:
  ActiveProgram(const ActiveProgram&) = delete;
  ActiveProgram& operator=(const ActiveProgram&) = delete;
  ActiveProgram(ActiveProgram&& other) {
    *this = std::move(other);
  }

  ActiveProgram& operator=(ActiveProgram&& other) {
    if (&other == this) {
      return *this;
    }
    if (program_) {
      glUseProgram(0);
    }
    program_ = other.program_;
    texture_index_ = other.texture_index_;
    other.program_ = 0;
    other.texture_index_ = 0;
    return *this;
  }

  ~ActiveProgram() {
    if (program_) {
      glUseProgram(0);
    }
  }

  GLuint uniform(const std::string& name) const {
    return glGetUniformLocation(program_, name.c_str());
  }

  void uniform_texture(const std::string& name, const Texture& texture) const {
    glUniform1i(uniform(name), texture_index_);
    glActiveTexture(GL_TEXTURE0 + texture_index_);
    glBindTexture(texture.target_, texture.texture_);
    glBindSampler(texture_index_, texture.sampler_);
    ++texture_index_;
  }

private:
  ActiveProgram(GLuint program) : program_{program} {
    glUseProgram(program_);
  }

  GLuint program_ = 0;
  mutable GLuint texture_index_ = 0;
  friend struct Program;
};

struct Program {
public:
  Program(const Program&) = delete;
  Program& operator=(const Program&) = delete;
  Program(Program&& other) {
    *this = std::move(other);
  }

  Program& operator=(Program&& other) {
    if (&other == this) {
      return *this;
    }
    if (program_) {
      glDeleteProgram(program_);
    }
    program_ = other.program_;
    other.program_ = 0;
    return *this;
  }

  ~Program() {
    if (program_) {
      glDeleteProgram(program_);
    }
  }

  Program(const std::string& name, Shader&& a, Shader&& b) {
    std::vector<Shader> shaders;
    shaders.emplace_back(std::move(a));
    shaders.emplace_back(std::move(b));
    compile(name, shaders);
  }

  Program(const std::string& name, const std::vector<Shader>& shaders) {
    compile(name, shaders);
  }

  ActiveProgram use() const {
    return {program_};
  }

private:
  void compile(const std::string& name, const std::vector<Shader>& shaders) {
    program_ = glCreateProgram();

    for (const auto& shader : shaders) {
      if (shader.shader_ == 0) {
        return;
      }
    }

    for (const auto& shader : shaders) {
      glAttachShader(program_, shader.shader_);
    }
    glLinkProgram(program_);

    GLint status;
    glGetProgramiv(program_, GL_LINK_STATUS, &status);
    for (const auto& shader : shaders) {
      glDetachShader(program_, shader.shader_);
    }
    if (status == GL_TRUE) {
      return;
    }

    GLint log_length;
    glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &log_length);

    std::unique_ptr<GLchar> log{new GLchar[log_length + 1]};
    glGetProgramInfoLog(program_, log_length, nullptr, log.get());

    std::cerr << "link error in program '" << name << "':\n" << log.get() << "\n";
    return;
  }

  GLuint program_ = 0;
};

struct ActiveFramebuffer {
public:
  ActiveFramebuffer(const ActiveFramebuffer&) = delete;
  ActiveFramebuffer& operator=(const ActiveFramebuffer&) = delete;
  ActiveFramebuffer(ActiveFramebuffer&& other) {
    *this = std::move(other);
  }

  ActiveFramebuffer& operator=(ActiveFramebuffer&& other) {
    if (&other == this) {
      return *this;
    }
    if (valid_) {
      if (read_) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      } else {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      }
    }
    valid_ = other.valid_;
    read_ = other.read_;
    other.valid_ = false;
    other.read_ = false;
    return *this;
  }

  ~ActiveFramebuffer() {
    if (valid_) {
      if (read_) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      } else {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      }
    }
  }

private:
  ActiveFramebuffer(GLuint fbo, bool read) : valid_{true}, read_{read} {
    if (read_) {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    } else {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    }
  }

  bool valid_ = false;
  bool read_ = false;
  friend struct Framebuffer;
};

struct Framebuffer {
public:
  Framebuffer(const Framebuffer&) = delete;
  Framebuffer& operator=(const Framebuffer&) = delete;
  Framebuffer(Framebuffer&& other) {
    *this = std::move(other);
  }

  Framebuffer& operator=(Framebuffer&& other) {
    if (&other == this) {
      return *this;
    }
    if (framebuffer_) {
      glDeleteFramebuffers(1, &framebuffer_);
    }
    multisampled_ = other.multisampled_;
    framebuffer_ = other.framebuffer_;
    rgba_texture_ = std::move(other.rgba_texture_);
    depth_stencil_texture_ = std::move(other.depth_stencil_texture_);
    other.multisampled_ = false;
    other.framebuffer_ = 0;
    return *this;
  }

  ~Framebuffer() {
    if (framebuffer_) {
      glDeleteFramebuffers(1, &framebuffer_);
    }
  }

  Framebuffer(const glm::ivec2& dimensions, bool depth_stencil, bool attempt_multisampling)
  : multisampled_{false} {
    GLint samples = 1;
    if (attempt_multisampling) {
      glGetIntegerv(GL_MAX_SAMPLES, &samples);
    }
    multisampled_ = samples > 1;

    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

    rgba_texture_.create_rgba(dimensions, samples);
    rgba_texture_.attach_to_framebuffer(GL_COLOR_ATTACHMENT0);
    if (depth_stencil) {
      depth_stencil_texture_.reset(new Texture);
      depth_stencil_texture_->create_depth_stencil(dimensions, samples);
      depth_stencil_texture_->attach_to_framebuffer(GL_DEPTH_STENCIL_ATTACHMENT);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      std::cerr << "intermediate framebuffer is not complete\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  bool is_multisampled() const {
    return multisampled_;
  }

  ActiveFramebuffer draw() const {
    return {framebuffer_, false};
  }

  ActiveFramebuffer read() const {
    return {framebuffer_, true};
  }

  const Texture& texture() const {
    return rgba_texture_;
  }

private:
  bool multisampled_ = false;
  GLuint framebuffer_ = 0;
  Texture rgba_texture_;
  std::unique_ptr<Texture> depth_stencil_texture_;
};

struct VertexData {
public:
  VertexData(const VertexData&) = delete;
  VertexData& operator=(const VertexData&) = delete;
  VertexData(VertexData&& other) {
    *this = std::move(other);
  }

  VertexData& operator=(VertexData&& other) {
    if (&other == this) {
      return *this;
    }
    if (vbo_) {
      glDeleteBuffers(1, &vbo_);
    }
    if (ibo_) {
      glDeleteBuffers(1, &ibo_);
    }
    if (vao_) {
      glDeleteVertexArrays(1, &vao_);
    }
    size_ = other.size_;
    vbo_ = other.vbo_;
    ibo_ = other.ibo_;
    vao_ = other.vao_;
    other.size_ = 0;
    other.vbo_ = 0;
    other.ibo_ = 0;
    other.vao_ = 0;
    return *this;
  }

  ~VertexData() {
    if (vbo_) {
      glDeleteBuffers(1, &vbo_);
    }
    if (ibo_) {
      glDeleteBuffers(1, &ibo_);
    }
    if (vao_) {
      glDeleteVertexArrays(1, &vao_);
    }
  }

  VertexData(const std::vector<GLfloat>& data, const std::vector<GLushort>& indices, GLuint hint)
  : size_{static_cast<GLuint>(indices.size())} {
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * data.size(), data.data(), hint);

    glGenBuffers(1, &ibo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices.size(), indices.data(), hint);

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  void enable_attribute(GLuint location, GLuint count, GLuint stride, GLuint offset) const {
    glBindVertexArray(vao_);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glVertexAttribPointer(location, count, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat),
                          reinterpret_cast<void*>(sizeof(float) * offset));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  void draw() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, size_, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
  }

private:
  GLuint size_ = 0;
  GLuint vbo_ = 0;
  GLuint ibo_ = 0;
  GLuint vao_ = 0;
};

}  // ::glo

#endif
