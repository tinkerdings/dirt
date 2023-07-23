#ifndef SCREEN_H
#define SCREEN_H

#include <windows.h>
#include <dirt/context/context.h>
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
      SHORT width = 0;
      SHORT height = 0;
      size_t nEntries = 0;
      char path[MAX_PATH] = {0};
      WIN32_FIND_DATA *entries = 0;
      struct CursorIndex
      {
        size_t visualIndex = 0;
        size_t actualIndex = 0;
        size_t scroll = 0;
      } cursorIndex;
      struct CursorMapEntry
      {
        CursorIndex cursorIndex;
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
    bool initScreenDirectoryViews(Context *context, ScreenData &screen);
    void setViewPath(Context *context, DirectoryView &view, char *relPath);
    void createFilenameCharInfoBuffer(CHAR_INFO *buffer, CHAR *filename, SHORT len, bool isDirectory);
    void renderScreenDirectoryViews(ScreenData &screen);
    void renderDirectoryView(ScreenData &screen, DirectoryView &view);
    void styleView(Context *context, HANDLE screenBuffer, DirectoryView view);
    void highlightLine(Context *context, ScreenData &screen);
    void swapScreenBuffers(ScreenData &screen);
    bool setActiveView(ScreenData &screen, DirectoryView &view);
    void clearScreen(ScreenData &screen);
    DirectoryView::CursorIndex getStoredViewCursorIndex(DirectoryView &view, size_t *hashIndexOut, size_t *dupeIndexOut);
    void styleScreenViews(Context *context, ScreenData &screen);
    void incrementScreenCursorIndex(ScreenData &screen);
    void decrementScreenCursorIndex(ScreenData &screen);
  }
}

#endif // SCREEN_H
