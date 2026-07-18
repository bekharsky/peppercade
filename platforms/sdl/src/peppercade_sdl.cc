#include "engine/src/debug_overlay.h"
#include "engine/src/gles2_renderer.h"
#include "engine/src/game_catalog.h"
#include "engine/src/game_hub.h"
#include "engine/src/input_state.h"
#include "engine/src/sdl_audio_engine.h"
#include "games/2048/src/game_2048.h"
#include "games/bubble-orbit/src/bubble_orbit_game.h"
#include "games/brickbound/src/brickbound_game.h"
#include "games/neon-serpent/src/neon_serpent_game.h"
#include "games/dragoban/src/dragoban_game.h"
#include "games/stackfall/src/stackfall_core.h"
#include "games/lightline/src/lightline_game.h"

#include <SDL.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

#include <stdint.h>
#include <stdio.h>

namespace {

enum ShellMode { kModeHub, kModePlaying, kModePaused };
using peppercade::GameCatalog;
using peppercade::GameId;
using peppercade::NextGame;
using namespace peppercade;

double NowMs() { return static_cast<double>(SDL_GetPerformanceCounter()) * 1000.0 /
                        static_cast<double>(SDL_GetPerformanceFrequency()); }

GameAction KeyToAction(SDL_Keycode key) {
  if (key == SDLK_LEFT) return kActionLeft;
  if (key == SDLK_RIGHT) return kActionRight;
  if (key == SDLK_UP) return kActionUp;
  if (key == SDLK_DOWN) return kActionDown;
  if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
    return kActionPrimary;
  if (key == SDLK_b) return kActionSecondary;
  if (key == SDLK_ESCAPE || key == SDLK_BACKSPACE) return kActionBack;
  return kActionCount;
}

bool AllowsPlatformRepeat(GameId game) {
  return game == kGameTetris || game == kGameArkanoid;
}

}  // namespace

class PeppercadeApp {
 public:
  PeppercadeApp()
      : window_(0),
        gl_(0),
        mode_(kModeHub),
        selected_(kGameTetris),
        current_(kGameTetris),
        width_(1280),
        height_(720),
        hub_variant_(0),
        last_ms_(0.0),
#if defined(__EMSCRIPTEN__)
        first_frame_reported_(false),
#endif
        running_(true) {}

