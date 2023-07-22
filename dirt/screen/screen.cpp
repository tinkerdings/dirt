#include <dirt/memory/memory.h>
#include <dirt/context/context.h>
#include <dirt/entry/entry.h>
#include <dirt/screen/screen.h>
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
        if(!WriteConsoleOutput(screen.frontBuffer, leftClear, size, pos, &rect))
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
        if(!WriteConsoleOutput(screen.frontBuffer, rightClear, size, pos, &rect))
        {
          printf("%s%d WriteConsoleOutput failed (%lu)\n", __FILE__, __LINE__, GetLastError());
          return;
        }
      }
    }


    bool allocScreen(ScreenData &screen)
    {
      screen.backBuffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL);
      screen.frontBuffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL);

      if((screen.backBuffer == INVALID_HANDLE_VALUE) || (screen.frontBuffer == INVALID_HANDLE_VALUE))
      {
        return false;
      }

      return true;
    }


    void renderScreenDirectoryViews(ScreenData &screen)
    {
      renderDirectoryView(screen, screen.leftView);
      renderDirectoryView(screen, screen.rightView);
    }


    void renderDirectoryView(ScreenData &screen, DirectoryView &view)
    {
      size_t minHeight = min(view.nEntries, view.height+view.cursorIndex.scroll);
      for(size_t i = view.cursorIndex.scroll; i < minHeight; i++)
      {
        CHAR_INFO filename[MAX_PATH] = {0};
        bool isDirectory = (view.entries[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        createFilenameCharInfoBuffer(filename, view.entries[i].cFileName, view.width, isDirectory);
        COORD filenameSize {view.width, 1};
        COORD filenamePos = {0, 0};
        SMALL_RECT rect;
        rect.Top = view.renderRect.Top+(SHORT)i-(SHORT)view.cursorIndex.scroll;
        rect.Left = view.renderRect.Left;
        rect.Bottom = view.renderRect.Bottom;
        rect.Right = view.renderRect.Right;
        if(!WriteConsoleOutput(screen.backBuffer, filename, filenameSize, filenamePos, &rect))
        {
          printf("%s%dWriteConsoleOutput failed (%lu)\n", __FILE__, __LINE__, GetLastError());
          return;
        }
      }
    }

    void styleView(Context *context, HANDLE screenBuffer, DirectoryView view)
    {
      size_t minHeight = min(view.nEntries, view.height+view.cursorIndex.scroll);
      for(int i = view.cursorIndex.scroll; i < minHeight; i++)
      {
        WIN32_FIND_DATA entry = view.entries[i];

        WORD attributes = entry.dwFileAttributes;
        
        COORD coords;
        coords.X = view.renderRect.Left;
        coords.Y = i-view.cursorIndex.scroll;
        size_t filenameLength = strlen(view.entries[i].cFileName);
        DWORD nSet = 0;

        if(attributes & FILE_ATTRIBUTE_DIRECTORY)
        {
        FillConsoleOutputAttribute(
            screenBuffer,
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
              screenBuffer,
              FOREGROUND_GREEN,
              filenameLength,
              coords,
              &nSet);
        }
      }
    }

    void highlightLine(Context *context, ScreenData &screen)
    {
      size_t cursorFilenameLength = strlen(
          screen.active->entries[screen.active->cursorIndex.actualIndex].cFileName);
      size_t emptySpace = 
        screen.active->width - cursorFilenameLength;
      COORD coords;
      DWORD nSet = 0;
      coords.X = screen.active->renderRect.Left;
      coords.Y = screen.active->cursorIndex.visualIndex;
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

      coords.X += cursorFilenameLength;
      FillConsoleOutputCharacter(screen.backBuffer,
        ' ',
        emptySpace,
        coords,
        &nSet);
    }

    void styleScreenViews(Context *context, ScreenData &screen)
    {
      styleView(context, screen.backBuffer, screen.leftView);
      styleView(context, screen.backBuffer, screen.rightView);
      highlightLine(context, screen);
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

    bool setActiveView(ScreenData &screen, DirectoryView &view)
    {
      screen.active = &view;
      if(!SetCurrentDirectory(screen.active->path))
      {
        printf("Failed to set current directory (%lu)\n", GetLastError());
        return false;
      }
      return true;
    }

    bool initScreenDirectoryViews(Context *context, ScreenData &screen)
    {
      char currentDir[MAX_PATH] = {0};
      if(!GetCurrentDirectoryA(MAX_PATH, currentDir))
      {
        printf("initScreenDirectoryViews:GetCurrentDirectory failed (%lu)\n", GetLastError());
        return false;
      }
      screen.leftView.cursorMap = 
        hashmapCreate(
          DIRT_CURSORINDICES_MIN_SIZE,
          DIRT_CURSORINDICES_MIN_DUPES,
          sizeof(DirectoryView::CursorMapEntry));
      strcpy(screen.leftView.path, currentDir);
      screen.leftView.renderRect.Top = 0;
      screen.leftView.renderRect.Left = 0;
      screen.leftView.renderRect.Bottom = 30;
      screen.leftView.renderRect.Right = 40;
      screen.leftView.width = screen.leftView.renderRect.Right - screen.leftView.renderRect.Left;
      screen.leftView.height = screen.leftView.renderRect.Bottom - screen.leftView.renderRect.Top;
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
            sizeof(DirectoryView::CursorMapEntry));
      strcpy(screen.rightView.path, "C:\\");
      screen.rightView.renderRect.Top = 0;
      screen.rightView.renderRect.Left = 42;
      screen.rightView.renderRect.Bottom = 30;
      screen.rightView.renderRect.Right = 82;
      screen.rightView.width = screen.rightView.renderRect.Right - screen.rightView.renderRect.Left;
      screen.rightView.height = screen.rightView.renderRect.Bottom - screen.rightView.renderRect.Top;
      screen.rightView.entries = Entry::findDirectoryEntries(context, screen.rightView.path, screen.rightView.nEntries);
      if(!screen.rightView.entries)
      {
        printf("findDirectoryEntries failed (%lu)\n", GetLastError());
        return false;
      }

      setActiveView(screen, screen.leftView);

      return true;
    }

    void setViewPath(Context *context, DirectoryView &view, char *relPath)
    {
      char fullPath[MAX_PATH] = {0};
      Entry::getFullPath(fullPath, relPath, MAX_PATH);

      if(!SetCurrentDirectory(fullPath))
      {
        printf("Failed to set current directory (%lu)\n", GetLastError());
      }

      if(view.path[0])
      {
        DirectoryView::CursorIndex cursorIndex = view.cursorIndex;
        DirectoryView::CursorMapEntry cursorIndexEntry;
        size_t viewPathLen = strlen(view.path);
        cursorIndexEntry.cursorIndex = cursorIndex;
        memcpy(cursorIndexEntry.path, view.path, viewPathLen);

        size_t hashIndex, dupeIndex;
        DirectoryView::CursorIndex testIndex = getViewCursorIndex(view, &hashIndex, &dupeIndex);
        if(!(testIndex.actualIndex + testIndex.visualIndex))
        {
          hashmapDirectWrite(
              view.cursorMap,
              &cursorIndexEntry,
              hashIndex,
              dupeIndex,
              sizeof(DirectoryView::CursorMapEntry));
        }
        else 
        {
          Dirt::Structures::hashmapInsertAtKey(
            view.cursorMap,
            view.path,
            &cursorIndexEntry,
            viewPathLen,
            sizeof(DirectoryView::CursorMapEntry));
        }
      }

      strcpy(view.path, fullPath);

      free(view.entries);
      view.entries = 0;
      context->maxEntriesInView = 128;
      view.entries = Entry::findDirectoryEntries(context, view.path, view.nEntries);

      view.cursorIndex = getViewCursorIndex(view, 0, 0);
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

    DirectoryView::CursorIndex getViewCursorIndex(DirectoryView &view, size_t *hashIndexOut, size_t *dupeIndexOut)
    {
      size_t viewPathLen = strlen(view.path);
      size_t hashIndex = hashmapGetIndex(view.cursorMap, view.path, viewPathLen);

      if(view.cursorMap->map[hashIndex][0].isSet)
      {
        for(size_t i = 0; i < view.cursorMap->nDupes; i++)
        {
          DirectoryView::CursorMapEntry *entry = (DirectoryView::CursorMapEntry *)view.cursorMap->map[hashIndex][i].data;
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

      DirectoryView::CursorIndex zeroIndex = {0, 0, 0};
      return zeroIndex;
    }

    void incrementScreenCursorIndex(ScreenData &screen)
    {
      DirectoryView::CursorIndex currentIndex = screen.active->cursorIndex;
      if(currentIndex.actualIndex < (screen.active->nEntries-1))
      {
        screen.active->cursorIndex.actualIndex++;
        if(currentIndex.visualIndex == screen.active->height-1)
        {
          screen.active->cursorIndex.scroll++;
          clearScreen(screen);
        }
      }
      size_t minHeight = min(screen.active->height-1, screen.active->nEntries-1);
      if(currentIndex.visualIndex < minHeight)
      {
        screen.active->cursorIndex.visualIndex++;
      }
    }

    void decrementScreenCursorIndex(ScreenData &screen)
    {
      DirectoryView::CursorIndex currentIndex = screen.active->cursorIndex;
      if(currentIndex.actualIndex > 0)
      {
        screen.active->cursorIndex.actualIndex--;
        if(currentIndex.visualIndex == 0)
        {
          screen.active->cursorIndex.scroll--;
          clearScreen(screen);
        }
      }
      if(currentIndex.visualIndex > 0)
      {
        screen.active->cursorIndex.visualIndex--;
      }
    }
  } // namespace Screen
} // namespace Dirt
