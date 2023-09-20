#include <stdio.h>
#include <dirt/rendering/rendering.h>
#include <dirt/structures/splitBox.h>
#include <dirt/screen/screen.h>
#include <dirt/entry/entry.h>
#include <windows.h>
#include <shlwapi.h>

namespace Dirt
{
  namespace Rendering
  {
    void clearScreen(Screen::ScreenData &screen)
    {
      CHAR_INFO leftClear[MAX_PATH] = {};
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
      CHAR_INFO rightClear[MAX_PATH] = {};
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
      renderCursorLine(context, *(context->currentScreen));
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

    void renderScreenViews(Screen::ScreenData &screen)
    {
      clearScreen(screen);
      renderViewEntries(screen, screen.leftView);
      renderViewEntries(screen, screen.rightView);
    }

    void renderTabsContainer(Context *context, Container *container)
    {

    }

    void refresh(Context *context, Screen::ScreenData &screen)
    {
      clearScreen(screen);
      Screen::setViewEntries(context, screen.leftView, false);
      Screen::setViewEntries(context, screen.rightView, false);
    }

    void renderString(Context *context, Structures::Container destination, char *text, WORD charAttribs)
    {
      size_t len = strlen(text);
      CHAR_INFO *stringData = (CHAR_INFO *)malloc(len * sizeof(CHAR_INFO));

      for(size_t i = 0; i < len; i++)
      {
        stringData[i] = {(WCHAR)text[i], charAttribs};
      }

      COORD sourceDimensions = {(SHORT)len, 1};
      COORD sourcePos = {0, 0};
      SMALL_RECT destinationRect = 
      {
        destination.pos[0],
        destination.pos[1],
        len, 1
      };

      if(!WriteConsoleOutput(context->currentScreen->backBuffer, stringData, sourceDimensions, sourcePos, &destinationRect))
      {
        printf("%s%dWriteConsoleOutput failed (%lu)\n", __FILE__, __LINE__, GetLastError());

        free(stringData);
        return;
      }

      free(stringData);
    }

    void renderViewEntries(Screen::ScreenData &screen, Screen::View &view)
    {
      size_t height = min(view.nEntries, view.height+view.cursorIndex.scroll);
      for(size_t i = view.cursorIndex.scroll; i < height; i++)
      {
        CHAR_INFO filename[MAX_PATH] = {};
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

    void renderCursorLine(Context *context, Screen::ScreenData &screen)
    {
      size_t cursorFilenameLength = min(
          strlen(screen.activeView->entries[screen.activeView->cursorIndex.actualIndex].cFileName),
          screen.activeView->width);
      size_t emptySpace = (screen.activeView->width - cursorFilenameLength);
      COORD coords;
      DWORD nSet = 0;
      coords.X = screen.activeView->renderRect.Left;
      coords.Y = screen.activeView->renderRect.Top + screen.activeView->cursorIndex.visualIndex;
      WORD attribs = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;

      char fullPath[MAX_PATH] = {};
      Entry::getFullPath(
          fullPath,
          screen.activeView->entries[screen.activeView->cursorIndex.actualIndex].cFileName,
          MAX_PATH);

      if(hashmapContains(context->selection, fullPath, MAX_PATH, 0, 0))
      {
        attribs ^= FOREGROUND_RED | FOREGROUND_BLUE;
      }

      FillConsoleOutputAttribute(
        screen.backBuffer,
        attribs,
        screen.activeView->width,
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
      char prevCurDir[MAX_PATH] = {};
      GetCurrentDirectoryA(MAX_PATH, prevCurDir);
      SetCurrentDirectory(view.path);

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

        char fullPath[MAX_PATH] = {};
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

      SetCurrentDirectory(prevCurDir);
    }

    int utf8ToUtf16(char *utf8, WCHAR *utf16, int outBufSize)
    {
      int len = MultiByteToWideChar(
          CP_UTF8,
          MB_COMPOSITE | MB_USEGLYPHCHARS,
          utf8,
          -1,
          utf16,
          outBufSize
          );
      return --len;
    }

    CHAR_INFO createGlyphWithAttributes(char *utf8, WORD attributes)
    {
      WCHAR utf16[32] = {};
      int len = utf8ToUtf16(utf8, utf16, 32);
      CHAR_INFO glyph = {*utf16, attributes};
      return glyph;
    }

    void renderUnicodeCharacter(Screen::ScreenData &screen, SHORT x, SHORT y, char *character, WORD attributes)
    {
      WCHAR utf16Char[32] = {};
      int len = utf8ToUtf16(character, utf16Char, 32);

      DWORD nWritten = 0;
      COORD coords;
      coords.X = x;
      coords.Y = y;
      WriteConsoleOutputCharacterW(
          screen.backBuffer,
          utf16Char,
          len,
          coords,
          &nWritten
          );
      WriteConsoleOutputAttribute(
          screen.backBuffer,
          &attributes,
          len,
          coords,
          &nWritten
          );
    }

    void renderBox(Context *context, Screen::ScreenData &screen, Structures::Container container, Structures::BoxGlyphs glyphs, char *title)
    {
        //render corners
        renderUnicodeCharacter(
            screen,
            container.pos[0],
            container.pos[1],
            glyphs.topLeft,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        renderUnicodeCharacter(
            screen,
            container.pos[0] + container.width,
            container.pos[1],
            glyphs.topRight,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        renderUnicodeCharacter(
            screen,
            container.pos[0],
            container.pos[1] + container.height,
            glyphs.bottomLeft,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        renderUnicodeCharacter(
            screen,
            container.pos[0] + container.width,
            container.pos[1] + container.height,
            glyphs.bottomRight,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        // Top line
        renderCardinalLineWithGlyph(
            screen,
            DIRT_DIRECTION_HORIZONTAL,
            container.pos[0] + 1,
            container.pos[1],
            container.width - 1,
            glyphs.horizontal);
        // Bottom line
        renderCardinalLineWithGlyph(
            screen,
            DIRT_DIRECTION_HORIZONTAL,
            container.pos[0] + 1,
            container.pos[1] + container.height,
            container.width - 1,
            glyphs.horizontal);
        // Left line
        renderCardinalLineWithGlyph(
            screen,
            DIRT_DIRECTION_VERTICAL,
            container.pos[0],
            container.pos[1] + 1,
            container.height - 1,
            glyphs.vertical);
        // Right line
        renderCardinalLineWithGlyph(
            screen,
            DIRT_DIRECTION_VERTICAL,
            container.pos[0] + container.width,
            container.pos[1] + 1,
            container.height - 1,
            glyphs.vertical);

        if(title)
        {
          size_t len = strlen(title);
          Structures::Container titleContainer = 
          {
            {container.pos[0] + 10, container.pos[1]},
            len, 10
          };
          renderString(context, titleContainer, title, FOREGROUND_RED);
        }
    }

    void renderSplitBox(Context *context, Screen::ScreenData &screen, Structures::SplitBox *splitBox)
    {
      if(!splitBox->parent)
      {
        renderBox(context, screen, splitBox->frameContainer, splitBox->glyphs, splitBox->title);
      }

      if(splitBox->splitType != DIRT_SPLIT_NONE)
      {
        switch(splitBox->splitType)
        {
          case(DIRT_SPLIT_HORIZONTAL):
          {
            renderUnicodeCharacter(
                screen,
                splitBox->frameContainer.pos[0],
                splitBox->childB->frameContainer.pos[1],
                splitBox->glyphs.splitHorizontalLeft,
                FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            renderUnicodeCharacter(
                screen,
                splitBox->frameContainer.pos[0] + splitBox->frameContainer.width,
                splitBox->childB->frameContainer.pos[1],
                splitBox->glyphs.splitHorizontalRight,
                FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

            renderCardinalLineWithGlyph(
                screen,
                DIRT_DIRECTION_HORIZONTAL,
                splitBox->frameContainer.pos[0] + 1,
                splitBox->childB->frameContainer.pos[1],
                splitBox->frameContainer.width - 1,
                splitBox->glyphs.horizontal);
          } break;
          case(DIRT_SPLIT_VERTICAL):
          {
            renderUnicodeCharacter(
                screen,
                splitBox->childB->frameContainer.pos[0],
                splitBox->frameContainer.pos[1],
                splitBox->glyphs.splitVerticalTop,
                FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            renderUnicodeCharacter(
                screen,
                splitBox->childB->frameContainer.pos[0],
                splitBox->frameContainer.pos[1] + splitBox->frameContainer.height,
                splitBox->glyphs.splitVerticalBottom,
                FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

            renderCardinalLineWithGlyph(
                screen,
                DIRT_DIRECTION_VERTICAL,
                splitBox->childB->frameContainer.pos[0],
                splitBox->frameContainer.pos[1] + 1,
                splitBox->frameContainer.height - 1,
                splitBox->glyphs.vertical);
          } break;
        }

        if(splitBox->parent)
        {
          SplitBox *childA = splitBox->parent->childA;
          SplitBox *childB = splitBox->parent->childB;
          if(childA->splitType == childB->splitType)
          {
            uint8_t coord = 0;
            if(childA->splitType == DIRT_SPLIT_HORIZONTAL)
            {
              coord++;
            }

            if(childA->childB->frameContainer.pos[coord] == childB->childB->frameContainer.pos[coord])
            {
              renderUnicodeCharacter(
                  screen,
                  childB->childB->frameContainer.pos[0],
                  childB->childB->frameContainer.pos[1],
                  splitBox->glyphs.splitCross,
                  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
          }
        }

        renderSplitBox(context, screen, splitBox->childA);
        renderSplitBox(context, screen, splitBox->childB);
      }
    }

    void renderCardinalLineWithGlyph(
        Screen::ScreenData &screen,
        uint8_t direction, // DIRT_DIRECTION_HORIZONTAL or DIRT_DIRECTION_VERTICAL
        SHORT startX,
        SHORT startY,
        SHORT signedLength, // Signed length along specified axis, by direction parameter
        char *character)
    {
      CHAR_INFO buffer[2048] = {};

      int len = signedLength;
      if(signedLength < 0)
      {
        len = -1*signedLength;
      }
      if(len < 1)
      {
        len = 1;
      }
      for(int i = 0; i < min(len, 2048); i++)
      {
        CHAR_INFO ci = createGlyphWithAttributes(character, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        buffer[i] = ci;
      }

      COORD dimensions;
      COORD start;
      COORD end;
      switch(direction)
      {
        case(DIRT_DIRECTION_HORIZONTAL):
        {
          start.X = startX;
          start.Y = startY;
          end.X = start.X + signedLength;
          end.Y = start.Y;
          dimensions.X = (SHORT)len;
          dimensions.Y = 1;
        } break;
        case(DIRT_DIRECTION_VERTICAL):
        {
          start.X = startX;
          start.Y = startY;
          end.X = start.X;
          end.Y = start.Y + signedLength;
          dimensions.X = 1;
          dimensions.Y = (SHORT)len;
        } break;
      }
      COORD bufferStartPos = {0, 0};
      SMALL_RECT rect = {start.X, start.Y, end.X, end.Y};

      if(!WriteConsoleOutputW(screen.backBuffer, buffer, dimensions, bufferStartPos, &rect))
      {
        printf("%s%dWriteConsoleOutput failed (%lu)\n", __FILE__, __LINE__, GetLastError());
        return;
      }
    }

    void fillContainer(Context *context, Structures::Container container, WORD charAttribs)
    {
      CHAR_INFO clearChar = {' ', charAttribs};
      size_t size = (container.width + 1) * (container.height + 1);
      CHAR_INFO *clearArea = (CHAR_INFO *)malloc(size * sizeof(CHAR_INFO));
      for(size_t i = 0; i < size; i++)
      {
        clearArea[i] = clearChar;
      }
      
      COORD sourcePos = {0, 0};
      SMALL_RECT destRect;
      COORD sourceSize = {(SHORT)container.width + 1, (SHORT)container.height + 1};
      destRect.Top = container.pos[1];
      destRect.Left = container.pos[0];
      destRect.Bottom = destRect.Top + container.height;
      destRect.Right = destRect.Left + container.width;
      if(!WriteConsoleOutput(context->currentScreen->backBuffer, clearArea, sourceSize, sourcePos, &destRect))
      {
        printf("%s%d WriteConsoleOutput failed (%lu)\n", __FILE__, __LINE__, GetLastError());
        free(clearArea);
        return;
      }

      free(clearArea);
    }

    void renderDriveSwitcher(Context *context)
    {
      uint16_t consoleWidth = 0;
      uint16_t consoleHeight = 0;
      Screen::getConsoleDimensions(consoleWidth, consoleHeight);
      uint16_t promptWidth = (uint16_t)(consoleWidth * 0.4);
      uint16_t promptHeight = (uint16_t)(consoleHeight * 0.4);

      Structures::Container promptContainer = 
      {
        {
          (uint16_t)((consoleWidth/2)-(promptWidth/2)),
          (uint16_t)((consoleHeight/2)-(promptHeight/2))
        },
        promptWidth,
        promptHeight
      };

      renderBox(context, *context->currentScreen, promptContainer, context->standardGlyphs, "Select Drive");

      Container innerContainer = 
      {
        {
          (uint16_t)(promptContainer.pos[0]+1),
          (uint16_t)(promptContainer.pos[1]+1)
        },
        (uint16_t)(promptContainer.width-2),
        (uint16_t)(promptContainer.height-2)
      };

      fillContainer(context, innerContainer, 0);
    }
  } // namespace Rendering
} // namespace Dirt
