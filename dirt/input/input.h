#ifndef INPUT_H
#define INPUT_H

#include <windows.h>
#include <dirt/screen/screen.h>

#define VK_Q 0x51
#define VK_H 0x48
#define VK_J 0x4A
#define VK_K 0x4B
#define VK_L 0x4C
#define VK_M 0x4D
#define VK_D 0x44

using namespace Dirt::Screen;

namespace Dirt
{
  namespace Input
  {
    struct InputData
    {
      WORD prevKeyCode = -1;
    };

    void handleInput(ScreenData &screen, HANDLE stdinHandle);
  }
}

#endif // INPUT_H
