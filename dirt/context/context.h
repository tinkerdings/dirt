#ifndef CONTEXT_H
#define CONTEXT_H

#include <dirt/structures/splitBox.h>
#include <dirt/predefinedValues.h>
#include <dirt/structures/container.h>
#include <windows.h>
#include <stdio.h>

namespace Dirt
{
  // Forward declarations
  namespace Structures
  {
    struct Hashmap;
    struct SplitBox;
    struct Container;
  } // namespace Structures;
  namespace Screen
  {
    struct ScreenData;
  } // namespace Screen;

  struct Context
  {
    Screen::ScreenData **screens;
    Screen::ScreenData *currentScreen = 0;
    Structures::Container viewsContainer = {};
    HANDLE backBuffer = 0;
    HANDLE frontBuffer = 0;
    bool quit;
    size_t entryBufferNSlots;
    Structures::Hashmap *selection;
    Structures::SplitBox *viewBox;
    Structures::SplitBox tabFrames[9] = {};
    Structures::BoxGlyphs standardGlyphs = {};
    struct InputData
    {
      WORD prevKeyCode = -1;
    };
    InputData input;
  };

  Context *createContext();
} // namespace Dirt;

#endif // CONTEXT_H
