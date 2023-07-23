#ifndef CONTEXT_H
#define CONTEXT_H

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
    Screen::ScreenData *currentScreen = 0;
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
