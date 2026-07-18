#include <stdint.h>
#include <stdio.h>
#include <vector>

#include "engine/src/gles2_renderer.h"
#include "engine/src/debug_overlay.h"
#include "engine/src/game_loop.h"
#include "engine/src/input_state.h"
#include "engine/src/nacl_input_adapter.h"
#include "engine/src/nacl_gl_context.h"
#include "engine/src/nacl_time.h"

#include "ppapi/c/ppb_graphics_3d.h"
#include "ppapi/cpp/graphics_3d.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/lib/gl/gles2/gl2ext_ppapi.h"
#include "ppapi/utility/completion_callback_factory.h"

namespace {

const int kMaxSprites = 512;
const int kInitialSprites = 256;
const int kAtlasSize = 128;
const int kTileSize = 32;

struct Sprite {
  float x;
  float y;
  float vx;
  float vy;
  float size;
  float r;
  float g;
  float b;
  float alpha;
  int tile;
};

uint32_t Rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  return (static_cast<uint32_t>(a) << 24) |
         (static_cast<uint32_t>(b) << 16) |
         (static_cast<uint32_t>(g) << 8) |
         static_cast<uint32_t>(r);
}

GlesRect TileSource(int tile) {
  const int x = (tile % 4) * kTileSize;
  const int y = (tile / 4) * kTileSize;
  return GlesRect{static_cast<float>(x), static_cast<float>(y),
                  static_cast<float>(kTileSize),
                  static_cast<float>(kTileSize)};
}

}  // namespace

class GLESBenchmarkInstance : public pp::Instance {
 public:
  explicit GLESBenchmarkInstance(PP_Instance instance)
      : pp::Instance(instance),
        callbacks_(this),
        clock_(1000.0 / 60.0),
        input_adapter_(&input_),
        width_(0),
        height_(0),
        sprite_count_(kInitialSprites),
        last_frame_ms_(0.0),
        running_(false) {
    RequestInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
  }

  bool Init(uint32_t, const char*[], const char*[]) {
    for (int i = 0; i < kMaxSprites; ++i) {
      sprites_[i].size = 18.0f + static_cast<float>((i * 7) % 42);
      sprites_[i].x = static_cast<float>((i * 67) % 1600);
      sprites_[i].y = static_cast<float>((i * 131) % 900);
      sprites_[i].vx = 80.0f + static_cast<float>((i * 17) % 180);
      sprites_[i].vy = 70.0f + static_cast<float>((i * 29) % 150);
      sprites_[i].r = 0.25f + static_cast<float>((i * 37) % 75) / 100.0f;
      sprites_[i].g = 0.35f + static_cast<float>((i * 53) % 60) / 100.0f;
      sprites_[i].b = 0.45f + static_cast<float>((i * 71) % 50) / 100.0f;
      sprites_[i].alpha = (i % 5 == 0) ? 0.58f : 1.0f;
      sprites_[i].tile = i % 16;
    }
    return true;
  }

  bool HandleInputEvent(const pp::InputEvent& event) {
    return input_adapter_.HandleInputEvent(event);
  }

  void DidChangeView(const pp::View& view) {
    const int new_width = view.GetRect().width();
    const int new_height = view.GetRect().height();
    if (new_width <= 0 || new_height <= 0) return;

    if (context_.is_null()) {
      if (!peppercade::InitNaclGlContext(this, new_width, new_height,
                                         &context_)) return;
      if (!renderer_.Init()) return;
      CreateAtlas();
      running_ = true;
      last_frame_ms_ = peppercade::NaclNowMs();
      clock_.Reset(last_frame_ms_);
      metrics_.Reset(last_frame_ms_);
      MainLoop(0);
    } else {
      context_.ResizeBuffers(new_width, new_height);
    }

    width_ = new_width;
    height_ = new_height;
    renderer_.Resize(width_, height_);
  }

 private:
  void Step(float dt) {
    if (input_.Pressed(kActionLeft)) sprite_count_ -= 64;
    if (input_.Pressed(kActionRight)) sprite_count_ += 64;
    if (input_.Pressed(kActionDown)) sprite_count_ = 64;
    if (input_.Pressed(kActionUp)) sprite_count_ = kMaxSprites;
    if (sprite_count_ < 16) sprite_count_ = 16;
    if (sprite_count_ > kMaxSprites) sprite_count_ = kMaxSprites;

    for (int i = 0; i < sprite_count_; ++i) {
      Sprite& s = sprites_[i];
      s.x += s.vx * dt;
      s.y += s.vy * dt;
      if (s.x < 0.0f || s.x + s.size > width_) {
        s.vx = -s.vx;
        s.x += s.vx * dt;
      }
      if (s.y < 0.0f || s.y + s.size > height_) {
        s.vy = -s.vy;
        s.y += s.vy * dt;
      }
    }
  }

