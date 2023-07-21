#ifndef SCREEN_H
#define SCREEN_H

#include <windows.h>
#include <dirt/structures/hashmap.h>

#define DIRT_CURSORINDICES_MIN_DUPES 10
#define DIRT_CURSORINDICES_MIN_SIZE 32

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
    bool initScreenDirectoryViews(ScreenData &screen);
    void setViewPath(DirectoryView &view, char *relPath);
    void createFilenameCharInfoBuffer(CHAR_INFO *buffer, CHAR *filename, SHORT len, bool isDirectory);
    void renderScreenDirectoryViews(ScreenData &screen);
    void renderDirectoryView(ScreenData &screen, DirectoryView &view);
    void styleView(HANDLE screenBuffer, DirectoryView view);
    void highlightLine(ScreenData &screen);
    void swapScreenBuffers(ScreenData &screen);
    bool setActiveView(ScreenData &screen, DirectoryView &view);
    void clearScreen(ScreenData &screen);
    size_t getViewCursorIndex(DirectoryView &view, size_t *hashIndexOut, size_t *dupeIndexOut);
    void styleScreenViews(ScreenData &screen);
    void incrementScreenCursorIndex(ScreenData &screen);
    void decrementScreenCursorIndex(ScreenData &screen);
  }
}

#endif // SCREEN_H
