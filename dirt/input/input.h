#ifndef INPUT_H
#define INPUT_H

#include <windows.h>

namespace Dirt
{
  namespace Input
  {
    void handleInput(Screen &screen, HANDLE stdinHandle);
  }
}

#endif // INPUT_H
