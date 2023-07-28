#ifndef RENDERING_H
#define RENDERING_H

#include <stdint.h>
#include <windows.h>
#include <dirt/structures/splitBox.h>
#include <dirt/screen/screen.h>

#define DIRT_DIRECTION_HORIZONTAL 1
#define DIRT_DIRECTION_VERTICAL 2

namespace Dirt
{
  namespace Rendering
  {
    void renderSplitBox(Screen::ScreenData &screen, Structures::SplitBox *splitBox);
    void renderScreenViews(Screen::ScreenData &screen, Container container);
    void renderTabsContainer(Context *context, Container *container);
    void renderContainerBorder(Screen::ScreenData &screen, Container container);
    void renderCardinalLineWithGlyph(
        Screen::ScreenData &screen,
        uint8_t direction, // DIRT_DIRECTION_HORIZONTAL or DIRT_DIRECTION_VERTICAL
        SHORT startX,
        SHORT startY,
        SHORT signedLength, // Signed length along specified axis, by direction parameter
        char *character);
    void renderView(Screen::ScreenData &screen, Screen::View &view);
    int utf8ToUtf16(char *utf8, WCHAR *utf16, int outBufSize);
    CHAR_INFO createGlyphWithAttributes(char *utf8, WORD attributes);
    void renderUnicodeCharacter(Screen::ScreenData &screen, SHORT x, SHORT y, char *character, WORD attributes);
    void styleView(Context *context, Screen::ScreenData &screen, Screen::View view);
    void clearScreen(Screen::ScreenData &screen);
    void styleScreenViews(Context *context, Screen::ScreenData &screen);
    void sizeScreenViews(Screen::ScreenData &screen, Container container);
    void highlightLine(Context *context, Screen::ScreenData &screen);
    void swapScreenBuffers(Screen::ScreenData &screen);
    void refresh(Context *context, Screen::ScreenData &screen);
  }
}

#endif // RENDERING_H
