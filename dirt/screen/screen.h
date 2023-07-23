#ifndef SCREEN_H
#define SCREEN_H

#include <windows.h>
#include <dirt/context/context.h>
#include <dirt/structures/hashmap.h>


using namespace Dirt::Structures;

namespace Dirt
{
  namespace Screen
  {
    struct View
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
      View leftView, rightView, *active;
    };

    bool allocScreen(ScreenData &screen);
    bool initScreenViews(Context *context, ScreenData &screen);
    void setViewPath(Context *context, View &view, char *relPath);
    void createFilenameCharInfoBuffer(CHAR_INFO *buffer, CHAR *filename, SHORT len, bool isDirectory);
    void renderScreenViews(ScreenData &screen);
    void renderView(ScreenData &screen, View &view);
    void styleView(Context *context, HANDLE screenBuffer, View view);
    void highlightLine(Context *context, ScreenData &screen);
    void swapScreenBuffers(ScreenData &screen);
    bool setActiveView(ScreenData &screen, View &view);
    void clearScreen(ScreenData &screen);
    View::CursorIndex getStoredViewCursorIndex(View &view, size_t *hashIndexOut, size_t *dupeIndexOut);
    void styleScreenViews(Context *context, ScreenData &screen);
    void incrementScreenCursorIndex(ScreenData &screen);
    void decrementScreenCursorIndex(ScreenData &screen);
  }
}

#endif // SCREEN_H
