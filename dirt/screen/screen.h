#ifndef SCREEN_H
#define SCREEN_H

#include <windows.h>
#include <dirt/context/context.h>
#include <dirt/structures/hashmap.h>
#include <dirt/structures/container.h>

using namespace Dirt::Structures;

namespace Dirt
{
  namespace Screen
  {
    struct View
    {
      SMALL_RECT renderRect = {};
      SHORT width = 0;
      SHORT height = 0;
      size_t nEntries = 0;
      char path[MAX_PATH] = {};
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
        char path[MAX_PATH] = {};
      };
      Hashmap *cursorMap;
    };

    struct ScreenData
    {
      HANDLE backBuffer, frontBuffer;
      View leftView, rightView, *active;
    };

    bool initScreens(Context *context, int nScreens);
    void getConsoleDimensions(uint16_t &width, uint16_t &height);
    bool initScreenViews(Context *context, ScreenData &screen, Container container);
    void setCurrentScreen(Context *context, int unsigned number);
    void setViewPath(Context *context, View &view, char *relPath);
    void createFilenameCharInfoBuffer(CHAR_INFO *buffer, CHAR *filename, SHORT len, bool isDirectory);
    bool setActiveView(ScreenData &screen, View &view);
    View::CursorIndex getStoredViewCursorIndex(View &view, size_t *hashIndexOut, size_t *dupeIndexOut);
    void setViewEntries(Context *context, View &view, bool resizeBuffer);
    void incrementScreenCursorIndex(Context *context, ScreenData &screen);
    void decrementScreenCursorIndex(Context *context, ScreenData &screen);
  }
}

#endif // SCREEN_H
