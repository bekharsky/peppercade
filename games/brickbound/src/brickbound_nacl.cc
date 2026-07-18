#include "brickbound_game.h"

#include "engine/src/debug_overlay.h"
#include "engine/src/game_loop.h"
#include "engine/src/gles2_renderer.h"
#include "engine/src/input_state.h"
#include "engine/src/nacl_audio_engine.h"
#include "engine/src/nacl_input_adapter.h"
#include "engine/src/nacl_gl_context.h"
#include "engine/src/nacl_time.h"

#include <ppapi/c/ppb_graphics_3d.h>
#include <ppapi/cpp/input_event.h>
#include <ppapi/cpp/graphics_3d.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/view.h>
#include <ppapi/lib/gl/gles2/gl2ext_ppapi.h>
#include <ppapi/utility/completion_callback_factory.h>

#include <stdio.h>

class NativeArkanoidInstance : public pp::Instance {
 public:
  explicit NativeArkanoidInstance(PP_Instance instance)
      : pp::Instance(instance),
        callbacks_(this),
        clock_(1000.0 / 60.0),
        input_adapter_(&input_),
        width_(0),
        height_(0),
        last_frame_ms_(0.0),
        running_(false) {
    RequestInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
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
      pp::Module::Get()->core()->CallOnMainThread(
          320, callbacks_.NewCallback(&NativeArkanoidInstance::StartLoop), 0);
    }
  }

  bool HandleInputEvent(const pp::InputEvent& event) {
    bool handled = input_adapter_.HandleInputEvent(event);
    if (IsImmediateBack(event)) {
      game_.TogglePauseFromSystem();
      input_.BeginFrame();
      return true;
    }
    if (IsImmediatePausedMuteToggle(event)) {
      game_.ToggleSoundMute();
      input_.BeginFrame();
      return true;
    }
    return handled;
  }

 private:
  void StartLoop(int32_t) {
    if (running_ || context_.is_null()) return;
    running_ = true;
    last_frame_ms_ = peppercade::NaclNowMs();
    clock_.Reset(last_frame_ms_);
    MainLoop(0);
  }

  void MainLoop(int32_t) {
    if (!running_ || context_.is_null()) return;
    double now = peppercade::NaclNowMs();
    double dt_ms = now - last_frame_ms_;
    if (dt_ms < 0.0 || dt_ms > 100.0) dt_ms = 16.0;
    last_frame_ms_ = now;

    int steps = clock_.BeginFrame(now);
    for (int i = 0; i < steps; ++i) {
      game_.Update(clock_.step_ms(), input_);
      audio_.SetEnabled(!game_.sound_muted());
      PlayPendingSounds();
      input_.Advance(clock_.step_ms());
      clock_.ConsumeStep();
    }
    if (steps == 0) audio_.SetEnabled(!game_.sound_muted());
    audio_.Update();
    game_.Render(&renderer_);
    metrics_.AddFrame(now, dt_ms);
    debug_overlay::DrawFps(&renderer_, metrics_.fps());
    renderer_.EndFrame();
    if (steps > 0) input_.BeginFrame();
    context_.SwapBuffers(
        callbacks_.NewCallback(&NativeArkanoidInstance::MainLoop));
  }

  void PlayPendingSounds() {
    arcade::PlaySoundEvents(game_.ConsumeSoundEvents(), &audio_);
  }

  bool IsImmediateBack(const pp::InputEvent& event) {
    const PP_InputEvent_Type type = event.GetType();
    if (type != PP_INPUTEVENT_TYPE_KEYDOWN &&
        type != PP_INPUTEVENT_TYPE_RAWKEYDOWN) {
      return false;
    }
    pp::KeyboardInputEvent key(event);
    const uint32_t code = key.GetKeyCode();
    return code == 8 || code == 461 || code == 10009 || code == 19 ||
           code == 65307;
  }

  bool IsImmediatePausedMuteToggle(const pp::InputEvent& event) {
    const PP_InputEvent_Type type = event.GetType();
    if (type != PP_INPUTEVENT_TYPE_KEYDOWN &&
        type != PP_INPUTEVENT_TYPE_RAWKEYDOWN) {
      return false;
    }
    pp::KeyboardInputEvent key(event);
    const uint32_t code = key.GetKeyCode();
    return code == 40 || code == 65364;
  }

  pp::CompletionCallbackFactory<NativeArkanoidInstance> callbacks_;
  pp::Graphics3D context_;
  FixedStepClock clock_;
  InputState input_;
  NaclInputAdapter input_adapter_;
  NaclAudioEngine audio_;
  Gles2Renderer renderer_;
  FrameMetrics metrics_;
  ArkanoidGlesGame game_;
  int width_;
  int height_;
  double last_frame_ms_;
  bool running_;
};

class NativeArkanoidModule : public pp::Module {
 public:
  pp::Instance* CreateInstance(PP_Instance instance) {
    return new NativeArkanoidInstance(instance);
  }
};

namespace pp {
Module* CreateModule() { return new NativeArkanoidModule(); }
}  // namespace pp
