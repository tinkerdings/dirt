#include <dirt/screen/screen.h>

namespace Dirt
{
  namespace Screen
  {
    void clearScreen(Screen &screen)
    {
      CHAR_INFO leftClear[MAX_PATH] = {0};
      for(int i = 0; i < screen.leftView.width; i++)
      {
        leftClear[i] = {' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE};
      }
      for(int i = 0; i < screen.leftView.nEntries; i++)
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
          printf("WriteConsoleOutput failed (%lu)\n", GetLastError());
          return;
        }
        if(!WriteConsoleOutput(screen.frontBuffer, leftClear, size, pos, &rect))
        {
          printf("WriteConsoleOutput failed (%lu)\n", GetLastError());
          return;
        }
      }
      CHAR_INFO rightClear[MAX_PATH] = {0};
      for(int i = 0; i < screen.rightView.width; i++)
      {
        rightClear[i] = {' ', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE};
      }
      for(int i = 0; i < screen.rightView.nEntries; i++)
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
          printf("WriteConsoleOutput failed (%lu)\n", GetLastError());
          return;
        }
        if(!WriteConsoleOutput(screen.frontBuffer, rightClear, size, pos, &rect))
        {
          printf("WriteConsoleOutput failed (%lu)\n", GetLastError());
          return;
        }
      }
    }


    bool allocScreen(Screen &screen)
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


    void renderScreenDirectoryViews(Screen &screen)
    {
      renderDirectoryView(screen, screen.leftView);
      renderDirectoryView(screen, screen.rightView);
    }


    void renderDirectoryView(Screen &screen, DirectoryView &view)
    {
      for(int i = 0; i < view.nEntries; i++)
      {
        CHAR_INFO filename[MAX_PATH] = {0};
        bool isDirectory = (view.entries[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        createFilenameCharInfoBuffer(filename, view.entries[i].cFileName, view.width, isDirectory);
        COORD filenameSize {view.width, 1};
        COORD filenamePos = {0, 0};
        SMALL_RECT rect;
        rect.Top = view.renderRect.Top+i;
        rect.Left = view.renderRect.Left;
        rect.Bottom = view.renderRect.Bottom;
        rect.Right = view.renderRect.Right;
        if(!WriteConsoleOutput(screen.backBuffer, filename, filenameSize, filenamePos, &rect))
        {
          printf("WriteConsoleOutput failed (%lu)\n", GetLastError());
          return;
        }
      }
    }

    void styleView(HANDLE screenBuffer, DirectoryView view)
    {
      for(int i = 0; i < view.nEntries; i++)
      {
        WIN32_FIND_DATA entry = view.entries[i];

        WORD attributes = entry.dwFileAttributes;
        
        COORD coords;
        coords.X = view.renderRect.Left;
        coords.Y = i;
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
        getFullPath(fullPath, entry.cFileName, MAX_PATH);

        size_t len = strlen(fullPath);
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

    void highlightLine(Screen &screen)
    {
      size_t cursorFilenameLength = strlen(screen.active->entries[screen.active->cursorIndex].cFileName);
      size_t emptySpace = 
        screen.active->width - cursorFilenameLength;
      COORD coords;
      DWORD nSet = 0;
      coords.X = screen.active->renderRect.Left;
      coords.Y = screen.active->cursorIndex;
      WORD attribs = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;

      char fullPath[MAX_PATH] = {0};
      getFullPath(fullPath, screen.active->entries[screen.active->cursorIndex].cFileName, MAX_PATH);

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

    void styleScreenViews(Screen &screen)
    {
      styleView(screen.backBuffer, screen.leftView);
      styleView(screen.backBuffer, screen.rightView);
      highlightLine(screen);
    }

    void swapScreenBuffers(Screen &screen)
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

    bool setActiveView(Screen &screen, DirectoryView &view)
    {
      screen.active = &view;
      if(!SetCurrentDirectory(screen.active->path))
      {
        printf("Failed to set current directory (%lu)\n", GetLastError());
        return false;
      }
      return true;
    }

    bool initScreenDirectoryViews(Screen &screen)
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
      screen.leftView.renderRect.Bottom = 80;
      screen.leftView.renderRect.Right = 40;
      screen.leftView.width = screen.leftView.renderRect.Right - screen.leftView.renderRect.Left;
      screen.leftView.height = screen.leftView.renderRect.Bottom - screen.leftView.renderRect.Top;
      screen.leftView.entries = findDirectoryEntries(screen.leftView.path, screen.leftView.nEntries);
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
      screen.rightView.renderRect.Bottom = 80;
      screen.rightView.renderRect.Right = 82;
      screen.rightView.width = screen.rightView.renderRect.Right - screen.rightView.renderRect.Left;
      screen.rightView.height = screen.rightView.renderRect.Bottom - screen.rightView.renderRect.Top;
      screen.rightView.entries = findDirectoryEntries(screen.rightView.path, screen.rightView.nEntries);
      if(!screen.rightView.entries)
      {
        printf("findDirectoryEntries failed (%lu)\n", GetLastError());
        return false;
      }

      setActiveView(screen, screen.leftView);

      return true;
    }

    void setViewPath(DirectoryView &view, char *relPath)
    {
      char fullPath[MAX_PATH] = {0};
      getFullPath(fullPath, relPath, MAX_PATH);

      if(!SetCurrentDirectory(fullPath))
      {
        printf("Failed to set current directory (%lu)\n", GetLastError());
      }

      if(view.path[0])
      {
        size_t cursorIndex = view.cursorIndex;
        DirectoryView::CursorMapEntry cursorIndexEntry;
        size_t viewPathLen = strlen(view.path);
        cursorIndexEntry.cursorIndex = cursorIndex;
        memcpy(cursorIndexEntry.path, view.path, viewPathLen);

        size_t hashIndex, dupeIndex;
        if(getViewCursorIndex(view, &hashIndex, &dupeIndex))
        {
          hashmapDirectWrite(view.cursorMap, &cursorIndexEntry, hashIndex, dupeIndex, sizeof(DirectoryView::CursorMapEntry));
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
      view.entries = findDirectoryEntries(view.path, view.nEntries);

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

    size_t getViewCursorIndex(DirectoryView &view, size_t *hashIndexOut, size_t *dupeIndexOut)
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
          if((ret = Dirt::Memory::compareBytes(entry->path, view.path, entryPathLen, viewPathLen)))
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

      return 0;
    }

    void incrementScreenCursorIndex(Screen &screen)
    {
      uint32_t currentIndex = screen.active->cursorIndex;
      if(currentIndex < (screen.active->nEntries-1))
      {
        screen.active->cursorIndex++;
      }
      else 
      {
        screen.active->cursorIndex = 0;
      }
    }

    void decrementScreenCursorIndex(Screen &screen)
    {
      uint32_t currentIndex = screen.active->cursorIndex;
      if(currentIndex > 0)
      {
        screen.active->cursorIndex--;
      }
      else 
      {
        screen.active->cursorIndex = screen.active->nEntries-1;
      }
    }
  } // namespace Screen
} // namespace Dirt
