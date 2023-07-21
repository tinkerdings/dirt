#ifndef CONTEXT_H
#define CONTEXT_H

#include <windows.h>

#define DIRT_SELECTIONBUF_MIN_DUPES 10
#define DIRT_SELECTIONBUF_MIN_SIZE 512

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
    size_t maxEntriesInView;
    Structures::Hashmap *selection;
    struct InputData
    {
      WORD prevKeyCode = -1;
    };
    InputData input;
  };
} // namespace Dirt;

#endif // CONTEXT_H
