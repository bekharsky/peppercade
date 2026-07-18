#ifndef CLASSIC_GAME_KIT_NACL_TIME_H_
#define CLASSIC_GAME_KIT_NACL_TIME_H_

#include <ppapi/cpp/module.h>

namespace peppercade {

inline double NaclNowSeconds() {
  return pp::Module::Get()->core()->GetTimeTicks();
}

inline double NaclNowMs() { return NaclNowSeconds() * 1000.0; }

}  // namespace peppercade

#endif  // CLASSIC_GAME_KIT_NACL_TIME_H_
