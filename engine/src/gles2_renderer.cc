#include "gles2_renderer.h"

#include <stdio.h>
#include <string.h>

namespace {

const size_t kMaxBatchQuads = 2048;

GLuint CompileShader(GLenum type, const char* source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  GLint ok = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (ok == GL_TRUE) return shader;

  char log[512];
  GLsizei len = 0;
  glGetShaderInfoLog(shader, sizeof(log), &len, log);
  fprintf(stderr, "shader compile failed: %s\n", log);
  glDeleteShader(shader);
  return 0;
}

GLuint LinkProgram(GLuint vertex_shader, GLuint fragment_shader) {
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  GLint ok = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (ok == GL_TRUE) return program;

  char log[512];
  GLsizei len = 0;
  glGetProgramInfoLog(program, sizeof(log), &len, log);
  fprintf(stderr, "program link failed: %s\n", log);
  glDeleteProgram(program);
  return 0;
}

double Percentile(double* values, int count, double p) {
  if (count <= 0) return 0.0;
  for (int i = 1; i < count; ++i) {
    double v = values[i];
    int j = i - 1;
    while (j >= 0 && values[j] > v) {
      values[j + 1] = values[j];
      --j;
    }
    values[j + 1] = v;
  }
  int index = static_cast<int>((count - 1) * p);
  if (index < 0) index = 0;
  if (index >= count) index = count - 1;
  return values[index];
}

}  // namespace

GlesTexture::GlesTexture() : id(0), width(0), height(0) {}

Gles2Renderer::Gles2Renderer()
    : width_(0),
      height_(0),
      program_(0),
      vertex_buffer_(0),
      pos_loc_(-1),
      uv_loc_(-1),
      color_loc_(-1),
      texture_loc_(-1),
      current_texture_(0),
      current_blend_mode_(kGlesBlendOpaque) {
  memset(&stats_, 0, sizeof(stats_));
}

Gles2Renderer::~Gles2Renderer() {
  DestroyTexture(&white_texture_);
  if (vertex_buffer_) glDeleteBuffers(1, &vertex_buffer_);
  if (program_) glDeleteProgram(program_);
}

bool Gles2Renderer::Init() {
  if (!InitShaders()) return false;
  glGenBuffers(1, &vertex_buffer_);
  vertices_.reserve(kMaxBatchQuads * 6);
  return InitWhiteTexture();
}

void Gles2Renderer::Resize(int width, int height) {
  width_ = width;
  height_ = height;
  glViewport(0, 0, width_, height_);
}

void Gles2Renderer::BeginFrame(GlesColor clear_color) {
  memset(&stats_, 0, sizeof(stats_));
  current_texture_ = 0;
  current_blend_mode_ = kGlesBlendOpaque;
  vertices_.clear();
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(program_);
  glUniform1i(texture_loc_, 0);
}

void Gles2Renderer::EndFrame() { Flush(); }

GlesTexture Gles2Renderer::CreateTexture(int width, int height,
                                         const uint32_t* rgba_pixels,
                                         bool nearest) {
  GlesTexture texture;
  texture.width = width;
  texture.height = height;
  glGenTextures(1, &texture.id);
  glBindTexture(GL_TEXTURE_2D, texture.id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  nearest ? GL_NEAREST : GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  nearest ? GL_NEAREST : GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, rgba_pixels);
  ++stats_.texture_uploads;
  return texture;
}

void Gles2Renderer::UpdateTexture(const GlesTexture& texture,
                                  const uint32_t* rgba_pixels) {
  if (!texture.id) return;
  Flush();
  glBindTexture(GL_TEXTURE_2D, texture.id);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.width, texture.height,
                  GL_RGBA, GL_UNSIGNED_BYTE, rgba_pixels);
  ++stats_.texture_uploads;
}

void Gles2Renderer::DestroyTexture(GlesTexture* texture) {
  if (!texture || !texture->id) return;
  glDeleteTextures(1, &texture->id);
  texture->id = 0;
  texture->width = 0;
  texture->height = 0;
}

void Gles2Renderer::FillRect(GlesRect rect, GlesColor color) {
  UseTexture(white_texture_.id);
  PushQuad(rect, GlesRect{0.0f, 0.0f, 1.0f, 1.0f}, color);
}

void Gles2Renderer::DrawTexture(const GlesTexture& texture, GlesRect dst,
                                GlesRect src, GlesColor tint) {
  if (!texture.id || texture.width <= 0 || texture.height <= 0) return;
  UseTexture(texture.id);
  GlesRect uv;
  uv.x = src.x / static_cast<float>(texture.width);
  uv.y = src.y / static_cast<float>(texture.height);
  uv.w = src.w / static_cast<float>(texture.width);
  uv.h = src.h / static_cast<float>(texture.height);
  PushQuad(dst, uv, tint);
}

void Gles2Renderer::SetBlendMode(GlesBlendMode blend_mode) {
  if (current_blend_mode_ == blend_mode) return;
  Flush();
  current_blend_mode_ = blend_mode;
  if (current_blend_mode_ == kGlesBlendAlpha) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    glDisable(GL_BLEND);
  }
}

