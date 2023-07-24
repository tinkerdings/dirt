#ifndef INPUT_H
#define INPUT_H

#include <windows.h>
#include <dirt/screen/screen.h>
#include <dirt/context/context.h>

#define VK_Q 0x51
#define VK_H 0x48
#define VK_J 0x4A
#define VK_K 0x4B
#define VK_L 0x4C
#define VK_M 0x4D
#define VK_D 0x44
#define VK_E 0x45
#define VK_X 0x58

#define INPUTBUF_SIZE 3

using namespace Dirt::Screen;

namespace Dirt
{
  namespace Input
  {
    void handleInput(Dirt::Context *context, ScreenData &screen, HANDLE stdinHandle);
    bool inputNoKeyRepeat(Dirt::Context *context, INPUT_RECORD *inputBuffer, uint32_t index, uint32_t size);
  }
}

#endif // INPUT_H
