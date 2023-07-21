#ifndef CONTEXT_H
#define CONTEXT_H

#include <dirt/structures/hashmap.h>
#include <dirt/input/input.h>
#include <dirt/screen/screen.h>

#define DIRT_SELECTIONBUF_MIN_DUPES 10
#define DIRT_SELECTIONBUF_MIN_SIZE 512

using namespace Dirt;

namespace Dirt
{
  struct Context
  {
    ScreenData *currentScreen = 0;
    bool quit = false;
    size_t maxEntriesInView = 128;
    Hashmap *selection;
    Input::InputData input;
  };
}

#endif // CONTEXT_H