void Gles2Renderer::Flush() {
  if (vertices_.empty()) return;
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex),
               &vertices_[0], GL_STREAM_DRAW);
  glEnableVertexAttribArray(pos_loc_);
  glEnableVertexAttribArray(uv_loc_);
  glEnableVertexAttribArray(color_loc_);
  glVertexAttribPointer(pos_loc_, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
  glVertexAttribPointer(uv_loc_, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void*>(sizeof(float) * 2));
  glVertexAttribPointer(color_loc_, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void*>(sizeof(float) * 4));
  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));
  ++stats_.draw_calls;
  stats_.vertices += static_cast<int>(vertices_.size());
  vertices_.clear();
}

bool Gles2Renderer::InitShaders() {
  const char* vertex_source =
      "attribute vec2 a_pos;\n"
      "attribute vec2 a_uv;\n"
      "attribute vec4 a_color;\n"
      "varying vec2 v_uv;\n"
      "varying vec4 v_color;\n"
      "void main(){\n"
      "  gl_Position = vec4(a_pos, 0.0, 1.0);\n"
      "  v_uv = a_uv;\n"
      "  v_color = a_color;\n"
      "}\n";
  const char* fragment_source =
#if defined(PEPPERCADE_DESKTOP_GL)
      "varying vec2 v_uv;\n"
      "varying vec4 v_color;\n"
      "uniform sampler2D u_texture;\n"
      "void main(){ gl_FragColor = texture2D(u_texture, v_uv) * v_color; }\n";
#else
      "precision mediump float;\n"
      "varying vec2 v_uv;\n"
      "varying vec4 v_color;\n"
      "uniform sampler2D u_texture;\n"
      "void main(){ gl_FragColor = texture2D(u_texture, v_uv) * v_color; }\n";
#endif

  GLuint vs = CompileShader(GL_VERTEX_SHADER, vertex_source);
  GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragment_source);
  if (!vs || !fs) return false;
  program_ = LinkProgram(vs, fs);
  glDeleteShader(vs);
  glDeleteShader(fs);
  if (!program_) return false;
  pos_loc_ = glGetAttribLocation(program_, "a_pos");
  uv_loc_ = glGetAttribLocation(program_, "a_uv");
  color_loc_ = glGetAttribLocation(program_, "a_color");
  texture_loc_ = glGetUniformLocation(program_, "u_texture");
  return pos_loc_ >= 0 && uv_loc_ >= 0 && color_loc_ >= 0 && texture_loc_ >= 0;
}

bool Gles2Renderer::InitWhiteTexture() {
  const uint32_t white = 0xffffffffu;
  white_texture_ = CreateTexture(1, 1, &white, true);
  return white_texture_.id != 0;
}

void Gles2Renderer::PushQuad(GlesRect dst, GlesRect uv, GlesColor color) {
  if (vertices_.size() + 6 > kMaxBatchQuads * 6) Flush();
  const float x0 = ToClipX(dst.x);
  const float y0 = ToClipY(dst.y);
  const float x1 = ToClipX(dst.x + dst.w);
  const float y1 = ToClipY(dst.y + dst.h);
  const float u0 = uv.x;
  const float v0 = uv.y;
  const float u1 = uv.x + uv.w;
  const float v1 = uv.y + uv.h;
  const Vertex quad[6] = {
      {x0, y0, u0, v0, color.r, color.g, color.b, color.a},
      {x1, y0, u1, v0, color.r, color.g, color.b, color.a},
      {x0, y1, u0, v1, color.r, color.g, color.b, color.a},
      {x0, y1, u0, v1, color.r, color.g, color.b, color.a},
      {x1, y0, u1, v0, color.r, color.g, color.b, color.a},
      {x1, y1, u1, v1, color.r, color.g, color.b, color.a},
  };
  vertices_.insert(vertices_.end(), quad, quad + 6);
  ++stats_.quads;
}

void Gles2Renderer::UseTexture(GLuint texture_id) {
  if (current_texture_ == texture_id) return;
  Flush();
  current_texture_ = texture_id;
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, current_texture_);
}

float Gles2Renderer::ToClipX(float x) const {
  return width_ > 0 ? (x / static_cast<float>(width_)) * 2.0f - 1.0f : -1.0f;
}

float Gles2Renderer::ToClipY(float y) const {
  return height_ > 0 ? 1.0f - (y / static_cast<float>(height_)) * 2.0f : 1.0f;
}

FrameMetrics::FrameMetrics() { Reset(0.0); }

void FrameMetrics::Reset(double now_ms) {
  window_start_ms_ = now_ms;
  frame_time_count_ = 0;
  frames_ = 0;
  fps_ = 0.0;
  p50_frame_ms_ = 0.0;
  p95_frame_ms_ = 0.0;
  last_frame_ms_ = 0.0;
}

bool FrameMetrics::AddFrame(double now_ms, double frame_ms) {
  last_frame_ms_ = frame_ms;
  if (frame_time_count_ < static_cast<int>(sizeof(frame_times_) / sizeof(double))) {
    frame_times_[frame_time_count_++] = frame_ms;
  }
  ++frames_;
  const double elapsed = now_ms - window_start_ms_;
  if (elapsed < 1000.0) return false;

  double sorted[128];
  memcpy(sorted, frame_times_, sizeof(double) * frame_time_count_);
  fps_ = frames_ * 1000.0 / elapsed;
  p50_frame_ms_ = Percentile(sorted, frame_time_count_, 0.50);
  memcpy(sorted, frame_times_, sizeof(double) * frame_time_count_);
  p95_frame_ms_ = Percentile(sorted, frame_time_count_, 0.95);
  frames_ = 0;
  frame_time_count_ = 0;
  window_start_ms_ = now_ms;
  return true;
}
