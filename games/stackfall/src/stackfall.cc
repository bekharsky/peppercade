#include "stackfall_core.h"

#include "engine/src/debug_overlay.h"
#include "engine/src/gles2_renderer.h"
#include "engine/src/input_state.h"
#include "engine/src/nacl_audio_engine.h"
#include "engine/src/nacl_input_adapter.h"
#include "engine/src/nacl_gl_context.h"
#include "engine/src/nacl_time.h"

#include <ppapi/c/ppb_graphics_3d.h>
#include <ppapi/cpp/graphics_3d.h>
#include <ppapi/cpp/input_event.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/view.h>
#include <ppapi/lib/gl/gles2/gl2ext_ppapi.h>
#include <ppapi/utility/completion_callback_factory.h>

#include <stdio.h>

namespace {

const int kTickMs = 16;

}  // namespace

class NativeTetrisInstance : public pp::Instance {
 public:
  explicit NativeTetrisInstance(PP_Instance instance)
      : pp::Instance(instance),
        callbacks_(this),
        input_adapter_(&input_),
        width_(0),
        height_(0),
        last_tick_seconds_(0.0),
        running_(false) {
    RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
  }

  bool Init(uint32_t, const char*[], const char*[]) { return true; }

  void DidChangeView(const pp::View& view) {
    int new_width = view.GetRect().width();
    int new_height = view.GetRect().height();
    if (new_width <= 0 || new_height <= 0) return;

    bool first_view = context_.is_null();
    if (first_view) {
      if (!peppercade::InitNaclGlContext(this, new_width, new_height,
                                         &context_)) return;
      if (!renderer_.Init()) return;
      audio_.Init(this);
      audio_.WarmUp();
    } else {
      context_.ResizeBuffers(new_width, new_height);
    }

    width_ = new_width;
    height_ = new_height;
    renderer_.Resize(width_, height_);
    game_.Resize(width_, height_);

    if (first_view) {
      running_ = true;
      last_tick_seconds_ = peppercade::NaclNowSeconds();
      ScheduleTick();
    }
  }

  bool HandleInputEvent(const pp::InputEvent& event) {
    return input_adapter_.HandleInputEvent(event);
  }

 private:
  void ScheduleTick() {
    pp::Module::Get()->core()->CallOnMainThread(
        kTickMs, callbacks_.NewCallback(&NativeTetrisInstance::OnTick), 0);
  }

  void OnTick(int32_t) {
    if (!running_ || context_.is_null()) return;
    double now = peppercade::NaclNowSeconds();
    int elapsed_ms = static_cast<int>((now - last_tick_seconds_) * 1000.0);
    last_tick_seconds_ = now;
    if (elapsed_ms < 1) elapsed_ms = kTickMs;
    if (elapsed_ms > 80) elapsed_ms = 80;
    game_.Update(elapsed_ms, input_);
    input_.Advance(elapsed_ms);
    input_.BeginFrame();
    audio_.SetEnabled(!game_.sound_muted());
    PlayPendingSounds();
    audio_.Update();
    game_.Render(&renderer_);
    renderer_.EndFrame();
    context_.SwapBuffers(callbacks_.NewCallback(&NativeTetrisInstance::OnSwap));
  }

  void OnSwap(int32_t) { ScheduleTick(); }

  void PlayPendingSounds() {
    arcade::PlaySoundEvents(game_.ConsumeSoundEvents(), &audio_);
  }

  pp::CompletionCallbackFactory<NativeTetrisInstance> callbacks_;
  pp::Graphics3D context_;
  Gles2Renderer renderer_;
  InputState input_;
  NaclInputAdapter input_adapter_;
  NaclAudioEngine audio_;
  TetrisGame game_;
  int width_;
  int height_;
  double last_tick_seconds_;
  bool running_;
};

class NativeTetrisModule : public pp::Module {
 public:
  pp::Instance* CreateInstance(PP_Instance instance) {
    return new NativeTetrisInstance(instance);
  }
};

namespace pp {
Module* CreateModule() { return new NativeTetrisModule(); }
}  // namespace pp
