#include "engine/src/debug_overlay.h"
#include "engine/src/game_loop.h"
#include "engine/src/gles2_renderer.h"
#include "engine/src/game_catalog.h"
#include "engine/src/game_hub.h"
#include "engine/src/input_state.h"
#include "engine/src/nacl_audio_engine.h"
#include "engine/src/nacl_input_adapter.h"
#include "engine/src/nacl_gl_context.h"
#include "engine/src/nacl_time.h"
#include "games/2048/src/game_2048.h"
#include "games/bubble-orbit/src/bubble_orbit_game.h"
#include "games/brickbound/src/brickbound_game.h"
#include "games/neon-serpent/src/neon_serpent_game.h"
#include "games/dragoban/src/dragoban_game.h"
#include "games/stackfall/src/stackfall_core.h"
#include "games/lightline/src/lightline_game.h"

#include <ppapi/c/ppb_graphics_3d.h>
#include <ppapi/cpp/graphics_3d.h>
#include <ppapi/cpp/input_event.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/view.h>
#include <ppapi/lib/gl/gles2/gl2ext_ppapi.h>
#include <ppapi/utility/completion_callback_factory.h>

#include <stdint.h>
#include <stdio.h>

namespace {

enum ShellMode { kModeHub, kModePlaying, kModePaused };
using peppercade::GameCatalog;
using peppercade::GameId;
using peppercade::NextGame;
using namespace peppercade;

}  // namespace

class NativePeppercadeInstance : public pp::Instance {
 public:
  explicit NativePeppercadeInstance(PP_Instance instance)
      : pp::Instance(instance),
        callbacks_(this),
        clock_(1000.0 / 60.0),
        input_adapter_(&input_),
        mode_(kModeHub),
        selected_(kGameTetris),
        current_(kGameTetris),
        width_(0),
        height_(0),
        hub_variant_(0),
        running_(false) {
    RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
    InitializeGames();
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
    ResizeGames();

    if (first_view) {
      pp::Module::Get()->core()->CallOnMainThread(
          320, callbacks_.NewCallback(&NativePeppercadeInstance::StartLoop), 0);
    }
  }

  bool HandleInputEvent(const pp::InputEvent& event) {
    if (!input_adapter_.HandleInputEvent(event)) return false;
    if (!IsKeyDown(event)) return true;

    pp::KeyboardInputEvent key(event);
    const uint32_t code = key.GetKeyCode();

    if (mode_ == kModeHub) {
      if (IsUp(code)) {
        selected_ = NextGame(selected_, -1);
        audio_.Play(kSfxPause);
      } else if (IsDown(code)) {
        selected_ = NextGame(selected_, 1);
        audio_.Play(kSfxPause);
      } else if (IsPrimary(code)) {
        StartSelectedGame();
      }
      input_.BeginFrame();
      return true;
    }

    if (mode_ == kModePaused) {
      if (IsBack(code)) {
        mode_ = kModeHub;
        ++hub_variant_;
        SetCurrentPaused(false);
        audio_.Play(kSfxPause);
      } else if (IsPrimary(code)) {
        SetCurrentPaused(false);
        mode_ = kModePlaying;
      } else if (IsDown(code)) {
        ToggleCurrentSound();
      } else if (IsUp(code)) {
        CycleCurrentDifficulty();
      }
      input_.BeginFrame();
      return true;
    }

    if (mode_ == kModePlaying && IsBack(code)) {
      SetCurrentPaused(true);
      mode_ = kModePaused;
      input_.BeginFrame();
      return true;
    }

    return true;
  }

 private:
  void StartLoop(int32_t) {
    if (running_ || context_.is_null()) return;
    running_ = true;
    hub_variant_ = static_cast<int>(peppercade::NaclNowMs()) & 31;
    clock_.Reset(peppercade::NaclNowMs());
    MainLoop(0);
  }

