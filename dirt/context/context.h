#ifndef CONTEXT_H
#define CONTEXT_H

#include <dirt/predefinedValues.h>
#include <dirt/structures/container.h>
#include <windows.h>

namespace Dirt
{
  // Forward declarations
  namespace Structures
  {
    struct Hashmap;
  } // namespace Structures;
  namespace Screen
  {
    struct ScreenData;
  } // namespace Screen;

  struct Context
  {
    Screen::ScreenData **screens;
    Screen::ScreenData *currentScreen = 0;
    Structures::Container viewsContainer = {0};
    HANDLE backBuffer = 0;
    HANDLE frontBuffer = 0;
    bool quit;
    size_t entryBufferNSlots;
    Structures::Hashmap *selection;
    struct InputData
    {
      WORD prevKeyCode = -1;
    };
    InputData input;
  };
} // namespace Dirt;

#endif // CONTEXT_H
