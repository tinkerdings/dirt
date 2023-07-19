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
  }
}
