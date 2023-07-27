#include <stdio.h>
#include <dirt/rendering/rendering.h>
#include <dirt/structures/splitBox.h>
#include <dirt/screen/screen.h>
#include <dirt/entry/entry.h>
#include <shlwapi.h>

namespace Dirt
{
  namespace Rendering
  {
    void clearScreen(Screen::ScreenData &screen)
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

    void styleScreenViews(Context *context, Screen::ScreenData &screen)
    {
      styleView(context, screen, screen.leftView);
      styleView(context, screen, screen.rightView);
      highlightLine(context, *(context->currentScreen));
    }

    void swapScreenBuffers(Screen::ScreenData &screen)
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

    // TODO: Update this to account for splitBoxes
    void sizeScreenViews(Screen::ScreenData &screen, Container container)
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

    void renderScreenViews(Screen::ScreenData &screen, Container container)
    {
      clearScreen(screen);
      renderView(screen, screen.leftView);
      renderView(screen, screen.rightView);
      renderContainerBorder(screen, container);
    }

    void renderTabsContainer(Context *context, Container *container)
    {

    }

    void refresh(Context *context, Screen::ScreenData &screen)
    {
      Rendering::clearScreen(screen);
      setViewEntries(context, screen.leftView, false);
      setViewEntries(context, screen.rightView, false);
    }

    void renderView(Screen::ScreenData &screen, Screen::View &view)
    {
      size_t minHeight = min(view.nEntries, view.height+view.cursorIndex.scroll);
      for(size_t i = view.cursorIndex.scroll; i < minHeight; i++)
      {
        CHAR_INFO filename[MAX_PATH] = {0};
        bool isDirectory = (view.entries[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        Screen::createFilenameCharInfoBuffer(filename, view.entries[i].cFileName, view.width, isDirectory);
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

    void highlightLine(Context *context, Screen::ScreenData &screen)
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
    void styleView(Context *context, Screen::ScreenData &screen, Screen::View view)
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

    void renderSplitBox(Structures::SplitBox *splitBox)
    {

    }


    void renderVerticalLineWithCharacter(Screen::ScreenData &screen, COORD startPos, COORD endPos, WCHAR *character)
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

    void renderHorizontalLineWithCharacter(Screen::ScreenData &screen, COORD startPos, COORD endPos, WCHAR *character)
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

    void renderContainerBorder(Screen::ScreenData &screen, Container container)
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
  }
}