  bool Init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
      fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
      return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    window_ = SDL_CreateWindow("Peppercade", SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, width_, height_,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window_) {
      fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
      return false;
    }
    gl_ = SDL_GL_CreateContext(window_);
    if (!gl_) {
      fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
      return false;
    }
#if !defined(__EMSCRIPTEN__)
    // Browsers already pace the Emscripten main loop with requestAnimationFrame.
    // Calling this before that loop exists only produces a legacy SDK warning.
    SDL_GL_SetSwapInterval(1);
#endif
    if (!renderer_.Init()) return false;
    renderer_.Resize(width_, height_);
    InitializeGames();
    audio_.Init();
    last_ms_ = NowMs();
    hub_variant_ = static_cast<int>(SDL_GetTicks() % 31);
    return true;
  }

  void Run() {
#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(&PeppercadeApp::MainLoop, this, 0, 1);
#else
    while (running_) {
      MainLoop(this);
      SDL_Delay(1);
    }
#endif
  }

  ~PeppercadeApp() {
    audio_.Shutdown();
    if (gl_) SDL_GL_DeleteContext(gl_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
  }

 private:
  static void MainLoop(void* userdata) {
    PeppercadeApp* app = reinterpret_cast<PeppercadeApp*>(userdata);
    SDL_Event event;
    while (SDL_PollEvent(&event)) app->HandleEvent(event);
    if (!app->running_) {
#if defined(__EMSCRIPTEN__)
      emscripten_cancel_main_loop();
#endif
      return;
    }
    double now = NowMs();
    double dt_ms = now - app->last_ms_;
    if (dt_ms < 1.0) dt_ms = 16.0;
    if (dt_ms > 80.0) dt_ms = 80.0;
    app->last_ms_ = now;
    app->Update(dt_ms);
    app->Render(now);
    SDL_GL_SwapWindow(app->window_);
#if defined(__EMSCRIPTEN__)
    if (!app->first_frame_reported_) {
      app->first_frame_reported_ = true;
      EM_ASM({
        if (Module.onFirstFrame) Module.onFirstFrame();
      });
    }
#endif
  }

  void HandleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
      running_ = false;
      return;
    }
    if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
      width_ = event.window.data1;
      height_ = event.window.data2;
      renderer_.Resize(width_, height_);
      games_.ResizeAll(width_, height_);
      return;
    }
    if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP) return;
    const bool down = event.type == SDL_KEYDOWN;
    const SDL_Keycode key = event.key.keysym.sym;
    if (down && event.key.repeat) {
      if (mode_ == kModePlaying && AllowsPlatformRepeat(current_)) {
        GameAction action = KeyToAction(key);
        if (action != kActionCount) input_.RepeatAction(action);
      }
      return;
    }

    if (mode_ == kModeHub) {
      if (!down) return;
      if (key == SDLK_UP) {
        selected_ = NextGame(selected_, -1);
        audio_.Play(kSfxPause);
      } else if (key == SDLK_DOWN) {
        selected_ = NextGame(selected_, 1);
        audio_.Play(kSfxPause);
      } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER ||
                 key == SDLK_SPACE) {
        StartSelectedGame();
      }
      // Back/Escape has no parent screen to return to from the hub.
      return;
    }

    if (mode_ == kModePaused && down) {
      if (key == SDLK_ESCAPE || key == SDLK_BACKSPACE) {
        mode_ = kModeHub;
        ++hub_variant_;
        audio_.Play(kSfxPause);
        return;
      }
      if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE) {
        SetCurrentPaused(false);
        mode_ = kModePlaying;
        return;
      }
      if (key == SDLK_DOWN) {
        ToggleCurrentSound();
        return;
      }
      if (key == SDLK_UP) {
        CycleCurrentDifficulty();
        return;
      }
      return;
    }

    if (mode_ == kModePlaying && down &&
        (key == SDLK_ESCAPE || key == SDLK_BACKSPACE)) {
      SetCurrentPaused(true);
      mode_ = kModePaused;
      return;
    }

    GameAction action = KeyToAction(key);
    if (action == kActionCount) return;
    input_.SetAction(action, down);
  }

  void StartSelectedGame() {
    current_ = selected_;
    mode_ = kModePlaying;
    Game* game = CurrentGame();
    game->Reset();
    game->Resize(width_, height_);
    game->SetPaused(false);
    input_.BeginFrame();
    audio_.Play(kSfxLevel);
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

  void Update(double dt_ms) {
    if (mode_ == kModeHub) return;
    Game* game = CurrentGame();
    game->Update(dt_ms, input_);
    input_.Advance(dt_ms);
    input_.BeginFrame();
    audio_.SetEnabled(!game->sound_muted());
    PlayPendingSounds(game);
  }

  void PlayPendingSounds(Game* game) {
    arcade::PlaySoundEvents(game->ConsumeSoundEvents(), &audio_);
  }

  void Render(double now_ms) {
    if (mode_ == kModeHub) {
      peppercade::DrawGameHub(&renderer_, width_, height_, hub_variant_,
                              selected_);
    } else {
      CurrentGame()->Render(&renderer_);
    }

    renderer_.EndFrame();
  }

  void InitializeGames() {
    games_.Set(kGameTetris, &tetris_);
    games_.Set(kGameArkanoid, &arkanoid_);
    games_.Set(kGame2048, &game2048_);
    games_.Set(kGameSnake, &snake_);
    games_.Set(kGameTron, &tron_);
    games_.Set(kGameSokoban, &sokoban_);
    games_.Set(kGameBubbleShooter, &bubble_orbit_);
    games_.ResizeAll(width_, height_);
  }

  Game* CurrentGame() { return games_.Get(current_); }

  SDL_Window* window_;
  SDL_GLContext gl_;
  Gles2Renderer renderer_;
  SdlAudioEngine audio_;
  TetrisGame tetris_;
  ArkanoidGlesGame arkanoid_;
  Game2048 game2048_;
  SnakeGame snake_;
  TronGame tron_;
  SokobanGame sokoban_;
  BubbleShooterGame bubble_orbit_;
  GameCatalog games_;
  InputState input_;
  ShellMode mode_;
  GameId selected_;
  GameId current_;
  int width_;
  int height_;
  int hub_variant_;
  double last_ms_;
#if defined(__EMSCRIPTEN__)
  bool first_frame_reported_;
#endif
  bool running_;
};

int main(int, char**) {
  PeppercadeApp app;
  if (!app.Init()) return 1;
  app.Run();
  return 0;
}
