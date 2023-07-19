#ifndef CONTEXT_H
#define CONTEXT_H

#include <dirt/structures/hashmap.h>
#include <dirt/input/input.h>
#include <dirt/screen/screen.h>

using namespace Dirt::Input;
using namespace Dirt::Structures;
using namespace Dirt::Screen;

namespace Dirt
{
  namespace Context
  {
    struct Context
    {
      ScreenData *currentScreen = 0;
      bool quit = false;
      size_t maxEntriesInView = 128;
      Hashmap *selection;
      InputData input;
    };
  }
}

#endif // CONTEXT_H
