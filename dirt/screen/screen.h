#ifndef SCREEN_H
#define SCREEN_H

#include <windows.h>
#include <dirt/structures/hashmap.h>

using namespace Dirt::Structures;

namespace Dirt
{
  namespace Screen
  {
    struct DirectoryView
    {
      SMALL_RECT renderRect = {0};
      SHORT width = 0, height = 0;
      size_t nEntries = 0;
      size_t cursorIndex = 0;
      char path[MAX_PATH] = {0};
      WIN32_FIND_DATA *entries = 0;
      struct CursorMapEntry
      {
        size_t cursorIndex;
        char path[MAX_PATH] = {0};
      };
      Hashmap *cursorMap;
    };

    struct ScreenData
    {
      HANDLE backBuffer, frontBuffer;
      DirectoryView leftView, rightView, *active;
    };


    bool allocScreen(ScreenData &screen);
    void renderScreenDirectoryViews(ScreenData &screen);
    void renderDirectoryView(ScreenData &screen, DirectoryView &view);
    void styleView(HANDLE screenBuffer, DirectoryView view);
    void highlightLine(ScreenData &screen);
    void swapScreenBuffers(ScreenData &screen);
    bool setActiveView(ScreenData &screen, DirectoryView &view);
    void clearScreen(ScreenData &screen);
  }
}

#endif // SCREEN_H
