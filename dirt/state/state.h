#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <dirt/context/context.h>

namespace Dirt
{
  namespace State
  {
    void stateMain(Context *context);
    void stateVolume(Context *context);
    void statePrompt(Context *context);

  } // namespace State
} // namespace Dirt

#endif // STATE_H
