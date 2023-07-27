#include "dirt/error/errorCode.h"
#include <dirt/memory/memory.h>
#include <dirt/context/context.h>
#include <dirt/entry/entry.h>
#include <dirt/screen/screen.h>
#include <dirt/predefinedValues.h>
#include <stdio.h>

namespace Dirt
{
  namespace Screen
  {
    void clearScreen(ScreenData &screen)
    {
      CHAR_INFO leftClear[MAX_PATH] = {0};
      for(int i = 0; i < screen.leftView.width; i++)
      {
        leftClear[i] = {' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE};
      }
      for(int i = 0; i < screen.leftView.height; i++)
      {
        COORD size {screen.leftView.width, 1};
        COORD pos = {0, 0};
        SMALL_RECT rect;
        rect.Top = screen.leftView.renderRect.Top+i;
        rect.Left = screen.leftView.renderRect.Left;
        rect.Bottom = screen.leftView.renderRect.Bottom;
        rect.Right = screen.leftView.renderRect.Right;
        if(!WriteConsoleOutput(screen.backBuffer, leftClear, size, pos, &rect))
        {
          printf("%s%d WriteConsoleOutput failed (%lu)\n", __FILE__, __LINE__, GetLastError());
          return;
        }
      }
      CHAR_INFO rightClear[MAX_PATH] = {0};
      for(int i = 0; i < screen.rightView.width; i++)
      {
        rightClear[i] = {' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE};
      }
      for(int i = 0; i < screen.rightView.height; i++)
      {
        COORD size {screen.rightView.width, 1};
        COORD pos = {0, 0};
        SMALL_RECT rect;
        rect.Top = screen.rightView.renderRect.Top+i;
        rect.Left = screen.rightView.renderRect.Left;
        rect.Bottom = screen.rightView.renderRect.Bottom;
        rect.Right = screen.rightView.renderRect.Right;
        if(!WriteConsoleOutput(screen.backBuffer, rightClear, size, pos, &rect))
        {
          printf("%s%d WriteConsoleOutput failed (%lu)\n", __FILE__, __LINE__, GetLastError());
          return;
        }
      }
    }

    void refresh(Context *context, ScreenData &screen)
    {
      clearScreen(screen);
      setViewEntries(context, screen.leftView, false);
      setViewEntries(context, screen.rightView, false);
    }

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

    void renderContainerBorder(ScreenData &screen, Container container)
    {
      SetConsoleOutputCP(65001);

      WCHAR horizontalChar[255];
      WCHAR verticalChar[255];
      WCHAR topSplitChar[255];
      WCHAR bottomSplitChar[255];
      WCHAR topLeftChar[255];
      WCHAR topRightChar[255];
      WCHAR bottomLeftChar[255];
      WCHAR bottomRightChar[255];

      MultiByteToWideChar(CP_UTF8, MB_COMPOSITE | MB_USEGLYPHCHARS, "═", -1, horizontalChar, 255);
      MultiByteToWideChar(CP_UTF8, MB_COMPOSITE | MB_USEGLYPHCHARS, "║", -1, verticalChar, 255);
      MultiByteToWideChar(CP_UTF8, MB_COMPOSITE | MB_USEGLYPHCHARS, "╦", -1, topSplitChar, 255);
      MultiByteToWideChar(CP_UTF8, MB_COMPOSITE | MB_USEGLYPHCHARS, "╩", -1, bottomSplitChar, 255);
      MultiByteToWideChar(CP_UTF8, MB_COMPOSITE | MB_USEGLYPHCHARS, "╔", -1, topLeftChar, 255);
      MultiByteToWideChar(CP_UTF8, MB_COMPOSITE | MB_USEGLYPHCHARS, "╗", -1, topRightChar, 255);
      MultiByteToWideChar(CP_UTF8, MB_COMPOSITE | MB_USEGLYPHCHARS, "╚", -1, bottomLeftChar, 255);
      MultiByteToWideChar(CP_UTF8, MB_COMPOSITE | MB_USEGLYPHCHARS, "╝", -1, bottomRightChar, 255);

      COORD topStart = {(SHORT)(container.pos[0] + 1), (SHORT)container.pos[1]};
      COORD topEnd = {(SHORT)(container.pos[0] + container.width), (SHORT)container.pos[1]};
      renderHorizontalLineWithCharacter(screen, topStart, topEnd, horizontalChar);
      COORD bottomStart = {(SHORT)(container.pos[0] + 1), (SHORT)(container.pos[1]+container.height)};
      COORD bottomEnd = {(SHORT)(container.pos[0] + container.width), (SHORT)(container.pos[1] + container.height)};
      renderHorizontalLineWithCharacter(screen, bottomStart, bottomEnd, horizontalChar);

      COORD leftStart = {(SHORT)(container.pos[0]), (SHORT)(container.pos[1])};
      COORD leftEnd = {(SHORT)(container.pos[0]), (SHORT)(container.pos[1] + container.height+1)};
      renderVerticalLineWithCharacter(screen, leftStart, leftEnd, verticalChar);
      COORD rightStart = {(SHORT)(container.pos[0] + container.width), (SHORT)(container.pos[1])};
      COORD rightEnd = {(SHORT)(container.pos[0] + container.width), (SHORT)(container.pos[1] + container.height+1)};
      renderVerticalLineWithCharacter(screen, rightStart, rightEnd, verticalChar);

      COORD split1Start = {(SHORT)(container.pos[0] + container.width/2), (SHORT)(container.pos[1] + 1)};
      COORD split1End = {(SHORT)(container.pos[0] + container.width/2), (SHORT)(container.pos[1] + container.height)};
      renderVerticalLineWithCharacter(screen, split1Start, split1End, verticalChar);
      COORD split2Start = {(SHORT)(container.pos[0] + container.width/2-1), (SHORT)(container.pos[1] + 1)};
      COORD split2End = {(SHORT)(container.pos[0] + container.width/2-1), (SHORT)(container.pos[1] + container.height)};
      renderVerticalLineWithCharacter(screen, split2Start, split2End, verticalChar);

      COORD topSplitCoord1 = {(SHORT)(container.pos[0] + (container.width/2)-1), (SHORT)(container.pos[1])};
      renderVerticalLineWithCharacter(screen, topSplitCoord1, topSplitCoord1, topSplitChar);

      COORD topSplitCoord2 = {(SHORT)(container.pos[0] + (container.width/2)), (SHORT)(container.pos[1])};
      renderVerticalLineWithCharacter(screen, topSplitCoord2, topSplitCoord2, topSplitChar);

      COORD bottomSplitCoord1 = {(SHORT)(container.pos[0] + container.width/2), (SHORT)(container.pos[1] + container.height)};
      renderVerticalLineWithCharacter(screen, bottomSplitCoord1, bottomSplitCoord1, bottomSplitChar);

      COORD bottomSplitCoord2 = {(SHORT)(container.pos[0] + (container.width/2) - 1), (SHORT)(container.pos[1] + container.height)};
      renderVerticalLineWithCharacter(screen, bottomSplitCoord2, bottomSplitCoord2, bottomSplitChar);

      COORD topLeftCoord = {(SHORT)(container.pos[0]), (SHORT)(container.pos[1])};
      renderVerticalLineWithCharacter(screen, topLeftCoord, topLeftCoord, topLeftChar);

      COORD topRightCoord = {(SHORT)(container.pos[0] + container.width), (SHORT)(container.pos[1])};
      renderVerticalLineWithCharacter(screen, topRightCoord, topRightCoord, topRightChar);

      COORD bottomLeftCoord = {(SHORT)(container.pos[0]), (SHORT)(container.pos[1]+container.height)};
      renderVerticalLineWithCharacter(screen, bottomLeftCoord, bottomLeftCoord, bottomLeftChar);

      COORD bottomRightCoord = {(SHORT)(container.pos[0] + container.width), (SHORT)(container.pos[1]+container.height)};
      renderVerticalLineWithCharacter(screen, bottomRightCoord, bottomRightCoord, bottomRightChar);
    }

    void renderScreenViews(ScreenData &screen, Container container)
    {
      clearScreen(screen);
      renderView(screen, screen.leftView);
      renderView(screen, screen.rightView);
      renderContainerBorder(screen, container);
    }

    void renderVerticalLineWithCharacter(ScreenData &screen, COORD startPos, COORD endPos, WCHAR *character)
    {
      CHAR_INFO buffer[2048] = {0};

      int len = endPos.Y - startPos.Y;
      if(len < 1)
      {
        len = 1;
      }
      for(int i = 0; i < min(len, 2048); i++)
      {
        CHAR_INFO ci = {*character, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
        buffer[i] = ci;
      }

      COORD dimensions = {1, (SHORT)(len)};
      COORD bufferStartPos = {0, 0};
      SMALL_RECT rect = {startPos.X, startPos.Y, endPos.X, endPos.Y};

      if(!WriteConsoleOutputW(screen.backBuffer, buffer, dimensions, bufferStartPos, &rect))
      {
        printf("%s%dWriteConsoleOutput failed (%lu)\n", __FILE__, __LINE__, GetLastError());
        return;
      }
    }

    void renderHorizontalLineWithCharacter(ScreenData &screen, COORD startPos, COORD endPos, WCHAR *character)
    {
      CHAR_INFO buffer[2048] = {0};

      int len = endPos.X - startPos.X;
      if(len < 1)
      {
        len = 1;
      }
      for(int i = 0; i < min(len, 2048); i++)
      {
        CHAR_INFO ci = {*character, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
        buffer[i] = ci;
      }

      COORD dimensions = {(SHORT)(len), 1};
      COORD bufferStartPos = {0, 0};
      SMALL_RECT rect = {startPos.X, startPos.Y, endPos.X, endPos.Y};

      if(!WriteConsoleOutputW(screen.backBuffer, buffer, dimensions, bufferStartPos, &rect))
      {
        printf("%s%dWriteConsoleOutput failed (%lu)\n", __FILE__, __LINE__, GetLastError());
        return;
      }
    }

    void renderView(ScreenData &screen, View &view)
    {
      size_t minHeight = min(view.nEntries, view.height+view.cursorIndex.scroll);
      for(size_t i = view.cursorIndex.scroll; i < minHeight; i++)
      {
        CHAR_INFO filename[MAX_PATH] = {0};
        bool isDirectory = (view.entries[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        createFilenameCharInfoBuffer(filename, view.entries[i].cFileName, view.width, isDirectory);
        COORD filenameDimensions = {view.width, 1};
        COORD filenamePos = {0, 0};
        SMALL_RECT rect;
        rect.Top = view.renderRect.Top+(SHORT)i-(SHORT)view.cursorIndex.scroll;
        rect.Left = view.renderRect.Left;
        rect.Bottom = view.renderRect.Bottom;
        rect.Right = view.renderRect.Right;
        if(!WriteConsoleOutput(screen.backBuffer, filename, filenameDimensions, filenamePos, &rect))
        {
          printf("%s%dWriteConsoleOutput failed (%lu)\n", __FILE__, __LINE__, GetLastError());
          return;
        }
      }
    }

    void styleView(Context *context, ScreenData &screen, View view)
    {
      size_t minHeight = min(view.nEntries, view.height+view.cursorIndex.scroll);
      for(int i = view.cursorIndex.scroll; i < minHeight; i++)
      {
        WIN32_FIND_DATA entry = view.entries[i];

        WORD attributes = entry.dwFileAttributes;
        
        COORD coords;
        coords.X = view.renderRect.Left;
        coords.Y = view.renderRect.Top + i-view.cursorIndex.scroll;
        size_t filenameLength = strlen(view.entries[i].cFileName);
        DWORD nSet = 0;

        if(attributes & FILE_ATTRIBUTE_DIRECTORY)
        {
        FillConsoleOutputAttribute(
            screen.backBuffer,
            FOREGROUND_RED | FOREGROUND_GREEN,
            filenameLength+1,
            coords,
            &nSet);
        }

        char fullPath[MAX_PATH] = {0};
        Entry::getFullPath(fullPath, entry.cFileName, MAX_PATH);

        if(hashmapContains(context->selection, fullPath, context->selection->dataSize, 0, 0))
        {
          FillConsoleOutputAttribute(
              screen.backBuffer,
              FOREGROUND_GREEN,
              filenameLength,
              coords,
              &nSet);
        }
      }
    }

    void highlightLine(Context *context, ScreenData &screen)
    {
      size_t cursorFilenameLength = min(
          strlen(screen.active->entries[screen.active->cursorIndex.actualIndex].cFileName),
          screen.active->width);
      size_t emptySpace = 
        screen.active->width - cursorFilenameLength;
      COORD coords;
      DWORD nSet = 0;
      coords.X = screen.active->renderRect.Left;
      coords.Y = screen.active->renderRect.Top + screen.active->cursorIndex.visualIndex;
      WORD attribs = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;

      char fullPath[MAX_PATH] = {0};
      Entry::getFullPath(
          fullPath,
          screen.active->entries[screen.active->cursorIndex.actualIndex].cFileName,
          MAX_PATH);

      if(hashmapContains(context->selection, fullPath, MAX_PATH, 0, 0))
      {
        attribs ^= FOREGROUND_RED | FOREGROUND_BLUE;
      }

      FillConsoleOutputAttribute(
        screen.backBuffer,
        attribs,
        screen.active->width,
        coords,
        &nSet);

      if(emptySpace > 0)
      {
        coords.X += cursorFilenameLength;
        FillConsoleOutputCharacter(screen.backBuffer,
          ' ',
          emptySpace,
          coords,
          &nSet);
      }
    }

    void styleScreenViews(Context *context, ScreenData &screen)
    {
      styleView(context, screen, screen.leftView);
      styleView(context, screen, screen.rightView);
      highlightLine(context, *(context->currentScreen));
    }

    void swapScreenBuffers(ScreenData &screen)
    {
      HANDLE temp = screen.frontBuffer;
      screen.frontBuffer = screen.backBuffer;
      screen.backBuffer = temp;
      if(!SetConsoleActiveScreenBuffer(screen.frontBuffer))
      {
        printf("SetConsoleActiveScreenBuffer failed (%lu)\n", GetLastError());
        return;
      }
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

    void renderTabsContainer(Context *context, Container *container)
    {

    }

    void sizeScreenViews(ScreenData &screen, Container container)
    {
      screen.leftView.renderRect.Top = container.pos[1] + 1;
      screen.leftView.renderRect.Left = container.pos[0] + 1;
      screen.leftView.renderRect.Bottom = screen.leftView.renderRect.Top + container.height - 2;
      screen.leftView.renderRect.Right = screen.leftView.renderRect.Left + (container.width/2) - 2;
      screen.leftView.width = screen.leftView.renderRect.Right - screen.leftView.renderRect.Left;
      screen.leftView.height = screen.leftView.renderRect.Bottom - screen.leftView.renderRect.Top;

      screen.rightView.renderRect.Top = container.pos[1] + 1;
      screen.rightView.renderRect.Left = screen.leftView.renderRect.Right + 2;
      screen.rightView.renderRect.Bottom = screen.rightView.renderRect.Top + container.height - 2;
      screen.rightView.renderRect.Right = screen.rightView.renderRect.Left + (container.width/2) - 2;
      screen.rightView.width = screen.rightView.renderRect.Right - screen.rightView.renderRect.Left;
      screen.rightView.height = screen.rightView.renderRect.Bottom - screen.rightView.renderRect.Top;
    }

    bool initScreenViews(Context *context, ScreenData &screen, Container container)
    {
      char currentDir[MAX_PATH] = {0};
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

      sizeScreenViews(screen, container);

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

      sizeScreenViews(screen, container);

      screen.rightView.entries = Entry::findDirectoryEntries(context, screen.rightView.path, screen.rightView.nEntries);
      if(!screen.rightView.entries)
      {
        printf("findDirectoryEntries failed (%lu)\n", GetLastError());
        return false;
      }

      setActiveView(screen, screen.leftView);

      return true;
    }

    void setViewPath(Context *context, View &view, char *relPath)
    {
      if(!SetCurrentDirectory(view.path))
      {
        printf("Failed to set current directory (%lu)\n", GetLastError());
      }

      char fullPath[MAX_PATH] = {0};
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
          renderScreenViews(screen, context->viewsContainer);
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
          renderScreenViews(screen, context->viewsContainer);
        }
      }
      if(currentIndex.visualIndex > 0)
      {
        screen.active->cursorIndex.visualIndex--;
      }
    }
  } // namespace Screen
} // namespace Dirt
