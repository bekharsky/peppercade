#ifndef CLASSIC_GAME_KIT_GLES2_RENDERER_H_
#define CLASSIC_GAME_KIT_GLES2_RENDERER_H_

#if defined(PEPPERCADE_DESKTOP_GL)
#define GL_SILENCE_DEPRECATION 1
#include <OpenGL/gl3.h>
#else
#include <GLES2/gl2.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <vector>

struct GlesColor {
  float r;
  float g;
  float b;
  float a;
};

struct GlesRect {
  float x;
  float y;
  float w;
  float h;
};

struct GlesTexture {
  GlesTexture();

  GLuint id;
  int width;
  int height;
};

struct GlesRenderStats {
  int draw_calls;
  int quads;
  int vertices;
  int texture_uploads;
};

enum GlesBlendMode {
  kGlesBlendOpaque = 0,
  kGlesBlendAlpha
};

class Gles2Renderer {
 public:
  Gles2Renderer();
  ~Gles2Renderer();

  bool Init();
  void Resize(int width, int height);
  void BeginFrame(GlesColor clear_color);
  void EndFrame();

  GlesTexture CreateTexture(int width, int height, const uint32_t* rgba_pixels,
                            bool nearest);
  void UpdateTexture(const GlesTexture& texture, const uint32_t* rgba_pixels);
  void DestroyTexture(GlesTexture* texture);

  void FillRect(GlesRect rect, GlesColor color);
  void DrawTexture(const GlesTexture& texture, GlesRect dst, GlesRect src,
                   GlesColor tint);
  void SetBlendMode(GlesBlendMode blend_mode);
  void Flush();

  GlesRenderStats stats() const { return stats_; }
  int width() const { return width_; }
  int height() const { return height_; }

 private:
  struct Vertex {
    float x;
    float y;
    float u;
    float v;
    float r;
    float g;
    float b;
    float a;
  };

  bool InitShaders();
  bool InitWhiteTexture();
  void PushQuad(GlesRect dst, GlesRect src_uv, GlesColor color);
  void UseTexture(GLuint texture_id);
  float ToClipX(float x) const;
  float ToClipY(float y) const;

  int width_;
  int height_;
  GLuint program_;
  GLuint vertex_buffer_;
  GLint pos_loc_;
  GLint uv_loc_;
  GLint color_loc_;
  GLint texture_loc_;
  GLuint current_texture_;
  GlesBlendMode current_blend_mode_;
  GlesTexture white_texture_;
  std::vector<Vertex> vertices_;
  GlesRenderStats stats_;
};

class FrameMetrics {
 public:
  FrameMetrics();

  void Reset(double now_ms);
  bool AddFrame(double now_ms, double frame_ms);
  double fps() const { return fps_; }
  double p50_frame_ms() const { return p50_frame_ms_; }
  double p95_frame_ms() const { return p95_frame_ms_; }
  double last_frame_ms() const { return last_frame_ms_; }
  int frames() const { return frames_; }

 private:
  double window_start_ms_;
  double frame_times_[128];
  int frame_time_count_;
  int frames_;
  double fps_;
  double p50_frame_ms_;
  double p95_frame_ms_;
  double last_frame_ms_;
};

#endif  // CLASSIC_GAME_KIT_GLES2_RENDERER_H_
