#include "dirt/error/errorCode.h"
#include <dirt/memory/memory.h>
#include <dirt/context/context.h>
#include <dirt/entry/entry.h>
#include <dirt/screen/screen.h>
#include <dirt/predefinedValues.h>
#include <dirt/rendering/rendering.h>
#include <processenv.h>
#include <stdio.h>

namespace Dirt
{
  namespace Screen
  {
    void setViewEntries(Context *context, View &view, bool resizeBuffer)
    {
      free(view.entries);
      view.entries = 0;
      if(resizeBuffer)
      {
        context->entryBufferNSlots = DIRT_ENTRYBUFFER_SIZE;
      }
      view.entries = Entry::findDirectoryEntries(context, view.path, view.nEntries);
    }

    bool initScreens(Context *context, int nScreens)
    {
      if(!(context->screens = (ScreenData **)calloc(nScreens, sizeof(ScreenData*))))
      {
        Error::errorCode = DIRT_ERROR_ALLOCATION_FAILURE;
        return false;
      }
      for(int i = 0; i < nScreens; i++)
      {
        if(!(context->screens[i] = (ScreenData *)calloc(1, sizeof(ScreenData))))
        {
          for(int j = i-1; j >= 0; j--)
          {
            free(context->screens[j]);
            context->screens[j] = 0;
          }
          free(context->screens);
          context->screens = 0;
          Error::errorCode = DIRT_ERROR_ALLOCATION_FAILURE;
          return false;
        }

        if(!initScreenViews(context, *context->screens[i], context->viewsContainer))
        {
          for(int j = i; j >= 0; j--)
          {
            free(context->screens[j]);
            context->screens[j] = 0;
          }
          free(context->screens);
          context->screens = 0;
          Error::errorCode = DIRT_ERROR_ALLOCATION_FAILURE;
          return false;
        }
      }

      context->backBuffer = CreateConsoleScreenBuffer(
          GENERIC_READ | GENERIC_WRITE,
          FILE_SHARE_READ | FILE_SHARE_WRITE,
          NULL,
          CONSOLE_TEXTMODE_BUFFER,
          NULL);
      context->frontBuffer = CreateConsoleScreenBuffer(
          GENERIC_READ | GENERIC_WRITE,
          FILE_SHARE_READ | FILE_SHARE_WRITE,
          NULL,
          CONSOLE_TEXTMODE_BUFFER,
          NULL);

      if((context->backBuffer == INVALID_HANDLE_VALUE) ||
          (context->frontBuffer == INVALID_HANDLE_VALUE))
      {
        free(context->backBuffer);
        free(context->frontBuffer);
        free(context->screens);
        context->backBuffer = 0;
        context->frontBuffer = 0;
        context->screens = 0;

        Error::errorCode = DIRT_ERROR_ALLOCATION_FAILURE;

        return false;
      }

      for(int i = 0; i < nScreens; i++)
      {
        context->screens[i]->backBuffer = context->backBuffer;
        context->screens[i]->frontBuffer = context->frontBuffer;
      }

      return true;
    }

    bool setActiveView(ScreenData &screen, View &view)
    {
      screen.active = &view;
      if(!SetCurrentDirectory(screen.active->path))
      {
        printf("Failed to set current directory (%lu)\n", GetLastError());
        return false;
      }
      return true;
    }

    void setCurrentScreen(Context *context, int unsigned number)
    {
      context->currentScreen = context->screens[min(number, DIRT_N_SCREENS-1)];
      setViewPath(context, context->currentScreen->leftView, context->currentScreen->leftView.path);
      setViewPath(context, context->currentScreen->rightView, context->currentScreen->rightView.path);
    }

    bool initScreenViews(Context *context, ScreenData &screen, Container container)
    {
      char currentDir[MAX_PATH] = {};
      if(!GetCurrentDirectoryA(MAX_PATH, currentDir))
      {
        printf("initScreenViews:GetCurrentDirectory failed (%lu)\n", GetLastError());
        return false;
      }

      screen.leftView.cursorMap = 
        hashmapCreate(
          DIRT_CURSORINDICES_MIN_SIZE,
          DIRT_CURSORINDICES_MIN_DUPES,
          sizeof(View::CursorMapEntry));

      strcpy(screen.leftView.path, currentDir);

      Rendering::sizeScreenViews(screen, container);

      screen.leftView.entries = Entry::findDirectoryEntries(context, screen.leftView.path, screen.leftView.nEntries);
      if(!screen.leftView.entries)
      {
        printf("findDirectoryEntries failed (%lu)\n", GetLastError());
        return false;
      }

      screen.rightView.cursorMap = 
        hashmapCreate(
            DIRT_CURSORINDICES_MIN_SIZE,
            DIRT_CURSORINDICES_MIN_DUPES,
            sizeof(View::CursorMapEntry));

      strcpy(screen.rightView.path, "C:\\");

      Rendering::sizeScreenViews(screen, container);

      screen.rightView.entries = Entry::findDirectoryEntries(context, screen.rightView.path, screen.rightView.nEntries);
      if(!screen.rightView.entries)
      {
        printf("findDirectoryEntries failed (%lu)\n", GetLastError());
        return false;
      }

      setActiveView(screen, screen.leftView);

      return true;
    }

