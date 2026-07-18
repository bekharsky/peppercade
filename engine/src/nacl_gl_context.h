#ifndef CLASSIC_GAME_KIT_NACL_GL_CONTEXT_H_
#define CLASSIC_GAME_KIT_NACL_GL_CONTEXT_H_

#include <ppapi/c/ppb_graphics_3d.h>
#include <ppapi/cpp/graphics_3d.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/lib/gl/gles2/gl2ext_ppapi.h>

#include <stdio.h>

namespace peppercade {

// Creates the exact GLES2 context used by every Samsung Pepper 47 target.
// Keeping the attributes here prevents standalone games, the pack, and the
// benchmark from silently drifting apart.
inline bool InitNaclGlContext(pp::Instance* instance, int width, int height,
                              pp::Graphics3D* context) {
  if (!instance || !context) return false;
  if (!glInitializePPAPI(pp::Module::Get()->get_browser_interface())) {
    fprintf(stderr, "glInitializePPAPI failed\n");
    return false;
  }
  const int32_t attrs[] = {
      PP_GRAPHICS3DATTRIB_ALPHA_SIZE, 0,
      PP_GRAPHICS3DATTRIB_DEPTH_SIZE, 0,
      PP_GRAPHICS3DATTRIB_STENCIL_SIZE, 0,
      PP_GRAPHICS3DATTRIB_SWAP_BEHAVIOR,
      PP_GRAPHICS3DATTRIB_BUFFER_DESTROYED,
      PP_GRAPHICS3DATTRIB_GPU_PREFERENCE,
      PP_GRAPHICS3DATTRIB_GPU_PREFERENCE_PERFORMANCE,
      PP_GRAPHICS3DATTRIB_FULLSCREEN_SAMSUNG, 1,
      PP_GRAPHICS3DATTRIB_WIDTH, width,
      PP_GRAPHICS3DATTRIB_HEIGHT, height,
      PP_GRAPHICS3DATTRIB_NONE};
  *context = pp::Graphics3D(instance, attrs);
  if (!instance->BindGraphics(*context)) {
    fprintf(stderr, "BindGraphics 3D failed\n");
    *context = pp::Graphics3D();
    return false;
  }
  glSetCurrentContextPPAPI(context->pp_resource());
  return true;
}

}  // namespace peppercade

#endif  // CLASSIC_GAME_KIT_NACL_GL_CONTEXT_H_