  void MainLoop(int32_t) {
    if (!running_ || context_.is_null()) return;
    const double now = peppercade::NaclNowMs();
    const int steps = clock_.BeginFrame(now);
    for (int i = 0; i < steps; ++i) {
      Update(clock_.step_ms());
      input_.Advance(clock_.step_ms());
      clock_.ConsumeStep();
    }
    audio_.Update();
    Render(now);
    metrics_.AddFrame(now, clock_.last_frame_ms());
    context_.SwapBuffers(
        callbacks_.NewCallback(&NativePeppercadeInstance::MainLoop));
    if (steps > 0) input_.BeginFrame();
  }

  void ResizeGames() {
    games_.ResizeAll(width_, height_);
  }

  void StartSelectedGame() {
    current_ = selected_;
    mode_ = kModePlaying;
    ResetCurrent();
    SetCurrentPaused(false);
    input_.BeginFrame();
    audio_.Play(kSfxLevel);
  }

  void ResetCurrent() {
    CurrentGame()->Reset();
    ResizeGames();
  }

  void Update(double dt_ms) {
    if (mode_ == kModeHub) return;
    CurrentGame()->Update(dt_ms, input_);
    audio_.SetEnabled(!CurrentGame()->sound_muted());
    PlayPendingSounds();
  }

  void Render(double) {
    if (mode_ == kModeHub)
      peppercade::DrawGameHub(&renderer_, width_, height_, hub_variant_,
                              selected_);
    else CurrentGame()->Render(&renderer_);
    debug_overlay::DrawFps(&renderer_, metrics_.fps());
    renderer_.EndFrame();
  }

  void SetCurrentPaused(bool paused) {
    CurrentGame()->SetPaused(paused);
  }

  void ToggleCurrentSound() {
    Game* game = CurrentGame();
    game->ToggleSoundMute();
    audio_.SetEnabled(!game->sound_muted());
  }

  void CycleCurrentDifficulty() {
    CurrentGame()->CycleDifficulty();
  }

  void PlayPendingSounds() {
    arcade::PlaySoundEvents(CurrentGame()->ConsumeSoundEvents(), &audio_);
  }

  bool IsKeyDown(const pp::InputEvent& event) const {
    const PP_InputEvent_Type type = event.GetType();
    return type == PP_INPUTEVENT_TYPE_KEYDOWN ||
           type == PP_INPUTEVENT_TYPE_RAWKEYDOWN;
  }

  bool IsUp(uint32_t code) const { return code == 38 || code == 65362; }
  bool IsDown(uint32_t code) const { return code == 40 || code == 65364; }
  bool IsPrimary(uint32_t code) const {
    return code == 13 || code == 65293 || code == 32;
  }
  bool IsBack(uint32_t code) const {
    return code == 8 || code == 461 || code == 10009 || code == 19 ||
           code == 65307;
  }

  void InitializeGames() {
    games_.Set(kGameTetris, &tetris_);
    games_.Set(kGameArkanoid, &arkanoid_);
    games_.Set(kGame2048, &game2048_);
    games_.Set(kGameSnake, &snake_);
    games_.Set(kGameTron, &tron_);
    games_.Set(kGameSokoban, &sokoban_);
    games_.Set(kGameBubbleShooter, &bubble_orbit_);
  }

  Game* CurrentGame() { return games_.Get(current_); }

  pp::CompletionCallbackFactory<NativePeppercadeInstance> callbacks_;
  pp::Graphics3D context_;
  FixedStepClock clock_;
  InputState input_;
  NaclInputAdapter input_adapter_;
  NaclAudioEngine audio_;
  Gles2Renderer renderer_;
  FrameMetrics metrics_;
  TetrisGame tetris_;
  ArkanoidGlesGame arkanoid_;
  Game2048 game2048_;
  SnakeGame snake_;
  TronGame tron_;
  SokobanGame sokoban_;
  BubbleShooterGame bubble_orbit_;
  GameCatalog games_;
  ShellMode mode_;
  GameId selected_;
  GameId current_;
  int width_;
  int height_;
  int hub_variant_;
  bool running_;
};

class NativePeppercadeModule : public pp::Module {
 public:
  pp::Instance* CreateInstance(PP_Instance instance) {
    return new NativePeppercadeInstance(instance);
  }
};

namespace pp {
Module* CreateModule() { return new NativePeppercadeModule(); }
}  // namespace pp