    void getConsoleDimensions(uint16_t &width, uint16_t &height)
    {
      CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
      GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &bufferInfo);
      width = (bufferInfo.srWindow.Right - bufferInfo.srWindow.Left) + 1;
      height = (bufferInfo.srWindow.Bottom - bufferInfo.srWindow.Top) + 1;
    }

    void setViewPath(Context *context, View &view, char *relPath)
    {
      if(!SetCurrentDirectory(view.path))
      {
        printf("Failed to set current directory (%lu)\n", GetLastError());
      }

      char fullPath[MAX_PATH] = {};
      Entry::getFullPath(fullPath, relPath, MAX_PATH);

      if(!SetCurrentDirectory(fullPath))
      {
        printf("Failed to set current directory (%lu)\n", GetLastError());
      }

      if(view.path[0])
      {
        View::CursorIndex cursorIndex = view.cursorIndex;
        View::CursorMapEntry cursorIndexEntry;
        size_t viewPathLen = strlen(view.path);
        cursorIndexEntry.cursorIndex = cursorIndex;
        memcpy(cursorIndexEntry.path, view.path, viewPathLen);

        size_t hashIndex, dupeIndex;
        View::CursorIndex testIndex = getStoredViewCursorIndex(view, &hashIndex, &dupeIndex);
        if(testIndex.actualIndex + testIndex.visualIndex)
        {
          hashmapDirectWrite(
              view.cursorMap,
              &cursorIndexEntry,
              hashIndex,
              dupeIndex,
              sizeof(View::CursorMapEntry));
        }
        else 
        {
          Dirt::Structures::hashmapInsertAtKey(
            view.cursorMap,
            view.path,
            &cursorIndexEntry,
            viewPathLen,
            sizeof(View::CursorMapEntry));
        }
      }

      strcpy(view.path, fullPath);

      setViewEntries(context, view, true);

      view.cursorIndex = getStoredViewCursorIndex(view, 0, 0);
    }

    void createFilenameCharInfoBuffer(CHAR_INFO *buffer, CHAR *filename, SHORT len, bool isDirectory)
    {
      size_t filenameLength = strlen(filename);
      int i = 0;
      for(; i < filenameLength; i++)
      {
        CHAR_INFO ci = {(WCHAR)filename[i], FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
        buffer[i] = ci;
      }
      if(isDirectory)
      {
        CHAR_INFO ci = {'/', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
        buffer[i] = ci;
      }
    }

    View::CursorIndex getStoredViewCursorIndex(View &view, size_t *hashIndexOut, size_t *dupeIndexOut)
    {
      size_t viewPathLen = strlen(view.path);
      size_t hashIndex = hashmapGetIndex(view.cursorMap, view.path, viewPathLen);

      if(view.cursorMap->map[hashIndex][0].isSet)
      {
        for(size_t i = 0; i < view.cursorMap->nDupes; i++)
        {
          View::CursorMapEntry *entry = (View::CursorMapEntry *)view.cursorMap->map[hashIndex][i].data;
          bool ret;
          size_t entryPathLen = strlen(entry->path);
          size_t viewPathLen = strlen(view.path);
          if((ret = Memory::compareBytes(entry->path, view.path, entryPathLen, viewPathLen)))
          {
            if(hashIndexOut)
            {
              *hashIndexOut = hashIndex;
            }
            if(dupeIndexOut)
            {
              *dupeIndexOut = i;
            }
            return entry->cursorIndex;
          }
        }
      }

      View::CursorIndex zeroIndex = {0, 0, 0};
      return zeroIndex;
    }

    void incrementScreenCursorIndex(Context *context, ScreenData &screen)
    {
      View::CursorIndex currentIndex = screen.active->cursorIndex;
      if(currentIndex.actualIndex < (screen.active->nEntries-1))
      {
        screen.active->cursorIndex.actualIndex++;
        if(currentIndex.visualIndex == screen.active->height-1)
        {
          screen.active->cursorIndex.scroll++;
          Rendering::renderScreenViews(screen, context->viewsContainer);
        }
      }
      size_t minHeight = min(screen.active->height-1, screen.active->nEntries-1);
      if(currentIndex.visualIndex < minHeight)
      {
        screen.active->cursorIndex.visualIndex++;
      }
    }

    void decrementScreenCursorIndex(Context *context, ScreenData &screen)
    {
      View::CursorIndex currentIndex = screen.active->cursorIndex;
      if(currentIndex.actualIndex > 0)
      {
        screen.active->cursorIndex.actualIndex--;
        if(currentIndex.visualIndex == 0)
        {
          screen.active->cursorIndex.scroll--;
          Rendering::renderScreenViews(screen, context->viewsContainer);
        }
      }
      if(currentIndex.visualIndex > 0)
      {
        screen.active->cursorIndex.visualIndex--;
      }
    }
  } // namespace Screen
} // namespace Dirt
