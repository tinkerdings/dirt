#ifndef INPUT_H
#define INPUT_H

#include <windows.h>
#include <dirt/screen/screen.h>
#include <dirt/context/context.h>

#define VK_Q 0x51 // Default quit
#define VK_H 0x48 // Default left
#define VK_J 0x4A // Default down
#define VK_K 0x4B // Default up
#define VK_L 0x4C // Default right
#define VK_M 0x4D // Default move
#define VK_D 0x44 // Default delete
#define VK_E 0x45 // Default explorer
#define VK_X 0x58 // Default cmd
#define VK_R 0x52 // Default refresh

#define VK_0 0x30
#define VK_1 0x31
#define VK_2 0x32
#define VK_3 0x33
#define VK_4 0x34
#define VK_5 0x35
#define VK_6 0x36
#define VK_7 0x37
#define VK_8 0x38
#define VK_9 0x39

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