  void Render() {
    renderer_.BeginFrame(GlesColor{0.02f, 0.025f, 0.04f, 1.0f});
    renderer_.SetBlendMode(kGlesBlendOpaque);
    for (int i = 0; i < sprite_count_; ++i) {
      const Sprite& s = sprites_[i];
      if (s.alpha < 1.0f) continue;
      renderer_.DrawTexture(atlas_, GlesRect{s.x, s.y, s.size, s.size},
                            TileSource(s.tile),
                            GlesColor{s.r, s.g, s.b, 1.0f});
    }
    renderer_.SetBlendMode(kGlesBlendAlpha);
    for (int i = 0; i < sprite_count_; ++i) {
      const Sprite& s = sprites_[i];
      if (s.alpha >= 1.0f) continue;
      renderer_.DrawTexture(atlas_, GlesRect{s.x, s.y, s.size, s.size},
                            TileSource(s.tile),
                            GlesColor{s.r, s.g, s.b, s.alpha});
    }
    debug_overlay::DrawFps(&renderer_, metrics_.fps());
    renderer_.EndFrame();
  }

  void MaybeReport(double now_ms, double dt_ms) {
    if (!metrics_.AddFrame(now_ms, dt_ms)) return;
    const GlesRenderStats stats = renderer_.stats();
    pp::VarDictionary msg;
    msg.Set("app", "render-benchmark");
    msg.Set("type", "metrics");
    msg.Set("mode", "gles2");
    msg.Set("fps", metrics_.fps());
    msg.Set("frameMs", metrics_.last_frame_ms());
    msg.Set("p50FrameMs", metrics_.p50_frame_ms());
    msg.Set("p95FrameMs", metrics_.p95_frame_ms());
    msg.Set("sprites", sprite_count_);
    msg.Set("drawCalls", stats.draw_calls);
    msg.Set("quads", stats.quads);
    msg.Set("width", width_);
    msg.Set("height", height_);
    msg.Set("uploadPixels", 0);
    PostMessage(msg);
  }

  void CreateAtlas() {
    std::vector<uint32_t> pixels(kAtlasSize * kAtlasSize, 0);
    for (int tile = 0; tile < 16; ++tile) {
      const int tx = (tile % 4) * kTileSize;
      const int ty = (tile / 4) * kTileSize;
      const uint8_t base_r = static_cast<uint8_t>(80 + (tile * 37) % 150);
      const uint8_t base_g = static_cast<uint8_t>(90 + (tile * 53) % 140);
      const uint8_t base_b = static_cast<uint8_t>(100 + (tile * 71) % 130);
      for (int y = 0; y < kTileSize; ++y) {
        for (int x = 0; x < kTileSize; ++x) {
          const bool border = x < 3 || y < 3 || x >= kTileSize - 3 ||
                              y >= kTileSize - 3;
          const bool stripe = ((x + y + tile * 3) & 7) < 2;
          uint8_t r = base_r;
          uint8_t g = base_g;
          uint8_t b = base_b;
          uint8_t a = 255;
          if (border) {
            r = 245;
            g = 246;
            b = 210;
          } else if (stripe) {
            r = static_cast<uint8_t>(r / 2);
            g = static_cast<uint8_t>(g / 2);
            b = static_cast<uint8_t>(b / 2);
          }
          pixels[(ty + y) * kAtlasSize + tx + x] = Rgba(r, g, b, a);
        }
      }
    }
    atlas_ = renderer_.CreateTexture(kAtlasSize, kAtlasSize, &pixels[0], true);
  }

  void MainLoop(int32_t) {
    if (!running_ || context_.is_null()) return;
    const double now = peppercade::NaclNowMs();
    const int steps = clock_.BeginFrame(now);
    double dt_ms = now - last_frame_ms_;
    if (dt_ms < 0.0 || dt_ms > 100.0) dt_ms = 16.0;
    last_frame_ms_ = now;
    for (int i = 0; i < steps; ++i) {
      Step(static_cast<float>(clock_.step_seconds()));
      input_.Advance(clock_.step_ms());
      clock_.ConsumeStep();
    }
    Render();
    MaybeReport(now, dt_ms);
    input_.BeginFrame();
    context_.SwapBuffers(callbacks_.NewCallback(&GLESBenchmarkInstance::MainLoop));
  }

  pp::CompletionCallbackFactory<GLESBenchmarkInstance> callbacks_;
  pp::Graphics3D context_;
  FixedStepClock clock_;
  NaclInputAdapter input_adapter_;
  int width_;
  int height_;
  int sprite_count_;
  Gles2Renderer renderer_;
  FrameMetrics metrics_;
  GlesTexture atlas_;
  InputState input_;
  Sprite sprites_[kMaxSprites];
  double last_frame_ms_;
  bool running_;
};

class GLESBenchmarkModule : public pp::Module {
 public:
  pp::Instance* CreateInstance(PP_Instance instance) {
    return new GLESBenchmarkInstance(instance);
  }
};

namespace pp {
Module* CreateModule() { return new GLESBenchmarkModule(); }
}  // namespace pp
