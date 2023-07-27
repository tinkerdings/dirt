#ifndef RENDERING_H
#define RENDERING_H

#include <stdint.h>
#include <windows.h>
#include <dirt/structures/splitBox.h>
#include <dirt/screen/screen.h>

namespace Dirt
{
  namespace Rendering
  {
    void renderSplitBox(Structures::SplitBox *splitBox);
    void renderScreenViews(Screen::ScreenData &screen, Container container);
    void renderTabsContainer(Context *context, Container *container);
    void renderContainerBorder(Screen::ScreenData &screen, Container container);
    void renderHorizontalLineWithCharacter(Screen::ScreenData &screen, COORD startPos, COORD endPos, WCHAR *character);
    void renderVerticalLineWithCharacter(Screen::ScreenData &screen, COORD startPos, COORD endPos, WCHAR *character);
    void renderView(Screen::ScreenData &screen, Screen::View &view);
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
